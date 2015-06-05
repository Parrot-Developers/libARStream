/*
    Copyright (C) 2014 Parrot SA

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in
      the documentation and/or other materials provided with the
      distribution.
    * Neither the name of Parrot nor the names
      of its contributors may be used to endorse or promote products
      derived from this software without specific prior written
      permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
    "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
    LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
    FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
    COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
    BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
    OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
    AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
    OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
    OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
    SUCH DAMAGE.
*/
/**
 * @file ARSTREAM_Sender2.c
 * @brief Stream sender over network (v2)
 * @date 04/17/2015
 * @author aurelien.barre@parrot.com
 */

#include <config.h>

/*
 * System Headers
 */

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>

/*
 * Private Headers
 */

#include "ARSTREAM_Buffers.h"
#include "ARSTREAM_NetworkHeaders.h"

/*
 * ARSDK Headers
 */

#include <libARStream/ARSTREAM_Sender2.h>
#include <libARSAL/ARSAL_Mutex.h>
#include <libARSAL/ARSAL_Print.h>
#include <libARSAL/ARSAL_Endianness.h>

/*
 * Macros
 */

/**
 * Tag for ARSAL_PRINT
 */
#define ARSTREAM_SENDER2_TAG "ARSTREAM_Sender2"

/**
 * Maximum number of NAL units in the FIFO
 */
#define ARSTREAM_SENDER2_DEFAULT_NALU_FIFO_SIZE (128 * 32)

/**
 * Sets *PTR to VAL if PTR is not null
 */
#define SET_WITH_CHECK(PTR,VAL)                 \
    do                                          \
    {                                           \
        if (PTR != NULL)                        \
        {                                       \
            *PTR = VAL;                         \
        }                                       \
    } while (0)


typedef struct ARSTREAM_Sender2_Nalu_s {
    uint8_t *naluBuffer;
    uint32_t naluSize;
    uint64_t auTimestamp;
    int isLastInAu;
    void *auUserPtr;

    struct ARSTREAM_Sender2_Nalu_s* prev;
    struct ARSTREAM_Sender2_Nalu_s* next;
    int used;
} ARSTREAM_Sender2_Nalu_t;


typedef struct {
    ARSTREAM_Sender2_t *sender;
    uint64_t auTimestamp;
    int isLastInAu;
    void *auUserPtr;
} ARSTREAM_Sender2_NetworkCallbackParam_t;


struct ARSTREAM_Sender2_t {
    /* Configuration on New */
    ARSTREAM_Sender2_NetworkMode_t networkMode;
    ARNETWORK_Manager_t *manager;
    int dataBufferID;
    int ackBufferID;
    char *ifaceAddr;
    char *sendAddr;
    int sendPort;
    ARSTREAM_Sender2_AuCallback_t auCallback;
    int maxPacketSize;
    int targetPacketSize;
    int naluFifoSize;
    int maxBitrate;
    int maxLatencyMs;
    void *custom;

    uint64_t lastAuTimestamp;
    uint64_t firstTimestamp;
    uint16_t seqNum;
    ARSAL_Mutex_t streamMutex;

    /* Thread status */
    int threadsShouldStop;
    int dataThreadStarted;
    int ackThreadStarted;

    /* Sockets */
    int sendMulticast;
    struct sockaddr_in sendSin;
    int sendSocket;
    int recvSocket;
    
    /* NAL unit FIFO */
    ARSAL_Mutex_t fifoMutex;
    ARSAL_Cond_t fifoCond;
    int fifoCount;
    ARSTREAM_Sender2_Nalu_t *fifoHead;
    ARSTREAM_Sender2_Nalu_t *fifoTail;
    ARSTREAM_Sender2_Nalu_t *fifoPool;
};


static int ARSTREAM_Sender2_EnqueueNalu(ARSTREAM_Sender2_t *sender, const ARSTREAM_Sender2_Nalu_t* nalu)
{
    int i;
    ARSTREAM_Sender2_Nalu_t* cur = NULL;

    if ((!sender) || (!nalu))
    {
        return -1;
    }

    ARSAL_Mutex_Lock(&(sender->fifoMutex));

    if (sender->fifoCount >= sender->naluFifoSize)
    {
        ARSAL_Mutex_Unlock(&(sender->fifoMutex));
        return -2;
    }

    for (i = 0; i < sender->naluFifoSize; i++)
    {
        if (!sender->fifoPool[i].used)
        {
            cur = &sender->fifoPool[i];
            memcpy(cur, nalu, sizeof(ARSTREAM_Sender2_Nalu_t));
            cur->prev = NULL;
            cur->next = NULL;
            cur->used = 1;
            break;
        }
    }

    if (!cur)
    {
        ARSAL_Mutex_Unlock(&(sender->fifoMutex));
        return -3;
    }

    sender->fifoCount++;
    if (sender->fifoTail)
    {
        sender->fifoTail->next = cur;
        cur->prev = sender->fifoTail;
    }
    sender->fifoTail = cur; 
    if (!sender->fifoHead)
    {
        sender->fifoHead = cur;
    }

    ARSAL_Mutex_Unlock(&(sender->fifoMutex));
    ARSAL_Cond_Signal(&(sender->fifoCond));

    return 0;
}


