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
#include <poll.h>
#include <math.h>

/*
 * Private Headers
 */

#include "ARSTREAM_NetworkHeaders.h"

/*
 * ARSDK Headers
 */

#include <libARStream/ARSTREAM_Sender2.h>
#include <libARSAL/ARSAL_Mutex.h>
#include <libARSAL/ARSAL_Print.h>
#include <libARSAL/ARSAL_Endianness.h>
#include <libARSAL/ARSAL_Socket.h>


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

#define ARSTREAM_SENDER2_MONITORING_OUTPUT
#ifdef ARSTREAM_SENDER2_MONITORING_OUTPUT
    #include <stdio.h>
    #define ARSTREAM_SENDER2_MONITORING_OUTPUT_ALLOW_DRONE
    #define ARSTREAM_SENDER2_MONITORING_OUTPUT_PATH_DRONE "/data/ftp/internal_000/streamdebug"
    #define ARSTREAM_SENDER2_MONITORING_OUTPUT_ALLOW_NAP_USB
    #define ARSTREAM_SENDER2_MONITORING_OUTPUT_PATH_NAP_USB "/tmp/mnt/STREAMDEBUG/streamdebug"
    //#define ARSTREAM_SENDER2_MONITORING_OUTPUT_ALLOW_NAP_INTERNAL
    #define ARSTREAM_SENDER2_MONITORING_OUTPUT_PATH_NAP_INTERNAL "/data/skycontroller/streamdebug"
    //#define ARSTREAM_SENDER2_MONITORING_OUTPUT_ALLOW_ANDROID_INTERNAL
    #define ARSTREAM_SENDER2_MONITORING_OUTPUT_PATH_ANDROID_INTERNAL "/storage/emulated/legacy/FF/streamdebug"
    #define ARSTREAM_SENDER2_MONITORING_OUTPUT_ALLOW_PCLINUX
    #define ARSTREAM_SENDER2_MONITORING_OUTPUT_PATH_PCLINUX "./streamdebug"
#endif


typedef struct ARSTREAM_Sender2_Nalu_s {
    uint8_t *naluBuffer;
    uint32_t naluSize;
    uint64_t auTimestamp;
    uint64_t naluInputTimestamp;
    int isLastInAu;
    void *auUserPtr;
    void *naluUserPtr;
    int drop;

    struct ARSTREAM_Sender2_Nalu_s* prev;
    struct ARSTREAM_Sender2_Nalu_s* next;
    int used;
} ARSTREAM_Sender2_Nalu_t;


#define ARSTREAM_SENDER2_MONITORING_MAX_POINTS (2048)

typedef struct ARSTREAM_Sender2_MonitoringPoint_s {
    uint64_t auTimestamp;
    uint64_t inputTimestamp;
    uint64_t sendTimestamp;
    uint32_t bytesSent;
    uint32_t bytesDropped;
} ARSTREAM_Sender2_MonitoringPoint_t;


struct ARSTREAM_Sender2_t {
    /* Configuration on New */
    char *ifaceAddr;
    char *sendAddr;
    int sendPort;
    ARSTREAM_Sender2_AuCallback_t auCallback;
    ARSTREAM_Sender2_NaluCallback_t naluCallback;
    int maxPacketSize;
    int targetPacketSize;
    int naluFifoSize;
    int maxBitrate;
    int maxLatencyMs;
    int maxNetworkLatencyMs;
    void *custom;

    uint64_t lastAuTimestamp;
    uint64_t firstTimestamp;
    uint16_t seqNum;
    ARSAL_Mutex_t streamMutex;

    /* Thread status */
    int threadsShouldStop;
    int sendThreadStarted;
    int recvThreadStarted;

    /* Sockets */
    int sendMulticast;
    int sendSocketBufferSize;
    struct sockaddr_in sendSin;
    int sendSocket;
    int recvSocket;
    
    /* NAL unit FIFO */
    ARSAL_Mutex_t fifoMutex;
    ARSAL_Cond_t fifoCond;
    int fifoCount;
    int naluFifoBufferSize;
    ARSTREAM_Sender2_Nalu_t *fifoHead;
    ARSTREAM_Sender2_Nalu_t *fifoTail;
    ARSTREAM_Sender2_Nalu_t *fifoPool;

    /* Monitoring */
    ARSAL_Mutex_t monitoringMutex;
    int monitoringCount;
    int monitoringIndex;
    ARSTREAM_Sender2_MonitoringPoint_t monitoringPoint[ARSTREAM_SENDER2_MONITORING_MAX_POINTS];
#ifdef ARSTREAM_SENDER2_MONITORING_OUTPUT
    FILE* fMonitorOut;
#endif
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

    if ((!sender->fifoHead) || (!sender->fifoCount))
    {
        ARSAL_Mutex_Unlock(&sender->fifoMutex);
        return -2;
    }

    cur = sender->fifoHead;
    if (cur->next)
    {
        cur->next->prev = NULL;
        sender->fifoHead = cur->next;
        sender->fifoCount--;
    }
    else
    {
        sender->fifoHead = NULL;
        sender->fifoCount = 0;
        sender->fifoTail = NULL;
    }

    cur->used = 0;
    cur->prev = NULL;
    cur->next = NULL;
    memcpy(nalu, cur, sizeof(ARSTREAM_Sender2_Nalu_t));

    ARSAL_Mutex_Unlock (&(sender->fifoMutex));

    return 0;
}


static int ARSTREAM_Sender2_DropFromFifo(ARSTREAM_Sender2_t *sender, int maxTargetSize)
{
    ARSTREAM_Sender2_Nalu_t* cur = NULL;
    int size = 0, dropSize[4] = {0, 0, 0, 0};
    int nri;

    if (!sender)
    {
        return -1;
    }

    ARSAL_Mutex_Lock(&(sender->fifoMutex));

    if ((!sender->fifoHead) || (!sender->fifoTail) || (!sender->fifoCount))
    {
        ARSAL_Mutex_Unlock(&sender->fifoMutex);
        return 0;
    }

    /* get the current FIFO size in bytes */
    for (cur = sender->fifoHead; cur; cur = cur->next)
    {
        if (!cur->drop)
        {
            size += cur->naluSize;
        }
    }

    ARSAL_Mutex_Unlock(&sender->fifoMutex);

    if (size <= maxTargetSize)
    {
        return 0;
    }

    //ARSAL_PRINT(ARSAL_PRINT_WARNING, ARSTREAM_SENDER2_TAG, "Drop from FIFO - initial size = %d bytes, maxTargetSize = %d", size, maxTargetSize); //TODO: debug

    for (nri = 0; nri < 4; nri++)
    {
        /* stop if the max target size has been reached */
        if (size <= maxTargetSize)
        {
            break;
        }

        ARSAL_Mutex_Lock(&(sender->fifoMutex));

        for (cur = sender->fifoTail; cur; cur = cur->prev)
        {
            /* stop if the max target size has been reached */
            if (size <= maxTargetSize)
            {
                break;
            }

            /* check the NRI bits */
            if ((((uint8_t)(*(cur->naluBuffer)) >> 5) & 0x3) == nri)
            {
                cur->drop = 1;
                size -= cur->naluSize;
                dropSize[nri] += cur->naluSize;
            }
        }

        ARSAL_Mutex_Unlock(&(sender->fifoMutex));
    }

    /*if (dropSize[0])
    {
        ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARSTREAM_SENDER2_TAG, "Drop from FIFO - dropSize[0] = %d bytes", dropSize[0]); //TODO: debug
    }
    if (dropSize[1])
    {
        ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARSTREAM_SENDER2_TAG, "Drop from FIFO - dropSize[1] = %d bytes", dropSize[1]); //TODO: debug
    }
    if (dropSize[2])
    {
        ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARSTREAM_SENDER2_TAG, "Drop from FIFO - dropSize[2] = %d bytes", dropSize[2]); //TODO: debug
    }
    if (dropSize[3])
    {
        ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARSTREAM_SENDER2_TAG, "Drop from FIFO - dropSize[3] = %d bytes", dropSize[3]); //TODO: debug
    }*/

    return dropSize[0] + dropSize[1] + dropSize[2] + dropSize[3];
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
        /* call the naluCallback */
        if (sender->naluCallback != NULL)
        {
            //ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARSTREAM_SENDER2_TAG, "Time %llu: flushing NALU FIFO -> naluCallback", nalu.auTimestamp);
            sender->naluCallback(ARSTREAM_SENDER2_STATUS_CANCELLED, nalu.naluUserPtr, sender->custom);
        }

        /* last NALU in the Access Unit: call the auCallback */
        if ((sender->auCallback != NULL) && (nalu.isLastInAu))
        {
            ARSAL_Mutex_Lock(&(sender->streamMutex));
            if (nalu.auTimestamp != sender->lastAuTimestamp)
            {
                sender->lastAuTimestamp = nalu.auTimestamp;
                ARSAL_Mutex_Unlock(&(sender->streamMutex));

                /* call the auCallback */
                //ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARSTREAM_SENDER2_TAG, "Time %llu: flushing NALU FIFO -> auCallback", nalu.auTimestamp);
                sender->auCallback(ARSTREAM_SENDER2_STATUS_CANCELLED, nalu.auUserPtr, sender->custom);
            }
            else
            {
                ARSAL_Mutex_Unlock(&(sender->streamMutex));
            }
        }
    }

    return 0;
}