static int ARSTREAM_Sender2_DequeueNalu(ARSTREAM_Sender2_t *sender, ARSTREAM_Sender2_Nalu_t* nalu)
{
    ARSTREAM_Sender2_Nalu_t* cur = NULL;
    
    if ((!sender) || (!nalu))
    {
        return -1;
    }
    
    ARSAL_Mutex_Lock(&(sender->fifoMutex));
    
    if (!sender->fifoHead)
    {
        ARSAL_Mutex_Unlock(&sender->fifoMutex);
        return -2;
    }
    
    cur = sender->fifoHead;
    if (cur->next)
    {
        cur->next->prev = NULL;
    }
    sender->fifoHead = cur->next;
    if (!sender->fifoHead)
    {
        sender->fifoTail = NULL;
    }
    sender->fifoCount--;
    
    cur->used = 0;
    cur->prev = NULL;
    cur->next = NULL;
    memcpy(nalu, cur, sizeof(ARSTREAM_Sender2_Nalu_t));
    
    ARSAL_Mutex_Unlock (&(sender->fifoMutex));
    
    return 0;
}


static int ARSTREAM_Sender2_FlushNaluFifo(ARSTREAM_Sender2_t *sender)
{
    if (!sender)
    {
        return -1;
    }

    int fifoRes;
    ARSTREAM_Sender2_Nalu_t nalu;

    while ((fifoRes = ARSTREAM_Sender2_DequeueNalu(sender, &nalu)) == 0)
    {
        /* last NALU in the Access Unit: call the auCallback */
        if (nalu.isLastInAu)
        {
            ARSAL_Mutex_Lock(&(sender->streamMutex));
            if (nalu.auTimestamp != sender->lastAuTimestamp)
            {
                sender->lastAuTimestamp = nalu.auTimestamp;
                ARSAL_Mutex_Unlock(&(sender->streamMutex));

                /* call the auCallback */
                ARSAL_PRINT(ARSAL_PRINT_ERROR, ARSTREAM_SENDER2_TAG, "Time %llu: flushing NALU FIFO -> auCallback", nalu.auTimestamp);
                sender->auCallback(ARSTREAM_SENDER2_STATUS_AU_CANCELLED, nalu.auUserPtr, sender->custom);
            }
            else
            {
                ARSAL_Mutex_Unlock(&(sender->streamMutex));
            }
        }
    }

    return 0;
}


eARNETWORK_MANAGER_CALLBACK_RETURN ARSTREAM_Sender2_NetworkCallback(int IoBufferId, uint8_t *dataPtr, void *customData, eARNETWORK_MANAGER_CALLBACK_STATUS status)
{
    eARNETWORK_MANAGER_CALLBACK_RETURN retVal = ARNETWORK_MANAGER_CALLBACK_RETURN_DEFAULT;

    return retVal;
}


void ARSTREAM_Sender2_InitStreamDataBuffer(ARNETWORK_IOBufferParam_t *bufferParams, int bufferID, int maxPacketSize)
{
    ARSTREAM_Buffers_InitStreamDataBuffer(bufferParams, bufferID, sizeof(ARSTREAM_NetworkHeaders_DataHeader2_t), maxPacketSize, ARSTREAM_BUFFERS_DATA_BUFFER_NUMBER_OF_CELLS);
}


void ARSTREAM_Sender2_InitStreamAckBuffer(ARNETWORK_IOBufferParam_t *bufferParams, int bufferID)
{
    ARSTREAM_Buffers_InitStreamAckBuffer(bufferParams, bufferID);
}


ARSTREAM_Sender2_t* ARSTREAM_Sender2_New(ARSTREAM_Sender2_Config_t *config, void *custom, eARSTREAM_ERROR *error)
{
    ARSTREAM_Sender2_t *retSender = NULL;
    int streamMutexWasInit = 0;
    int fifoWasCreated = 0;
    int fifoMutexWasInit = 0;
    int fifoCondWasInit = 0;
    eARSTREAM_ERROR internalError = ARSTREAM_OK;
    /* ARGS Check */
    if ((config == NULL) ||
        ((config->networkMode == ARSTREAM_SENDER2_NETWORK_MODE_ARNETWORK) &&
            (config->manager == NULL)) ||
        ((config->networkMode == ARSTREAM_SENDER2_NETWORK_MODE_SOCKET) &&
            ((config->sendAddr == NULL) || (strlen(config->sendAddr) <= 0) || (config->sendPort <= 0))) ||
        (config->auCallback == NULL) ||
        (config->maxPacketSize < 100) ||
        (config->targetPacketSize < 100) ||
        (config->maxBitrate <= 0) ||
        (config->maxLatencyMs <= 0) ||
        (config->naluFifoSize <= 0))
    {
        SET_WITH_CHECK(error, ARSTREAM_ERROR_BAD_PARAMETERS);
        return retSender;
    }

    /* Alloc new sender */
    retSender = malloc(sizeof(ARSTREAM_Sender2_t));
    if (retSender == NULL)
    {
        internalError = ARSTREAM_ERROR_ALLOC;
    }

    /* Initialize the sender and copy parameters */
    if (internalError == ARSTREAM_OK)
    {
        memset(retSender, 0, sizeof(ARSTREAM_Sender2_t));
        retSender->sendMulticast = 0;
        retSender->sendSocket = -1;
        retSender->recvSocket = -1;
        retSender->networkMode = config->networkMode;
        retSender->manager = config->manager;
        retSender->dataBufferID = config->dataBufferID;
        retSender->ackBufferID = config->ackBufferID;
        if (config->networkMode == ARSTREAM_SENDER2_NETWORK_MODE_SOCKET)
        {
            if (config->sendAddr)
            {
                retSender->sendAddr = strndup(config->sendAddr, 16);
            }
            if (config->ifaceAddr)
            {
                retSender->ifaceAddr = strndup(config->ifaceAddr, 16);
            }
        }
        retSender->sendPort = config->sendPort;
        retSender->auCallback = config->auCallback;
        retSender->naluFifoSize = config->naluFifoSize;
        retSender->maxPacketSize = config->maxPacketSize - sizeof(ARSTREAM_NetworkHeaders_DataHeader2_t);           //TODO: include all headers size
        retSender->targetPacketSize = config->targetPacketSize - sizeof(ARSTREAM_NetworkHeaders_DataHeader2_t);     //TODO: include all headers size
        retSender->maxBitrate = config->maxBitrate;
        retSender->maxLatencyMs = config->maxLatencyMs;
        retSender->custom = custom;
    }

    /* Setup internal mutexes/sems */
    if (internalError == ARSTREAM_OK)
    {
        int mutexInitRet = ARSAL_Mutex_Init(&(retSender->streamMutex));
        if (mutexInitRet != 0)
        {
            internalError = ARSTREAM_ERROR_ALLOC;
        }
        else
        {
            streamMutexWasInit = 1;
        }
    }

    /* Setup the NAL unit FIFO */
    if (internalError == ARSTREAM_OK)
    {
        retSender->fifoPool = malloc(config->naluFifoSize * sizeof(ARSTREAM_Sender2_Nalu_t));
        if (retSender->fifoPool == NULL)
        {
            internalError = ARSTREAM_ERROR_ALLOC;
        }
        else
        {
            fifoWasCreated = 1;
            memset(retSender->fifoPool, 0, config->naluFifoSize * sizeof(ARSTREAM_Sender2_Nalu_t));
        }
    }
    if (internalError == ARSTREAM_OK)
    {
        int mutexInitRet = ARSAL_Mutex_Init(&(retSender->fifoMutex));
        if (mutexInitRet != 0)
        {
            internalError = ARSTREAM_ERROR_ALLOC;
        }
        else
        {
            fifoMutexWasInit = 1;
        }
    }
    if (internalError == ARSTREAM_OK)
    {
        int condInitRet = ARSAL_Cond_Init(&(retSender->fifoCond));
        if (condInitRet != 0)
        {
            internalError = ARSTREAM_ERROR_ALLOC;
        }
        else
        {
            fifoCondWasInit = 1;
        }
    }

    if ((internalError != ARSTREAM_OK) &&
        (retSender != NULL))
    {
        if (streamMutexWasInit == 1)
        {
            ARSAL_Mutex_Destroy(&(retSender->streamMutex));
        }
        if (fifoWasCreated == 1)
        {
            free(retSender->fifoPool);
        }
        if (fifoMutexWasInit == 1)
        {
            ARSAL_Mutex_Destroy(&(retSender->fifoMutex));
        }
        if (fifoCondWasInit == 1)
        {
            ARSAL_Cond_Destroy(&(retSender->fifoCond));
        }
        if ((retSender) && (retSender->sendAddr))
        {
            free(retSender->sendAddr);
        }
        if ((retSender) && (retSender->ifaceAddr))
        {
            free(retSender->ifaceAddr);
        }
        free(retSender);
        retSender = NULL;
    }

    SET_WITH_CHECK(error, internalError);
    return retSender;
}