ARSTREAM_Sender2_t* ARSTREAM_Sender2_New(ARSTREAM_Sender2_Config_t *config, void *custom, eARSTREAM_ERROR *error)
{
    ARSTREAM_Sender2_t *retSender = NULL;
    int streamMutexWasInit = 0;
    int monitoringMutexWasInit = 0;
    int fifoWasCreated = 0;
    int fifoMutexWasInit = 0;
    int fifoCondWasInit = 0;
    eARSTREAM_ERROR internalError = ARSTREAM_OK;
    /* ARGS Check */
    if ((config == NULL) ||
        (config->sendAddr == NULL) ||
        (strlen(config->sendAddr) <= 0) ||
        (config->sendPort <= 0) ||
        (config->maxPacketSize < 100) ||
        (config->targetPacketSize < 100) ||
        (config->maxBitrate <= 0) ||
        (config->maxLatencyMs < 0) ||
        (config->maxNetworkLatencyMs <= 0) ||
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
        if (config->sendAddr)
        {
            retSender->sendAddr = strndup(config->sendAddr, 16);
        }
        if (config->ifaceAddr)
        {
            retSender->ifaceAddr = strndup(config->ifaceAddr, 16);
        }
        retSender->sendPort = config->sendPort;
        retSender->auCallback = config->auCallback;
        retSender->naluCallback = config->naluCallback;
        retSender->naluFifoSize = config->naluFifoSize;
        retSender->maxPacketSize = config->maxPacketSize - sizeof(ARSTREAM_NetworkHeaders_DataHeader2_t) - ARSTREAM_NETWORK_UDP_HEADER_SIZE - ARSTREAM_NETWORK_IP_HEADER_SIZE;
        retSender->targetPacketSize = config->targetPacketSize - sizeof(ARSTREAM_NetworkHeaders_DataHeader2_t) - ARSTREAM_NETWORK_UDP_HEADER_SIZE - ARSTREAM_NETWORK_IP_HEADER_SIZE;
        retSender->maxBitrate = config->maxBitrate;
        retSender->maxLatencyMs = config->maxLatencyMs;
        retSender->maxNetworkLatencyMs = config->maxNetworkLatencyMs;
        int totalBufSize = config->maxBitrate * config->maxNetworkLatencyMs / 1000 / 8;
        retSender->sendSocketBufferSize = totalBufSize / 2; //TODO: tuning
        retSender->naluFifoBufferSize = totalBufSize / 2; //TODO: tuning
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
    if (internalError == ARSTREAM_OK)
    {
        int mutexInitRet = ARSAL_Mutex_Init(&(retSender->monitoringMutex));
        if (mutexInitRet != 0)
        {
            internalError = ARSTREAM_ERROR_ALLOC;
        }
        else
        {
            monitoringMutexWasInit = 1;
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

#ifdef ARSTREAM_SENDER2_MONITORING_OUTPUT
    if (internalError == ARSTREAM_OK)
    {
        int i;
        char szOutputFileName[128];
        char *pszFilePath = NULL;
        szOutputFileName[0] = '\0';
        if (0)
        {
        }
#ifdef ARSTREAM_SENDER2_MONITORING_OUTPUT_ALLOW_DRONE
        else if ((access(ARSTREAM_SENDER2_MONITORING_OUTPUT_PATH_DRONE, F_OK) == 0) && (access(ARSTREAM_SENDER2_MONITORING_OUTPUT_PATH_DRONE, W_OK) == 0))
        {
            pszFilePath = ARSTREAM_SENDER2_MONITORING_OUTPUT_PATH_DRONE;
        }
#endif
#ifdef ARSTREAM_SENDER2_MONITORING_OUTPUT_ALLOW_NAP_USB
        else if ((access(ARSTREAM_SENDER2_MONITORING_OUTPUT_PATH_NAP_USB, F_OK) == 0) && (access(ARSTREAM_SENDER2_MONITORING_OUTPUT_PATH_NAP_USB, W_OK) == 0))
        {
            pszFilePath = ARSTREAM_SENDER2_MONITORING_OUTPUT_PATH_NAP_USB;
        }
#endif
#ifdef ARSTREAM_SENDER2_MONITORING_OUTPUT_ALLOW_NAP_INTERNAL
        else if ((access(ARSTREAM_SENDER2_MONITORING_OUTPUT_PATH_NAP_INTERNAL, F_OK) == 0) && (access(ARSTREAM_SENDER2_MONITORING_OUTPUT_PATH_NAP_INTERNAL, W_OK) == 0))
        {
            pszFilePath = ARSTREAM_SENDER2_MONITORING_OUTPUT_PATH_NAP_INTERNAL;
        }
#endif
#ifdef ARSTREAM_SENDER2_MONITORING_OUTPUT_ALLOW_ANDROID_INTERNAL
        else if ((access(ARSTREAM_SENDER2_MONITORING_OUTPUT_PATH_ANDROID_INTERNAL, F_OK) == 0) && (access(ARSTREAM_SENDER2_MONITORING_OUTPUT_PATH_ANDROID_INTERNAL, W_OK) == 0))
        {
            pszFilePath = ARSTREAM_SENDER2_MONITORING_OUTPUT_PATH_ANDROID_INTERNAL;
        }
#endif
#ifdef ARSTREAM_SENDER2_MONITORING_OUTPUT_ALLOW_PCLINUX
        else if ((access(ARSTREAM_SENDER2_MONITORING_OUTPUT_PATH_PCLINUX, F_OK) == 0) && (access(ARSTREAM_SENDER2_MONITORING_OUTPUT_PATH_PCLINUX, W_OK) == 0))
        {
            pszFilePath = ARSTREAM_SENDER2_MONITORING_OUTPUT_PATH_PCLINUX;
        }
#endif
        if (pszFilePath)
        {
            for (i = 0; i < 1000; i++)
            {
                snprintf(szOutputFileName, 128, "%s/sender_monitor_%03d.dat", pszFilePath, i);
                if (access(szOutputFileName, F_OK) == -1)
                {
                    // file does not exist
                    break;
                }
                szOutputFileName[0] = '\0';
            }
        }

        if (strlen(szOutputFileName))
        {
            retSender->fMonitorOut = fopen(szOutputFileName, "w");
            if (!retSender->fMonitorOut)
            {
                ARSAL_PRINT(ARSAL_PRINT_WARNING, ARSTREAM_SENDER2_TAG, "Unable to open monitor output file '%s'", szOutputFileName);
            }
        }

        if (retSender->fMonitorOut)
        {
            fprintf(retSender->fMonitorOut, "sendTimestamp inputTimestamp auTimestamp rtpTimestamp rtpSeqNum rtpMarkerBit bytesSent bytesDropped\n");
        }
    }
#endif //#ifdef ARSTREAM_SENDER2_MONITORING_OUTPUT

    if ((internalError != ARSTREAM_OK) &&
        (retSender != NULL))
    {
        if (streamMutexWasInit == 1)
        {
            ARSAL_Mutex_Destroy(&(retSender->streamMutex));
        }
        if (monitoringMutexWasInit == 1)
        {
            ARSAL_Mutex_Destroy(&(retSender->monitoringMutex));
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
        if (((*sender)->sendThreadStarted == 0) &&
            ((*sender)->recvThreadStarted == 0))
        {
            canDelete = 1;
        }
        ARSAL_Mutex_Unlock(&((*sender)->streamMutex));

        if (canDelete == 1)
        {
            ARSAL_Mutex_Destroy(&((*sender)->streamMutex));
            ARSAL_Mutex_Destroy(&((*sender)->monitoringMutex));
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
#ifdef ARSTREAM_SENDER2_MONITORING_OUTPUT
            if ((*sender)->fMonitorOut)
            {
                fclose((*sender)->fMonitorOut);
            }
#endif
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


eARSTREAM_ERROR ARSTREAM_Sender2_SendNewNalu(ARSTREAM_Sender2_t *sender, uint8_t *naluBuffer, uint32_t naluSize, uint64_t auTimestamp, int isLastNaluInAu, void *auUserPtr, void *naluUserPtr)
{
    eARSTREAM_ERROR retVal = ARSTREAM_OK;
    struct timespec t1;
    ARSAL_Time_GetTime(&t1);

    // Args check
    if ((sender == NULL) ||
        (naluBuffer == NULL) ||
        (naluSize == 0) ||
        (auTimestamp == 0))
    {
        retVal = ARSTREAM_ERROR_BAD_PARAMETERS;
    }

    ARSAL_Mutex_Lock(&(sender->streamMutex));
    if (!sender->sendThreadStarted)
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
        nalu.naluInputTimestamp = (uint64_t)t1.tv_sec * 1000000 + (uint64_t)t1.tv_nsec / 1000;
        nalu.isLastInAu = isLastNaluInAu;
        nalu.auUserPtr = auUserPtr;
        nalu.naluUserPtr = naluUserPtr;

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


static void ARSTREAM_Sender2_UpdateMonitoring(ARSTREAM_Sender2_t *sender, uint64_t inputTimestamp, uint64_t auTimestamp, uint32_t rtpTimestamp, uint16_t seqNum, uint16_t markerBit, uint32_t bytesSent, uint32_t bytesDropped)
{
    uint64_t curTime;
    struct timespec t1;
    ARSAL_Time_GetTime(&t1);
    curTime = (uint64_t)t1.tv_sec * 1000000 + (uint64_t)t1.tv_nsec / 1000;

    ARSAL_Mutex_Lock(&(sender->monitoringMutex));

    if (sender->monitoringCount < ARSTREAM_SENDER2_MONITORING_MAX_POINTS)
    {
        sender->monitoringCount++;
    }
    sender->monitoringIndex = (sender->monitoringIndex + 1) % ARSTREAM_SENDER2_MONITORING_MAX_POINTS;
    sender->monitoringPoint[sender->monitoringIndex].auTimestamp = auTimestamp;
    sender->monitoringPoint[sender->monitoringIndex].inputTimestamp = inputTimestamp;
    sender->monitoringPoint[sender->monitoringIndex].sendTimestamp = curTime;
    sender->monitoringPoint[sender->monitoringIndex].bytesSent = bytesSent;
    sender->monitoringPoint[sender->monitoringIndex].bytesDropped = bytesDropped;

    ARSAL_Mutex_Unlock(&(sender->monitoringMutex));

#ifdef ARSTREAM_SENDER2_MONITORING_OUTPUT
    if (sender->fMonitorOut)
    {
        fprintf(sender->fMonitorOut, "%llu ", curTime);
        fprintf(sender->fMonitorOut, "%llu ", inputTimestamp);
        fprintf(sender->fMonitorOut, "%llu ", auTimestamp);
        fprintf(sender->fMonitorOut, "%lu %u %u %lu %lu\n", rtpTimestamp, seqNum, markerBit, bytesSent, bytesDropped);
    }
#endif
}


static int ARSTREAM_Sender2_SetSocketSendBufferSize(ARSTREAM_Sender2_t *sender, int size)
{
    int ret = 0, err;
    int size2 = sizeof(int);

    size /= 2;
    err = ARSAL_Socket_Setsockopt(sender->sendSocket, SOL_SOCKET, SO_SNDBUF, (void*)&size, sizeof(size));
    if (err != 0)
    {
        ret = -1;
        ARSAL_PRINT(ARSAL_PRINT_ERROR, ARSTREAM_SENDER2_TAG, "Failed to set send socket buffer size to 2*%d bytes: error=%d (%s)", size, errno, strerror(errno));
    }

    size = -1;
    err = ARSAL_Socket_Getsockopt(sender->sendSocket, SOL_SOCKET, SO_SNDBUF, (void*)&size, &size2);
    if (err != 0)
    {
        ret = -1;
        ARSAL_PRINT(ARSAL_PRINT_ERROR, ARSTREAM_SENDER2_TAG, "Failed to get send socket buffer size: error=%d (%s)", errno, strerror(errno));
    }
    else
    {
        ARSAL_PRINT(ARSAL_PRINT_WARNING, ARSTREAM_SENDER2_TAG, "Send socket buffer size is %d bytes", size); //TODO: debug
    }

    return ret;
}


static int ARSTREAM_Sender2_Connect(ARSTREAM_Sender2_t *sender)
{
    int ret = 0;

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

    if (ret == 0)
    {
        /* set the socket buffer size and NALU FIFO buffer size */
        err = ARSTREAM_Sender2_SetSocketSendBufferSize(sender, sender->sendSocketBufferSize);
        if (err != 0)
        {
            ARSAL_PRINT(ARSAL_PRINT_ERROR, ARSTREAM_SENDER2_TAG, "Failed to set the send socket buffer size");
            ret = -1;
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

    return ret;
}


static int ARSTREAM_Sender2_SendData(ARSTREAM_Sender2_t *sender, uint8_t *sendBuffer, uint32_t sendSize, uint64_t inputTimestamp, uint64_t auTimestamp, int isLastInAu, int maxLatencyUs, int maxNetworkLatencyUs)
{
    int ret = 0, totalLatencyDrop = 0, networkLatencyDrop = 0;
    ARSTREAM_NetworkHeaders_DataHeader2_t *header = (ARSTREAM_NetworkHeaders_DataHeader2_t *)sendBuffer;
    uint16_t flags;
    uint32_t rtpTimestamp;

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
    rtpTimestamp = (uint32_t)(((((auTimestamp - sender->firstTimestamp) * 90) + 500) / 1000) & 0xFFFFFFFF); /* microseconds to 90000 Hz clock */
    header->timestamp = htonl(rtpTimestamp);
    header->ssrc = htonl(ARSTREAM_NETWORK_HEADERS2_RTP_SSRC);

    /* send to the network layer */
    ssize_t bytes;
    struct timespec t1;
    ARSAL_Time_GetTime(&t1);
    uint64_t curTime = (uint64_t)t1.tv_sec * 1000000 + (uint64_t)t1.tv_nsec / 1000;
    if ((maxLatencyUs > 0) && (curTime - auTimestamp > maxLatencyUs))
    {
        totalLatencyDrop = 1;
    }
    else if (curTime - inputTimestamp > maxNetworkLatencyUs)
    {
        networkLatencyDrop = 1;
    }

    if ((!totalLatencyDrop) && (!networkLatencyDrop))
    {
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
            /* socket send failed */
            switch (errno)
            {
            case EAGAIN:
                /* the socket buffer is full - drop some NALUs */
                ret = ARSTREAM_Sender2_DropFromFifo(sender, sender->naluFifoBufferSize);
                if (ret < 0)
                {
                    ARSAL_PRINT(ARSAL_PRINT_ERROR, ARSTREAM_SENDER2_TAG, "Failed to drop NALUs from FIFO (%d)", ret);
                }
                else if (ret > 0)
                {
                    ARSAL_PRINT(ARSAL_PRINT_WARNING, ARSTREAM_SENDER2_TAG, "Socket buffer full - dropped %d bytes -> polling", ret);
                }
                ret = 0;

                struct pollfd p;
                p.fd = sender->sendSocket;
                p.events = POLLOUT;
                p.revents = 0;
                int pollTimeMs = (maxNetworkLatencyUs - (int)(curTime - inputTimestamp)) / 1000;
                int pollRet = poll(&p, 1, pollTimeMs);
                if (pollRet == 0)
                {
                    /* failed: poll timeout */
                    ARSTREAM_Sender2_UpdateMonitoring(sender, inputTimestamp, auTimestamp, rtpTimestamp, sender->seqNum - 1, isLastInAu, 0, sendSize);
                    ARSAL_PRINT(ARSAL_PRINT_WARNING, ARSTREAM_SENDER2_TAG, "Time %llu: dropped packet (polling timed out: timeout = %dms) (seqNum = %d)",
                                auTimestamp, pollTimeMs, header->seqNum); //TODO: debug
                    ret = -2;
                }
                else if (pollRet < 0)
                {
                    /* failed: poll error */
                    ARSTREAM_Sender2_UpdateMonitoring(sender, inputTimestamp, auTimestamp, rtpTimestamp, sender->seqNum - 1, isLastInAu, 0, sendSize);
                    ARSAL_PRINT(ARSAL_PRINT_ERROR, ARSTREAM_SENDER2_TAG, "Poll error: error=%d (%s)", errno, strerror(errno));
                    ret = -1;
                }
                else if (p.revents & POLLOUT)
                {
                    ARSAL_Time_GetTime(&t1);
                    curTime = (uint64_t)t1.tv_sec * 1000000 + (uint64_t)t1.tv_nsec / 1000;
                    if ((maxLatencyUs > 0) && (curTime - auTimestamp > maxLatencyUs))
                    {
                        totalLatencyDrop = 1;
                    }
                    else if (curTime - inputTimestamp > maxNetworkLatencyUs)
                    {
                        networkLatencyDrop = 1;
                    }

                    if ((!totalLatencyDrop) && (!networkLatencyDrop))
                    {
                        if (sender->sendMulticast)
                        {
                            bytes = ARSAL_Socket_Sendto(sender->sendSocket, sendBuffer, sendSize, 0, (struct sockaddr*)&sender->sendSin, sizeof(sender->sendSin));
                        }
                        else
                        {
                            bytes = ARSAL_Socket_Send(sender->sendSocket, sendBuffer, sendSize, 0);
                        }
                        if (bytes > -1)
                        {
                            /* socket send successful */
                            ARSTREAM_Sender2_UpdateMonitoring(sender, inputTimestamp, auTimestamp, rtpTimestamp, sender->seqNum - 1, isLastInAu, (uint32_t)bytes, 0);
                            ret = 0;
                        }
                        else if (errno == EAGAIN)
                        {
                            /* failed: socket buffer full */
                            ARSTREAM_Sender2_UpdateMonitoring(sender, inputTimestamp, auTimestamp, rtpTimestamp, sender->seqNum - 1, isLastInAu, 0, sendSize);
                            ARSAL_PRINT(ARSAL_PRINT_WARNING, ARSTREAM_SENDER2_TAG, "Time %llu: dropped packet (socket buffer full #2) (seqNum = %d)",
                                        auTimestamp, header->seqNum); //TODO: debug
                            ret = -2;
                        }
                        else
                        {
                            /* failed: socket error */
                            ARSTREAM_Sender2_UpdateMonitoring(sender, inputTimestamp, auTimestamp, rtpTimestamp, sender->seqNum - 1, isLastInAu, 0, sendSize);
                            ARSAL_PRINT(ARSAL_PRINT_ERROR, ARSTREAM_SENDER2_TAG, "Socket send error #2 error=%d (%s)", errno, strerror(errno));
                            ret = -1;
                        }
                    }
                    else
                    {
                        /* packet dropped: too late */
                        ARSTREAM_Sender2_UpdateMonitoring(sender, inputTimestamp, auTimestamp, rtpTimestamp, sender->seqNum - 1, isLastInAu, 0, sendSize);
                        if (totalLatencyDrop)
                        {
                            ARSAL_PRINT(ARSAL_PRINT_WARNING, ARSTREAM_SENDER2_TAG, "Time %llu: dropped packet (after poll total latency %.1fms > %.1fms) (seqNum = %d)",
                                        auTimestamp, (float)(curTime - auTimestamp) / 1000., (float)maxLatencyUs / 1000., header->seqNum); //TODO: debug
                        }
                        if (networkLatencyDrop)
                        {
                            ARSAL_PRINT(ARSAL_PRINT_WARNING, ARSTREAM_SENDER2_TAG, "Time %llu: dropped packet (after poll network latency %.1fms > %.1fms) (seqNum = %d)",
                                        auTimestamp, (float)(curTime - inputTimestamp) / 1000., (float)maxNetworkLatencyUs / 1000., header->seqNum); //TODO: debug
                        }
                    }
                }
                break;
            default:
                /* failed: socket error */
                ARSTREAM_Sender2_UpdateMonitoring(sender, inputTimestamp, auTimestamp, rtpTimestamp, sender->seqNum - 1, isLastInAu, 0, sendSize);
                ARSAL_PRINT(ARSAL_PRINT_ERROR, ARSTREAM_SENDER2_TAG, "Socket send error: error=%d (%s)", errno, strerror(errno));
                ret = -1;
                break;
            }
        }
        else
        {
            /* socket send successful */
            ARSTREAM_Sender2_UpdateMonitoring(sender, inputTimestamp, auTimestamp, rtpTimestamp, sender->seqNum - 1, isLastInAu, (uint32_t)bytes, 0);
        }
    }
    else
    {
        /* packet dropped: too late */
        ARSTREAM_Sender2_UpdateMonitoring(sender, inputTimestamp, auTimestamp, rtpTimestamp, sender->seqNum - 1, isLastInAu, 0, sendSize);
        if (totalLatencyDrop)
        {
            ARSAL_PRINT(ARSAL_PRINT_WARNING, ARSTREAM_SENDER2_TAG, "Time %llu: dropped packet (total latency %.1fms > %.1fms) (seqNum = %d)",
                        auTimestamp, (float)(curTime - auTimestamp) / 1000., (float)maxLatencyUs / 1000., header->seqNum); //TODO: debug
        }
        if (networkLatencyDrop)
        {
            ARSAL_PRINT(ARSAL_PRINT_WARNING, ARSTREAM_SENDER2_TAG, "Time %llu: dropped packet (network latency %.1fms > %.1fms) (seqNum = %d)",
                        auTimestamp, (float)(curTime - inputTimestamp) / 1000., (float)maxNetworkLatencyUs / 1000., header->seqNum); //TODO: debug
        }
    }

    return ret;
}


void* ARSTREAM_Sender2_RunSendThread (void *ARSTREAM_Sender2_t_Param)
{
    /* Local declarations */
    ARSTREAM_Sender2_t *sender = (ARSTREAM_Sender2_t*)ARSTREAM_Sender2_t_Param;
    uint8_t *sendBuffer = NULL;
    uint32_t sendSize = 0;
    ARSTREAM_Sender2_Nalu_t nalu;
    uint64_t previousTimestamp = 0, stapAuTimestamp = 0, stapFirstNaluInputTimestamp = 0;
    void *previousAuUserPtr = NULL;
    int shouldStop;
    int targetPacketSize, maxPacketSize, packetSize, fragmentSize, meanFragmentSize, offset, fragmentOffset, fragmentCount;
    int ret, totalLatencyDrop, networkLatencyDrop;
    int stapPending = 0, stapNaluCount = 0, maxLatencyUs, maxNetworkLatencyUs;
    uint8_t stapMaxNri = 0;
    uint64_t curTime;
    struct timespec t1;

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

    ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARSTREAM_SENDER2_TAG, "Stream sender sending thread running");
    ARSAL_Mutex_Lock(&(sender->streamMutex));
    sender->sendThreadStarted = 1;
    shouldStop = sender->threadsShouldStop;
    ARSAL_Mutex_Unlock(&(sender->streamMutex));

    while (shouldStop == 0)
    {
        int fifoRes = ARSTREAM_Sender2_DequeueNalu(sender, &nalu);

        if (fifoRes == 0)
        {
            ARSAL_Time_GetTime(&t1);
            curTime = (uint64_t)t1.tv_sec * 1000000 + (uint64_t)t1.tv_nsec / 1000;

            ARSAL_Mutex_Lock(&(sender->streamMutex));
            targetPacketSize = sender->targetPacketSize;
            maxPacketSize = sender->maxPacketSize;
            maxLatencyUs = (sender->maxLatencyMs) ? sender->maxLatencyMs * 1000 - (int)((uint64_t)sender->sendSocketBufferSize * 8 * 1000000 / sender->maxBitrate) : 0;
            maxNetworkLatencyUs = sender->maxNetworkLatencyMs * 1000 - (int)((uint64_t)sender->sendSocketBufferSize * 8 * 1000000 / sender->maxBitrate);
            ARSAL_Mutex_Unlock(&(sender->streamMutex));

            if ((previousTimestamp != 0) && (nalu.auTimestamp != previousTimestamp))
            {
                if (stapPending)
                {
                    /* Finish the previous STAP-A packet */
//ARSAL_PRINT(ARSAL_PRINT_WARNING, ARSTREAM_SENDER2_TAG, "Time %llu: STAP-A (previous) sendSize=%d, naluCount=%d", nalu.auTimestamp, sendSize, stapNaluCount); //TODO: debug
                    uint8_t stapHeader = ARSTREAM_NETWORK_HEADERS2_NALU_TYPE_STAPA | ((stapMaxNri & 3) << 5);
                    *(sendBuffer + sizeof(ARSTREAM_NetworkHeaders_DataHeader2_t)) = stapHeader;
                    ret = ARSTREAM_Sender2_SendData(sender, sendBuffer, sendSize, stapFirstNaluInputTimestamp, stapAuTimestamp, 0, maxLatencyUs, maxNetworkLatencyUs); // do not set the marker bit
                    if (ret != 0)
                    {
                        //ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARSTREAM_SENDER2_TAG, "Time %llu: STAP-A SendData failed (error %d)", nalu.auTimestamp, ret);
                    }
                    stapPending = 0;
                    sendSize = 0;
                }

                if (sender->auCallback != NULL)
                {
                    /* new Access Unit: do we need to call the auCallback? */
                    ARSAL_Mutex_Lock(&(sender->streamMutex));
                    if (previousTimestamp != sender->lastAuTimestamp)
                    {
                        sender->lastAuTimestamp = previousTimestamp;
                        ARSAL_Mutex_Unlock(&(sender->streamMutex));

                        /* call the auCallback */
                        //ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARSTREAM_SENDER2_TAG, "Time %llu: start sending new AU -> auCallback", previousTimestamp);
                        sender->auCallback(ARSTREAM_SENDER2_STATUS_SENT, previousAuUserPtr, sender->custom);
                    }
                    else
                    {
                        ARSAL_Mutex_Unlock(&(sender->streamMutex));
                    }
                }
            }

            /* check that the NALU is not older than the max latency */
            totalLatencyDrop = networkLatencyDrop = 0;
            if ((maxLatencyUs > 0) && (curTime - nalu.auTimestamp > maxLatencyUs))
            {
                totalLatencyDrop = 1;
            }
            else if (curTime - nalu.naluInputTimestamp > maxNetworkLatencyUs)
            {
                networkLatencyDrop = 1;
            }

            /* check that the NALU has not been flagged for dropping */
            if ((!nalu.drop) && (!totalLatencyDrop) && (!networkLatencyDrop))
            {
                /* A NALU is ready to send */
                fragmentCount = (nalu.naluSize + targetPacketSize / 2) / targetPacketSize;
                if (fragmentCount < 1) fragmentCount = 1;

                if ((fragmentCount > 1) || (nalu.naluSize > maxPacketSize))
                {
                    /* Fragmentation (FU-A) */

                    if (stapPending)
                    {
                        /* Finish the previous STAP-A packet */
//ARSAL_PRINT(ARSAL_PRINT_WARNING, ARSTREAM_SENDER2_TAG, "Time %llu: STAP-A (previous) sendSize=%d, naluCount=%d", nalu.auTimestamp, sendSize, stapNaluCount); //TODO: debug
                        uint8_t stapHeader = ARSTREAM_NETWORK_HEADERS2_NALU_TYPE_STAPA | ((stapMaxNri & 3) << 5);
                        *(sendBuffer + sizeof(ARSTREAM_NetworkHeaders_DataHeader2_t)) = stapHeader;
                        ret = ARSTREAM_Sender2_SendData(sender, sendBuffer, sendSize, stapFirstNaluInputTimestamp, stapAuTimestamp, 0, maxLatencyUs, maxNetworkLatencyUs); // do not set the marker bit
                        if (ret != 0)
                        {
                            //ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARSTREAM_SENDER2_TAG, "Time %llu: STAP-A SendData failed (error %d)", nalu.auTimestamp, ret);
                        }
                        stapPending = 0;
                        sendSize = 0;
                    }

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
                            packetSize = (fragmentSize - fragmentOffset > maxPacketSize - 2) ? maxPacketSize - 2 : fragmentSize - fragmentOffset;
//ARSAL_PRINT(ARSAL_PRINT_ERROR, ARSTREAM_SENDER2_TAG, "-- Time %llu: FU-A, packetSize=%d", nalu.auTimestamp, packetSize); //TODO: debug

                            if (packetSize + 2 <= maxPacketSize)
                            {
                                memcpy(sendBuffer + sizeof(ARSTREAM_NetworkHeaders_DataHeader2_t) + 2, nalu.naluBuffer + offset, packetSize);
                                sendSize = packetSize + 2 + sizeof(ARSTREAM_NetworkHeaders_DataHeader2_t);
                                *(sendBuffer + sizeof(ARSTREAM_NetworkHeaders_DataHeader2_t)) = fuIndicator;
                                startBit = (offset == 1) ? 0x80 : 0;
                                endBit = ((i == fragmentCount - 1) && (fragmentOffset + packetSize == fragmentSize)) ? 0x40 : 0;
                                *(sendBuffer + sizeof(ARSTREAM_NetworkHeaders_DataHeader2_t) + 1) = fuHeader | startBit | endBit;

//ARSAL_PRINT(ARSAL_PRINT_WARNING, ARSTREAM_SENDER2_TAG, "Time %llu: FU-A, sendSize=%d, startBit=%d, endBit=%d, markerBit=%d", nalu.auTimestamp, sendSize, startBit, endBit, ((nalu.isLastInAu) && (endBit)) ? 1 : 0); //TODO: debug
                                ret = ARSTREAM_Sender2_SendData(sender, sendBuffer, sendSize, nalu.naluInputTimestamp, nalu.auTimestamp, ((nalu.isLastInAu) && (endBit)) ? 1 : 0, maxLatencyUs, maxNetworkLatencyUs);
                                if (ret != 0)
                                {
                                    //ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARSTREAM_SENDER2_TAG, "Time %llu: FU-A SendData failed (error %d)", nalu.auTimestamp, ret);
                                }
                                sendSize = 0;
                            }
                            else
                            {
                                ARSAL_PRINT(ARSAL_PRINT_ERROR, ARSTREAM_SENDER2_TAG, "-- Time %llu: FU-A, packetSize + 2 > maxPacketSize (packetSize=%d)", nalu.auTimestamp, packetSize); //TODO: debug
                            }

                            fragmentOffset += packetSize;
                            offset += packetSize;
                        }
                        while (fragmentOffset != fragmentSize);
                    }
                }
                else
                {
                    int stapSize = nalu.naluSize + 2 + ((!stapPending) ? sizeof(ARSTREAM_NetworkHeaders_DataHeader2_t) + 1 : 0);
                    if ((sendSize + stapSize >= maxPacketSize) || (sendSize + stapSize > targetPacketSize))
                    {
                        if (stapPending)
                        {
                            /* Finish the previous STAP-A packet */
//ARSAL_PRINT(ARSAL_PRINT_WARNING, ARSTREAM_SENDER2_TAG, "Time %llu: STAP-A (previous) sendSize=%d, naluCount=%d", nalu.auTimestamp, sendSize, stapNaluCount); //TODO: debug
                            uint8_t stapHeader = ARSTREAM_NETWORK_HEADERS2_NALU_TYPE_STAPA | ((stapMaxNri & 3) << 5);
                            *(sendBuffer + sizeof(ARSTREAM_NetworkHeaders_DataHeader2_t)) = stapHeader;
                            ret = ARSTREAM_Sender2_SendData(sender, sendBuffer, sendSize, stapFirstNaluInputTimestamp, stapAuTimestamp, 0, maxLatencyUs, maxNetworkLatencyUs); // do not set the marker bit
                            if (ret != 0)
                            {
                                //ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARSTREAM_SENDER2_TAG, "Time %llu: STAP-A SendData failed (error %d)", nalu.auTimestamp, ret);
                            }
                            stapPending = 0;
                            sendSize = 0;

                            stapSize = nalu.naluSize + 2 + ((!stapPending) ? sizeof(ARSTREAM_NetworkHeaders_DataHeader2_t) + 1 : 0);
                        }
                    }

                    if ((sendSize + stapSize >= maxPacketSize) || (sendSize + stapSize > targetPacketSize))
                    {
                        /* Single NAL unit */
                        memcpy(sendBuffer + sizeof(ARSTREAM_NetworkHeaders_DataHeader2_t), nalu.naluBuffer, nalu.naluSize);
                        sendSize = nalu.naluSize + sizeof(ARSTREAM_NetworkHeaders_DataHeader2_t);
                        
//ARSAL_PRINT(ARSAL_PRINT_WARNING, ARSTREAM_SENDER2_TAG, "Time %llu: SingleNALU sendSize=%d", nalu.auTimestamp, sendSize); //TODO: debug
                        ret = ARSTREAM_Sender2_SendData(sender, sendBuffer, sendSize, nalu.naluInputTimestamp, nalu.auTimestamp, nalu.isLastInAu, maxLatencyUs, maxNetworkLatencyUs);
                        if (ret != 0)
                        {
                            //ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARSTREAM_SENDER2_TAG, "Time %llu: singleNALU SendData failed (error %d)", nalu.auTimestamp, ret);
                        }
                        sendSize = 0;
                    }
                    else
                    {
                        /* Aggregation (STAP-A) */
                        if (!stapPending)
                        {
                            stapPending = 1;
                            stapMaxNri = 0;
                            stapNaluCount = 0;
                            sendSize = sizeof(ARSTREAM_NetworkHeaders_DataHeader2_t) + 1;
                            stapAuTimestamp = nalu.auTimestamp;
                            stapFirstNaluInputTimestamp = nalu.naluInputTimestamp;
                        }
                        uint8_t nri = ((uint8_t)(*(nalu.naluBuffer)) >> 5) & 0x3;
                        if (nri > stapMaxNri)
                        {
                            stapMaxNri = nri;
                        }
                        *(sendBuffer + sendSize) = ((nalu.naluSize >> 8) & 0xFF);
                        *(sendBuffer + sendSize + 1) = (nalu.naluSize & 0xFF);
                        sendSize += 2;
                        memcpy(sendBuffer + sendSize, nalu.naluBuffer, nalu.naluSize);
                        sendSize += nalu.naluSize;
                        stapNaluCount++;
                        if (nalu.isLastInAu)
                        {
                            /* Finish the STAP-A packet */
//ARSAL_PRINT(ARSAL_PRINT_WARNING, ARSTREAM_SENDER2_TAG, "Time %llu: STAP-A (current) sendSize=%d, naluCount=%d", nalu.auTimestamp, sendSize, stapNaluCount); //TODO: debug
                            uint8_t stapHeader = ARSTREAM_NETWORK_HEADERS2_NALU_TYPE_STAPA | ((stapMaxNri & 3) << 5);
                            *(sendBuffer + sizeof(ARSTREAM_NetworkHeaders_DataHeader2_t)) = stapHeader;
                            ret = ARSTREAM_Sender2_SendData(sender, sendBuffer, sendSize, stapFirstNaluInputTimestamp, stapAuTimestamp, 1, maxLatencyUs, maxNetworkLatencyUs); // set the marker bit
                            if (ret != 0)
                            {
                                //ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARSTREAM_SENDER2_TAG, "Time %llu: STAP-A SendData failed (error %d)", nalu.auTimestamp, ret);
                            }
                            stapPending = 0;
                            sendSize = 0;
                        }
                    }
                }

                /* call the naluCallback */
                if (sender->naluCallback != NULL)
                {
                    //ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARSTREAM_SENDER2_TAG, "Time %llu: sent NALU -> naluCallback", nalu.auTimestamp);
                    sender->naluCallback(ARSTREAM_SENDER2_STATUS_SENT, nalu.naluUserPtr, sender->custom);
                }
            }
            else
            {
                uint32_t rtpTimestamp = (uint32_t)(((((nalu.auTimestamp - sender->firstTimestamp) * 90) + 500) / 1000) & 0xFFFFFFFF);
                ARSTREAM_Sender2_UpdateMonitoring(sender, nalu.naluInputTimestamp, nalu.auTimestamp, rtpTimestamp, sender->seqNum, nalu.isLastInAu, 0, nalu.naluSize);
                if (nalu.drop)
                {
                    ARSAL_PRINT(ARSAL_PRINT_WARNING, ARSTREAM_SENDER2_TAG, "Time %llu: dropped NALU in FIFO (seqNum = %d)", nalu.auTimestamp, sender->seqNum); //TODO: debug
                }
                else if (totalLatencyDrop)
                {
                    ARSAL_PRINT(ARSAL_PRINT_WARNING, ARSTREAM_SENDER2_TAG, "Time %llu: dropped NALU (total latency %.1fms > %.1fms) (seqNum = %d)",
                                nalu.auTimestamp, (float)(curTime - nalu.auTimestamp) / 1000., (float)maxLatencyUs / 1000., sender->seqNum); //TODO: debug
                }
                if (networkLatencyDrop)
                {
                    ARSAL_PRINT(ARSAL_PRINT_WARNING, ARSTREAM_SENDER2_TAG, "Time %llu: dropped NALU (network latency %.1fms > %.1fms) (seqNum = %d)",
                                nalu.auTimestamp, (float)(curTime - nalu.naluInputTimestamp) / 1000., (float)maxNetworkLatencyUs / 1000., sender->seqNum); //TODO: debug
                }
                /* increment the sequence number to let the receiver know that we dropped something */
                sender->seqNum++;

                /* call the naluCallback */
                if (sender->naluCallback != NULL)
                {
                    //ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARSTREAM_SENDER2_TAG, "Time %llu: dropped NALU -> naluCallback", nalu.auTimestamp);
                    sender->naluCallback(ARSTREAM_SENDER2_STATUS_CANCELLED, nalu.naluUserPtr, sender->custom);
                }
            }

            /* last NALU in the Access Unit: call the auCallback */
            if ((sender->auCallback != NULL) && (nalu.isLastInAu))
            {
                ARSAL_Mutex_Lock(&(sender->streamMutex));
                if (nalu.auTimestamp != sender->lastAuTimestamp)
                {
                    sender->lastAuTimestamp = nalu.auTimestamp;
                    ARSAL_Mutex_Unlock(&(sender->streamMutex));

                    /* call the auCallback */
                    //ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARSTREAM_SENDER2_TAG, "Time %llu: all packets were sent -> auCallback", nalu.auTimestamp);
                    sender->auCallback(ARSTREAM_SENDER2_STATUS_SENT, nalu.auUserPtr, sender->custom);
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
    sender->sendThreadStarted = 0;
    ARSAL_Mutex_Unlock(&(sender->streamMutex));

    /* cancel the last Access Unit */
    if (sender->auCallback != NULL)
    {
        ARSAL_Mutex_Lock(&(sender->streamMutex));
        if (previousTimestamp != sender->lastAuTimestamp)
        {
            sender->lastAuTimestamp = previousTimestamp;
            ARSAL_Mutex_Unlock(&(sender->streamMutex));

            /* call the auCallback */
            //ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARSTREAM_SENDER2_TAG, "Time %llu: cancel frame -> auCallback", previousTimestamp);
            sender->auCallback(ARSTREAM_SENDER2_STATUS_CANCELLED, previousAuUserPtr, sender->custom);
        }
        else
        {
            ARSAL_Mutex_Unlock(&(sender->streamMutex));
        }
    }

    /* flush the NALU FIFO */
    ARSTREAM_Sender2_FlushNaluFifo(sender);

    free(sendBuffer);

    ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARSTREAM_SENDER2_TAG, "Stream sender sending thread ended");

    return (void*)0;
}


void* ARSTREAM_Sender2_RunRecvThread(void *ARSTREAM_Sender2_t_Param)
{
    ARSTREAM_Sender2_t *sender = (ARSTREAM_Sender2_t*)ARSTREAM_Sender2_t_Param;

    ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARSTREAM_SENDER2_TAG, "Stream sender receiving thread running");
    ARSAL_Mutex_Lock(&(sender->streamMutex));
    sender->recvThreadStarted = 1;
    ARSAL_Mutex_Unlock(&(sender->streamMutex));

    ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARSTREAM_SENDER2_TAG, "Stream sender receiving thread ended");
    ARSAL_Mutex_Lock(&(sender->streamMutex));
    sender->recvThreadStarted = 0;
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
        ret = sender->targetPacketSize + sizeof(ARSTREAM_NetworkHeaders_DataHeader2_t) + ARSTREAM_NETWORK_UDP_HEADER_SIZE + ARSTREAM_NETWORK_IP_HEADER_SIZE;
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
    sender->targetPacketSize = targetPacketSize - sizeof(ARSTREAM_NetworkHeaders_DataHeader2_t) - ARSTREAM_NETWORK_UDP_HEADER_SIZE - ARSTREAM_NETWORK_IP_HEADER_SIZE;
    ARSAL_Mutex_Unlock(&(sender->streamMutex));

    return ret;
}


int ARSTREAM_Sender2_GetMaxBitrate(ARSTREAM_Sender2_t *sender)
{
    int ret = -1;
    if (sender != NULL)
    {
        ARSAL_Mutex_Lock(&(sender->streamMutex));
        ret = sender->maxBitrate;
        ARSAL_Mutex_Unlock(&(sender->streamMutex));
    }
    return ret;
}


int ARSTREAM_Sender2_GetMaxLatencyMs(ARSTREAM_Sender2_t *sender)
{
    int ret = -1;
    if (sender != NULL)
    {
        ARSAL_Mutex_Lock(&(sender->streamMutex));
        ret = sender->maxLatencyMs;
        ARSAL_Mutex_Unlock(&(sender->streamMutex));
    }
    return ret;
}


int ARSTREAM_Sender2_GetMaxNetworkLatencyMs(ARSTREAM_Sender2_t *sender)
{
    int ret = -1;
    if (sender != NULL)
    {
        ARSAL_Mutex_Lock(&(sender->streamMutex));
        ret = sender->maxNetworkLatencyMs;
        ARSAL_Mutex_Unlock(&(sender->streamMutex));
    }
    return ret;
}


eARSTREAM_ERROR ARSTREAM_Sender2_SetMaxBitrateAndLatencyMs(ARSTREAM_Sender2_t *sender, int maxBitrate, int maxLatencyMs, int maxNetworkLatencyMs)
{
    eARSTREAM_ERROR ret = ARSTREAM_OK;
    if ((sender == NULL) || (maxBitrate <= 0) || (maxLatencyMs < 0) || (maxNetworkLatencyMs <= 0))
    {
        return ARSTREAM_ERROR_BAD_PARAMETERS;
    }

    ARSAL_Mutex_Lock(&(sender->streamMutex));
    sender->maxBitrate = maxBitrate;
    sender->maxLatencyMs = maxLatencyMs;
    sender->maxNetworkLatencyMs = maxNetworkLatencyMs;
    int totalBufSize = maxBitrate * maxNetworkLatencyMs / 1000 / 8;
    sender->sendSocketBufferSize = totalBufSize / 2;
    sender->naluFifoBufferSize = totalBufSize / 2;
    int err = ARSTREAM_Sender2_SetSocketSendBufferSize(sender, sender->sendSocketBufferSize);
    if (err != 0)
    {
        ret = ARSTREAM_ERROR_BAD_PARAMETERS;
    }
    ARSAL_Mutex_Unlock(&(sender->streamMutex));

    return ret;
}


eARSTREAM_ERROR ARSTREAM_Sender2_GetMonitoring(ARSTREAM_Sender2_t *sender, uint64_t startTime, uint32_t timeIntervalUs, uint32_t *realTimeIntervalUs, uint32_t *meanAcqToNetworkTime,
                                               uint32_t *acqToNetworkJitter, uint32_t *meanNetworkTime, uint32_t *networkJitter, uint32_t *bytesSent, uint32_t *meanPacketSize,
                                               uint32_t *packetSizeStdDev, uint32_t *packetsSent, uint32_t *bytesDropped, uint32_t *naluDropped)
{
    eARSTREAM_ERROR ret = ARSTREAM_OK;
    uint64_t endTime, curTime, previousTime, acqToNetworkSum = 0, networkSum = 0, acqToNetworkVarSum = 0, networkVarSum = 0, packetSizeVarSum = 0;
    uint32_t _bytesSent, _bytesDropped, bytesSentSum = 0, bytesDroppedSum = 0, _meanPacketSize = 0, acqToNetworkTime = 0, networkTime = 0;
    uint32_t _acqToNetworkJitter = 0, _networkJitter = 0, _meanAcqToNetworkTime = 0, _meanNetworkTime = 0, _packetSizeStdDev = 0;
    int points = 0, usefulPoints = 0, _packetsSent = 0, _naluDropped = 0, idx, i, firstUsefulIdx = -1;

    //TODO: code review

    if ((sender == NULL) || (timeIntervalUs == 0))
    {
        return ARSTREAM_ERROR_BAD_PARAMETERS;
    }

    if (startTime == 0)
    {
        struct timespec t1;
        ARSAL_Time_GetTime(&t1);
        startTime = (uint64_t)t1.tv_sec * 1000000 + (uint64_t)t1.tv_nsec / 1000;
    }
    endTime = startTime;

    ARSAL_Mutex_Lock(&(sender->monitoringMutex));

    if (sender->monitoringCount > 0)
    {
        idx = sender->monitoringIndex;
        previousTime = startTime;

        while (points < sender->monitoringCount)
        {
            curTime = sender->monitoringPoint[idx].sendTimestamp;
            if (curTime > startTime)
            {
                points++;
                idx = (idx - 1 >= 0) ? idx - 1 : ARSTREAM_SENDER2_MONITORING_MAX_POINTS - 1;
                continue;
            }
            if (startTime - curTime > timeIntervalUs)
            {
                break;
            }
            if (firstUsefulIdx == -1)
            {
                firstUsefulIdx = idx;
            }
            _bytesSent = sender->monitoringPoint[idx].bytesSent;
            bytesSentSum += _bytesSent;
            if (_bytesSent)
            {
                _packetsSent++;
                acqToNetworkTime = curTime - sender->monitoringPoint[idx].auTimestamp;
                acqToNetworkSum += acqToNetworkTime;
                networkTime = curTime - sender->monitoringPoint[idx].inputTimestamp;
                networkSum += networkTime;
            }
            _bytesDropped = sender->monitoringPoint[idx].bytesDropped;
            bytesDroppedSum += _bytesDropped;
            if (_bytesDropped)
            {
                _naluDropped++;
            }
            previousTime = curTime;
            usefulPoints++;
            points++;
            idx = (idx - 1 >= 0) ? idx - 1 : ARSTREAM_SENDER2_MONITORING_MAX_POINTS - 1;
        }

        endTime = previousTime;
        _meanPacketSize = (_packetsSent > 0) ? (bytesSentSum / _packetsSent) : 0;
        _meanAcqToNetworkTime = (_packetsSent > 0) ? (uint32_t)(acqToNetworkSum / _packetsSent) : 0;
        _meanNetworkTime = (_packetsSent > 0) ? (uint32_t)(networkSum / _packetsSent) : 0;

        if ((acqToNetworkJitter) || (networkJitter) || (packetSizeStdDev))
        {
            for (i = 0, idx = firstUsefulIdx; i < usefulPoints; i++)
            {
                idx = (idx - 1 >= 0) ? idx - 1 : ARSTREAM_SENDER2_MONITORING_MAX_POINTS - 1;
                curTime = sender->monitoringPoint[idx].sendTimestamp;
                _bytesSent = sender->monitoringPoint[idx].bytesSent;
                if (_bytesSent)
                {
                    acqToNetworkTime = curTime - sender->monitoringPoint[idx].auTimestamp;
                    networkTime = curTime - sender->monitoringPoint[idx].inputTimestamp;
                    packetSizeVarSum += ((_bytesSent - _meanPacketSize) * (_bytesSent - _meanPacketSize));
                    acqToNetworkVarSum += ((acqToNetworkTime - _meanAcqToNetworkTime) * (acqToNetworkTime - _meanAcqToNetworkTime));
                    networkVarSum += ((networkTime - _meanNetworkTime) * (networkTime - _meanNetworkTime));
                }
            }
            _acqToNetworkJitter = (_packetsSent > 0) ? (uint32_t)(sqrt((double)acqToNetworkVarSum / _packetsSent)) : 0;
            _networkJitter = (_packetsSent > 0) ? (uint32_t)(sqrt((double)networkVarSum / _packetsSent)) : 0;
            _packetSizeStdDev = (_packetsSent > 0) ? (uint32_t)(sqrt((double)packetSizeVarSum / _packetsSent)) : 0;
        }
    }

    ARSAL_Mutex_Unlock(&(sender->monitoringMutex));

    if (realTimeIntervalUs)
    {
        *realTimeIntervalUs = (startTime - endTime);
    }
    if (meanAcqToNetworkTime)
    {
        *meanAcqToNetworkTime = _meanAcqToNetworkTime;
    }
    if (acqToNetworkJitter)
    {
        *acqToNetworkJitter = _acqToNetworkJitter;
    }
    if (meanNetworkTime)
    {
        *meanNetworkTime = _meanNetworkTime;
    }
    if (networkJitter)
    {
        *networkJitter = _networkJitter;
    }
    if (bytesSent)
    {
        *bytesSent = bytesSentSum;
    }
    if (meanPacketSize)
    {
        *meanPacketSize = _meanPacketSize;
    }
    if (packetSizeStdDev)
    {
        *packetSizeStdDev = _packetSizeStdDev;
    }
    if (packetsSent)
    {
        *packetsSent = _packetsSent;
    }
    if (bytesDropped)
    {
        *bytesDropped = bytesDroppedSum;
    }
    if (naluDropped)
    {
        *naluDropped = _naluDropped;
    }

    return ret;
}