void ARSTREAM_Sender2_StopSender(ARSTREAM_Sender2_t *sender)
{
    if (sender != NULL)
    {
        ARSAL_Mutex_Lock(&(sender->streamMutex));
        sender->threadsShouldStop = 1;
        ARSAL_Mutex_Unlock(&(sender->streamMutex));
        /* signal the sending thread to avoid a deadlock */
        ARSAL_Cond_Signal(&(sender->fifoCond));
    }
}


eARSTREAM_ERROR ARSTREAM_Sender2_Delete(ARSTREAM_Sender2_t **sender)
{
    eARSTREAM_ERROR retVal = ARSTREAM_ERROR_BAD_PARAMETERS;
    if ((sender != NULL) &&
        (*sender != NULL))
    {
        int canDelete = 0;
        ARSAL_Mutex_Lock(&((*sender)->streamMutex));
        if (((*sender)->dataThreadStarted == 0) &&
            ((*sender)->ackThreadStarted == 0))
        {
            canDelete = 1;
        }
        ARSAL_Mutex_Unlock(&((*sender)->streamMutex));

        if (canDelete == 1)
        {
            ARSAL_Mutex_Destroy(&((*sender)->streamMutex));
            free((*sender)->fifoPool);
            ARSAL_Mutex_Destroy(&((*sender)->fifoMutex));
            ARSAL_Cond_Destroy(&((*sender)->fifoCond));
            if ((*sender)->sendSocket != -1)
            {
                ARSAL_Socket_Close((*sender)->sendSocket);
                (*sender)->sendSocket = -1;
            }
            if ((*sender)->recvSocket != -1)
            {
                ARSAL_Socket_Close((*sender)->recvSocket);
                (*sender)->recvSocket = -1;
            }
            if ((*sender)->sendAddr)
            {
                free((*sender)->sendAddr);
            }
            if ((*sender)->ifaceAddr)
            {
                free((*sender)->ifaceAddr);
            }
            free(*sender);
            *sender = NULL;
            retVal = ARSTREAM_OK;
        }
        else
        {
            ARSAL_PRINT(ARSAL_PRINT_ERROR, ARSTREAM_SENDER2_TAG, "Call ARSTREAM_Sender2_StopSender before calling this function");
            retVal = ARSTREAM_ERROR_BUSY;
        }
    }
    return retVal;
}


eARSTREAM_ERROR ARSTREAM_Sender2_SendNewNalu(ARSTREAM_Sender2_t *sender, uint8_t *naluBuffer, uint32_t naluSize, uint64_t auTimestamp, int isLastInAu, void *auUserPtr)
{
    eARSTREAM_ERROR retVal = ARSTREAM_OK;
    // Args check
    if ((sender == NULL) ||
        (naluBuffer == NULL) ||
        (naluSize == 0) ||
        (auTimestamp == 0))
    {
        retVal = ARSTREAM_ERROR_BAD_PARAMETERS;
    }

    ARSAL_Mutex_Lock(&(sender->streamMutex));
    if (!sender->dataThreadStarted)
    {
        retVal = ARSTREAM_ERROR_BAD_PARAMETERS;
    }
    ARSAL_Mutex_Unlock(&(sender->streamMutex));

    if (retVal == ARSTREAM_OK)
    {
        int res;
        ARSTREAM_Sender2_Nalu_t nalu;
        memset(&nalu, 0, sizeof(nalu));

        nalu.naluBuffer = naluBuffer;
        nalu.naluSize = naluSize;
        nalu.auTimestamp = auTimestamp;
        nalu.isLastInAu = isLastInAu;
        nalu.auUserPtr = auUserPtr;

        res = ARSTREAM_Sender2_EnqueueNalu(sender, &nalu);
        if (res < 0)
        {
            retVal = ARSTREAM_ERROR_QUEUE_FULL;
        }
    }
    return retVal;
}


eARSTREAM_ERROR ARSTREAM_Sender2_FlushNaluQueue(ARSTREAM_Sender2_t *sender)
{
    eARSTREAM_ERROR retVal = ARSTREAM_OK;

    // Args check
    if (sender == NULL)
    {
        retVal = ARSTREAM_ERROR_BAD_PARAMETERS;
    }

    int ret = ARSTREAM_Sender2_FlushNaluFifo(sender);
    if (ret != 0)
    {
        retVal = ARSTREAM_ERROR_BAD_PARAMETERS;
    }

    return retVal;
}


static int ARSTREAM_Sender2_Connect(ARSTREAM_Sender2_t *sender)
{
    int ret = 0;

    if (sender->networkMode == ARSTREAM_SENDER2_NETWORK_MODE_SOCKET)
    {
        int err;

        /* check parameters */
        //TODO

        /* create socket */
        sender->sendSocket = ARSAL_Socket_Create(AF_INET, SOCK_DGRAM, 0);
        if (sender->sendSocket < 0)
        {
            ret = -1;
        }

        /* initialize socket */
#if HAVE_DECL_SO_NOSIGPIPE
        if (ret == 0)
        {
            /* remove SIGPIPE */
            int set = 1;
            err = ARSAL_Socket_Setsockopt(sender->sendSocket, SOL_SOCKET, SO_NOSIGPIPE, (void*)&set, sizeof(int));
            if (err != 0)
            {
                ARSAL_PRINT(ARSAL_PRINT_ERROR, ARSTREAM_SENDER2_TAG, "Error on setsockopt: error=%d (%s)", errno, strerror(errno));
            }
        }
#endif

        if (ret == 0)
        {
            /* send address */
            memset(&sender->sendSin, 0, sizeof(struct sockaddr_in));
            sender->sendSin.sin_family = AF_INET;
            sender->sendSin.sin_port = htons(sender->sendPort);
            err = inet_pton(AF_INET, sender->sendAddr, &(sender->sendSin.sin_addr));
            if (err <= 0)
            {
                ARSAL_PRINT(ARSAL_PRINT_ERROR, ARSTREAM_SENDER2_TAG, "Failed to convert address '%s'", sender->sendAddr);
                ret = -1;
            }
        }

        if (ret == 0)
        {
            /* set to non-blocking */
            int flags = fcntl(sender->sendSocket, F_GETFL, 0);
            fcntl(sender->sendSocket, F_SETFL, flags | O_NONBLOCK);

            int addrFirst = atoi(sender->sendAddr);
            if ((addrFirst >= 224) && (addrFirst <= 239))
            {
                /* multicast */

                if ((sender->ifaceAddr) && (strlen(sender->ifaceAddr) > 0))
                {
                    /* source address */
                    struct sockaddr_in addr;
                    memset(&addr, 0, sizeof(addr));
                    addr.sin_family = AF_INET;
                    addr.sin_port = htons(0);
                    err = inet_pton(AF_INET, sender->ifaceAddr, &(addr.sin_addr));
                    if (err <= 0)
                    {
                        ARSAL_PRINT(ARSAL_PRINT_ERROR, ARSTREAM_SENDER2_TAG, "Failed to convert address '%s'", sender->ifaceAddr);
                        ret = -1;
                    }

                    if (ret == 0)
                    {
                        /* bind the socket */
                        err = ARSAL_Socket_Bind(sender->sendSocket, (struct sockaddr*)&addr, sizeof(addr));
                        if (err != 0)
                        {
                            ARSAL_PRINT(ARSAL_PRINT_ERROR, ARSTREAM_SENDER2_TAG, "Error on socket bind to addr='%s': error=%d (%s)", sender->ifaceAddr, errno, strerror(errno));
                            ret = -1;
                        }
                        sender->sendMulticast = 1;
                    }
                }
                else
                {
                    ARSAL_PRINT(ARSAL_PRINT_ERROR, ARSTREAM_SENDER2_TAG, "Trying to send multicast to address '%s' without an interface address", sender->sendAddr);
                    ret = -1;
                }
            }
            else
            {
                /* unicast */

                /* connect the socket */
                err = ARSAL_Socket_Connect(sender->sendSocket, (struct sockaddr*)&sender->sendSin, sizeof(sender->sendSin));
                if (err != 0)
                {
                    ARSAL_PRINT(ARSAL_PRINT_ERROR, ARSTREAM_SENDER2_TAG, "Error on socket connect to addr='%s' port=%d: error=%d (%s)", sender->sendAddr, sender->sendPort, errno, strerror(errno));
                    ret = -1;
                }                
            }
        }

        if (ret != 0)
        {
            if (sender->sendSocket >= 0)
            {
                ARSAL_Socket_Close(sender->sendSocket);
            }
            sender->sendSocket = -1;
        }
    }
    else if (sender->networkMode == ARSTREAM_SENDER2_NETWORK_MODE_ARNETWORK)
    {
        /* nothing to do */
    }

    return ret;
}


static int ARSTREAM_Sender2_SendData(ARSTREAM_Sender2_t *sender, uint8_t *sendBuffer, uint32_t sendSize, uint64_t auTimestamp, int isLastInAu)
{
    int ret = 0;
    ARSTREAM_NetworkHeaders_DataHeader2_t *header = (ARSTREAM_NetworkHeaders_DataHeader2_t *)sendBuffer;
    uint16_t flags;

    /* header */
    flags = 0x8060; /* with PT=96 */
    if (isLastInAu)
    {
        /* set the marker bit */
        flags |= (1 << 7);
    }
    header->flags = htons(flags);
    header->seqNum = htons(sender->seqNum++);
    if (sender->firstTimestamp == 0)
    {
        sender->firstTimestamp = auTimestamp;
    }
    header->timestamp = htonl((uint32_t)(((((auTimestamp - sender->firstTimestamp) * 90) + 500) / 1000) & 0xFFFFFFFF)); /* microseconds to 90000 Hz clock */
    header->ssrc = htonl(ARSTREAM_NETWORK_HEADERS2_SSRC);

    /* send to the network layer */
    if (sender->networkMode == ARSTREAM_SENDER2_NETWORK_MODE_SOCKET)
    {
        ssize_t bytes;
        if (sender->sendMulticast)
        {
            bytes = ARSAL_Socket_Sendto(sender->sendSocket, sendBuffer, sendSize, 0, (struct sockaddr*)&sender->sendSin, sizeof(sender->sendSin));
        }
        else
        {
            bytes = ARSAL_Socket_Send(sender->sendSocket, sendBuffer, sendSize, 0);
        }
        if (bytes < 0)
        {
            switch (errno)
            {
            case EAGAIN:
                ARSAL_PRINT(ARSAL_PRINT_WARNING, ARSTREAM_SENDER2_TAG, "Socket buffer full: error=%d (%s)", errno, strerror(errno));
                //TODO
                break;
            default:
                ARSAL_PRINT(ARSAL_PRINT_ERROR, ARSTREAM_SENDER2_TAG, "Socket send error: error=%d (%s)", errno, strerror(errno));
                //TODO
                break;
            }
            ret = -1;
        }
    }
    else if (sender->networkMode == ARSTREAM_SENDER2_NETWORK_MODE_ARNETWORK)
    {
        eARNETWORK_ERROR netError = ARNETWORK_OK;
        netError = ARNETWORK_Manager_SendData(sender->manager, sender->dataBufferID, sendBuffer, sendSize, NULL, ARSTREAM_Sender2_NetworkCallback, 1); //TODO: nocopy
        if (netError != ARNETWORK_OK)
        {
            ARSAL_PRINT(ARSAL_PRINT_ERROR, ARSTREAM_SENDER2_TAG, "Error occurred during sending of the packet; error: %d (%s)", netError, ARNETWORK_Error_ToString(netError));
            ret = -1;
        }
    }

    return ret;
}


void* ARSTREAM_Sender2_RunDataThread (void *ARSTREAM_Sender2_t_Param)
{
    /* Local declarations */
    ARSTREAM_Sender2_t *sender = (ARSTREAM_Sender2_t*)ARSTREAM_Sender2_t_Param;
    uint8_t *sendBuffer = NULL;
    uint32_t sendSize = 0;
    ARSTREAM_Sender2_Nalu_t nalu;
    uint64_t previousTimestamp = 0;
    void *previousAuUserPtr = NULL;
    int shouldStop;
    int targetPacketSize, maxPacketSize, packetSize, fragmentSize, meanFragmentSize, offset, fragmentOffset, fragmentCount;
    int ret;

    /* Parameters check */
    if (sender == NULL)
    {
        ARSAL_PRINT(ARSAL_PRINT_ERROR, ARSTREAM_SENDER2_TAG, "Error while starting %s, bad parameters", __FUNCTION__);
        return (void *)0;
    }

    /* Alloc and check */
    sendBuffer = malloc(sender->maxPacketSize + sizeof(ARSTREAM_NetworkHeaders_DataHeader2_t));
    if (sendBuffer == NULL)
    {
        ARSAL_PRINT(ARSAL_PRINT_ERROR, ARSTREAM_SENDER2_TAG, "Error while starting %s, cannot allocate memory", __FUNCTION__);
        return (void*)0;
    }

    /* Connection */
    ret = ARSTREAM_Sender2_Connect(sender);
    if (ret != 0)
    {
        ARSAL_PRINT(ARSAL_PRINT_ERROR, ARSTREAM_SENDER2_TAG, "Failed to connect (error %d) - aborting", ret);
        free(sendBuffer);
        return (void*)0;
    }

    ARSAL_PRINT(ARSAL_PRINT_ERROR, ARSTREAM_SENDER2_TAG, "Sender thread running"); //TODO: debug
    ARSAL_Mutex_Lock(&(sender->streamMutex));
    sender->dataThreadStarted = 1;
    shouldStop = sender->threadsShouldStop;
    ARSAL_Mutex_Unlock(&(sender->streamMutex));

    while (shouldStop == 0)
    {
        int fifoRes = ARSTREAM_Sender2_DequeueNalu(sender, &nalu);

        if (fifoRes == 0)
        {
            if ((previousTimestamp != 0) && (nalu.auTimestamp != previousTimestamp))
            {
                /* new Access Unit: do we need to call the auCallback? */
                ARSAL_Mutex_Lock(&(sender->streamMutex));
                if (previousTimestamp != sender->lastAuTimestamp)
                {
                    sender->lastAuTimestamp = previousTimestamp;
                    ARSAL_Mutex_Unlock(&(sender->streamMutex));

                    /* call the auCallback */
                    ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARSTREAM_SENDER2_TAG, "Time %llu: start sending new AU -> auCallback", previousTimestamp);
                    sender->auCallback(ARSTREAM_SENDER2_STATUS_AU_SENT, previousAuUserPtr, sender->custom);
                }
                else
                {
                    ARSAL_Mutex_Unlock(&(sender->streamMutex));
                }
            }

            /* A NALU is ready to send */
            ARSAL_Mutex_Lock(&(sender->streamMutex));
            targetPacketSize = sender->targetPacketSize;
            maxPacketSize = sender->maxPacketSize;
            ARSAL_Mutex_Unlock(&(sender->streamMutex));
            fragmentCount = (nalu.naluSize + targetPacketSize / 2) / targetPacketSize;
            if (fragmentCount < 1) fragmentCount = 1;

            if ((fragmentCount != 1) || (nalu.naluSize > maxPacketSize))
            {
                /* Fragmentation (FU-A) */
                int i;
                uint8_t fuIndicator, fuHeader, startBit, endBit;
                fuIndicator = fuHeader = *nalu.naluBuffer;
                fuIndicator &= ~0x1F;
                fuIndicator |= ARSTREAM_NETWORK_HEADERS2_NALU_TYPE_FUA;
                fuHeader &= ~0xE0;
                meanFragmentSize = (nalu.naluSize + fragmentCount / 2) / fragmentCount;
//ARSAL_PRINT(ARSAL_PRINT_ERROR, ARSTREAM_SENDER2_TAG, "Time %llu: FU-A, naluSize=%d, fragmentCount=%d, meanFragmentSize=%d", nalu.auTimestamp, nalu.naluSize, fragmentCount, meanFragmentSize); //TODO: debug

                for (i = 0, offset = 1; i < fragmentCount; i++)
                {
                    fragmentSize = (i == fragmentCount - 1) ? nalu.naluSize - offset : meanFragmentSize;
                    fragmentOffset = 0;
                    do
                    {
                        packetSize = (fragmentSize - fragmentOffset + 2 > maxPacketSize) ? maxPacketSize : fragmentSize - fragmentOffset;
//ARSAL_PRINT(ARSAL_PRINT_ERROR, ARSTREAM_SENDER2_TAG, "-- Time %llu: FU-A, packetSize=%d", nalu.auTimestamp, packetSize); //TODO: debug

                        memcpy(sendBuffer + sizeof(ARSTREAM_NetworkHeaders_DataHeader2_t) + 2, nalu.naluBuffer + offset, packetSize);
                        sendSize = packetSize + 2 + sizeof(ARSTREAM_NetworkHeaders_DataHeader2_t);
                        *(sendBuffer + sizeof(ARSTREAM_NetworkHeaders_DataHeader2_t)) = fuIndicator;
                        startBit = (offset == 1) ? 0x80 : 0;
                        endBit = ((i == fragmentCount - 1) && (fragmentOffset + packetSize == fragmentSize)) ? 0x40 : 0;
                        *(sendBuffer + sizeof(ARSTREAM_NetworkHeaders_DataHeader2_t) + 1) = fuHeader | startBit | endBit;

//ARSAL_PRINT(ARSAL_PRINT_ERROR, ARSTREAM_SENDER2_TAG, "-- Time %llu: FU-A, sendSize=%d, startBit=%d, endBit=%d, markerBit=%d", nalu.auTimestamp, sendSize, startBit, endBit, ((nalu.isLastInAu) && (endBit)) ? 1 : 0); //TODO: debug
                        ret = ARSTREAM_Sender2_SendData(sender, sendBuffer, sendSize, nalu.auTimestamp, ((nalu.isLastInAu) && (endBit)) ? 1 : 0);
                        if (ret != 0)
                        {
                            ARSAL_PRINT(ARSAL_PRINT_ERROR, ARSTREAM_SENDER2_TAG, "Time %llu: FU-A SendData failed (error %d)", ret); //TODO: debug
                        }

                        fragmentOffset += packetSize;
                        offset += packetSize;
                    }
                    while (fragmentOffset != fragmentSize);
                }
            }
            else
            {
                /* Aggregation (STAP-A) */
                //TODO

                /* Single NAL unit */
                memcpy(sendBuffer + sizeof(ARSTREAM_NetworkHeaders_DataHeader2_t), nalu.naluBuffer, nalu.naluSize);
                sendSize = nalu.naluSize + sizeof(ARSTREAM_NetworkHeaders_DataHeader2_t);
                
                ret = ARSTREAM_Sender2_SendData(sender, sendBuffer, sendSize, nalu.auTimestamp, nalu.isLastInAu);
                if (ret != 0)
                {
                    ARSAL_PRINT(ARSAL_PRINT_ERROR, ARSTREAM_SENDER2_TAG, "Time %llu: singleNALU SendData failed (error %d)", ret); //TODO: debug
                }
            }

            /* last NALU in the Access Unit: call the auCallback */
            if (nalu.isLastInAu)
            {
                ARSAL_Mutex_Lock(&(sender->streamMutex));
                if (nalu.auTimestamp != sender->lastAuTimestamp)
                {
                    sender->lastAuTimestamp = nalu.auTimestamp;
                    ARSAL_Mutex_Unlock(&(sender->streamMutex));

                    /* call the auCallback */
                    ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARSTREAM_SENDER2_TAG, "Time %llu: all packets were sent -> auCallback", nalu.auTimestamp);
                    sender->auCallback(ARSTREAM_SENDER2_STATUS_AU_SENT, nalu.auUserPtr, sender->custom);
                }
                else
                {
                    ARSAL_Mutex_Unlock(&(sender->streamMutex));
                }
            }

            previousTimestamp = nalu.auTimestamp;
            previousAuUserPtr = nalu.auUserPtr;
        }
        
        ARSAL_Mutex_Lock(&(sender->streamMutex));
        shouldStop = sender->threadsShouldStop;
        ARSAL_Mutex_Unlock(&(sender->streamMutex));
        
        if ((fifoRes != 0) && (shouldStop == 0))
        {
            /* Wake up when a new NALU is in the FIFO or when we need to exit */
            ARSAL_Mutex_Lock(&(sender->fifoMutex));
            ARSAL_Cond_Wait(&(sender->fifoCond), &(sender->fifoMutex));
            ARSAL_Mutex_Unlock(&(sender->fifoMutex));
        }
    }

    ARSAL_Mutex_Lock(&(sender->streamMutex));
    sender->dataThreadStarted = 0;
    ARSAL_Mutex_Unlock(&(sender->streamMutex));

    /* cancel the last Access Unit */
    ARSAL_Mutex_Lock(&(sender->streamMutex));
    if (previousTimestamp != sender->lastAuTimestamp)
    {
        sender->lastAuTimestamp = previousTimestamp;
        ARSAL_Mutex_Unlock(&(sender->streamMutex));

        /* call the auCallback */
        ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARSTREAM_SENDER2_TAG, "Time %llu: cancel frame -> auCallback", previousTimestamp);
        sender->auCallback(ARSTREAM_SENDER2_STATUS_AU_CANCELLED, previousAuUserPtr, sender->custom);
    }
    else
    {
        ARSAL_Mutex_Unlock(&(sender->streamMutex));
    }

    /* flush the NALU FIFO */
    ARSTREAM_Sender2_FlushNaluFifo(sender);

    ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARSTREAM_SENDER2_TAG, "Sender thread ended");

    if (sendBuffer)
    {
        free(sendBuffer);
        sendBuffer = NULL;
    }

    return (void*)0;
}


void* ARSTREAM_Sender2_RunAckThread(void *ARSTREAM_Sender2_t_Param)
{
    ARSTREAM_Sender2_t *sender = (ARSTREAM_Sender2_t*)ARSTREAM_Sender2_t_Param;

    ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARSTREAM_SENDER2_TAG, "Ack thread running");
    ARSAL_Mutex_Lock(&(sender->streamMutex));
    sender->ackThreadStarted = 1;
    ARSAL_Mutex_Unlock(&(sender->streamMutex));

    ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARSTREAM_SENDER2_TAG, "Ack thread ended");
    ARSAL_Mutex_Lock(&(sender->streamMutex));
    sender->ackThreadStarted = 0;
    ARSAL_Mutex_Unlock(&(sender->streamMutex));

    return (void*)0;
}


void* ARSTREAM_Sender2_GetCustom(ARSTREAM_Sender2_t *sender)
{
    void *ret = NULL;
    if (sender != NULL)
    {
        ret = sender->custom;
    }
    return ret;
}


int ARSTREAM_Sender2_GetTargetPacketSize(ARSTREAM_Sender2_t *sender)
{
    int ret = -1;
    if (sender != NULL)
    {
        ARSAL_Mutex_Lock(&(sender->streamMutex));
        ret = sender->targetPacketSize + sizeof(ARSTREAM_NetworkHeaders_DataHeader2_t);             //TODO: include all headers size
        ARSAL_Mutex_Unlock(&(sender->streamMutex));
    }
    return ret;
}


eARSTREAM_ERROR ARSTREAM_Sender2_SetTargetPacketSize(ARSTREAM_Sender2_t *sender, int targetPacketSize)
{
    eARSTREAM_ERROR ret = ARSTREAM_OK;
    if ((sender == NULL) || (targetPacketSize == 0))
    {
        return ARSTREAM_ERROR_BAD_PARAMETERS;
    }

    ARSAL_Mutex_Lock(&(sender->streamMutex));
    sender->targetPacketSize = targetPacketSize - sizeof(ARSTREAM_NetworkHeaders_DataHeader2_t);    //TODO: include all headers size
    ARSAL_Mutex_Unlock(&(sender->streamMutex));

    return ret;
}

