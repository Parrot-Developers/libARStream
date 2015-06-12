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
 * @file ARSTREAM_Reader2.c
 * @brief Stream reader on network (v2)
 * @date 04/16/2015
 * @author aurelien.barre@parrot.com
 */

#include <config.h>

/*
 * System Headers
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
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

#include <libARStream/ARSTREAM_Reader2.h>
#include <libARSAL/ARSAL_Print.h>
#include <libARSAL/ARSAL_Mutex.h>
#include <libARSAL/ARSAL_Endianness.h>

/*
 * Macros
 */

#define ARSTREAM_READER2_TAG "ARSTREAM_Reader2"
#define ARSTREAM_READER2_DATAREAD_TIMEOUT_MS (500)

#define ARSTREAM_H264_STARTCODE 0x00000001
#define ARSTREAM_H264_STARTCODE_LENGTH 4

#define ARSTREAM_READER2_DEBUG
#ifdef ARSTREAM_READER2_DEBUG
    #include "ARSTREAM_Reader2Debug.h"
#endif


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

/*
 * Types
 */

struct ARSTREAM_Reader2_t {
    /* Configuration on New */
    ARSTREAM_Reader2_NetworkMode_t networkMode;
    ARNETWORK_Manager_t *manager;
    int dataBufferID;
    int ackBufferID;
    char *ifaceAddr;
    char *recvAddr;
    int recvPort;
    int recvTimeoutSec;
    int maxPacketSize;
    int insertStartCodes;
    int outputIncompleteAu;
    ARSTREAM_Reader2_AuCallback_t auCallback;
    void *custom;

    /* Current frame storage */
    int currentAuBufferSize; // Usable length of the buffer
    int currentAuSize;       // Actual data length
    uint8_t *currentAuBuffer;

    /* Thread status */
    ARSAL_Mutex_t streamMutex;
    int threadsShouldStop;
    int dataThreadStarted;
    int ackThreadStarted;

    /* Sockets */
    int sendSocket;
    int recvSocket;
    int recvMulticast;

#ifdef ARSTREAM_READER2_DEBUG
    /* Debug */
    ARSTREAM_Reader2Debug_t* rdbg;
#endif
};


void ARSTREAM_Reader2_InitStreamDataBuffer(ARNETWORK_IOBufferParam_t *bufferParams, int bufferID, int maxPacketSize)
{
    ARSTREAM_Buffers_InitStreamDataBuffer(bufferParams, bufferID, sizeof(ARSTREAM_NetworkHeaders_DataHeader2_t), maxPacketSize, ARSTREAM_BUFFERS_DATA_BUFFER_NUMBER_OF_CELLS);
}


void ARSTREAM_Reader2_InitStreamAckBuffer(ARNETWORK_IOBufferParam_t *bufferParams, int bufferID)
{
    ARSTREAM_Buffers_InitStreamAckBuffer(bufferParams, bufferID);
}


ARSTREAM_Reader2_t* ARSTREAM_Reader2_New(ARSTREAM_Reader2_Config_t *config, uint8_t *auBuffer, int auBufferSize, void *custom, eARSTREAM_ERROR *error)
{
    ARSTREAM_Reader2_t *retReader = NULL;
    int streamMutexWasInit = 0;
    eARSTREAM_ERROR internalError = ARSTREAM_OK;
    /* ARGS Check */
    if ((config == NULL) ||
        ((config->networkMode == ARSTREAM_READER2_NETWORK_MODE_ARNETWORK) &&
            (config->manager == NULL)) ||
        ((config->networkMode == ARSTREAM_READER2_NETWORK_MODE_SOCKET) &&
            ((config->recvPort <= 0) || (config->recvTimeoutSec <= 0))) ||
        (config->auCallback == NULL) ||
        (auBuffer == NULL) ||
        (auBufferSize == 0) ||
        (config->maxPacketSize < 100))
    {
        SET_WITH_CHECK(error, ARSTREAM_ERROR_BAD_PARAMETERS);
        return retReader;
    }

    /* Alloc new reader */
    retReader = malloc(sizeof(ARSTREAM_Reader2_t));
    if (retReader == NULL)
    {
        internalError = ARSTREAM_ERROR_ALLOC;
    }

    /* Initialize the reader and copy parameters */
    if (internalError == ARSTREAM_OK)
    {
        memset(retReader, 0, sizeof(ARSTREAM_Reader2_t));
        retReader->recvMulticast = 0;
        retReader->sendSocket = -1;
        retReader->recvSocket = -1;
        retReader->networkMode = config->networkMode;
        retReader->manager = config->manager;
        retReader->dataBufferID = config->dataBufferID;
        retReader->ackBufferID = config->ackBufferID;
        if (config->networkMode == ARSTREAM_READER2_NETWORK_MODE_SOCKET)
        {
            if (config->recvAddr)
            {
                retReader->recvAddr = strndup(config->recvAddr, 16);
            }
            if (config->ifaceAddr)
            {
                retReader->ifaceAddr = strndup(config->ifaceAddr, 16);
            }
        }
        retReader->recvPort = config->recvPort;
        retReader->maxPacketSize = config->maxPacketSize - sizeof(ARSTREAM_NetworkHeaders_DataHeader2_t) - ARSTREAM_NETWORK_UDP_HEADER_SIZE - ARSTREAM_NETWORK_IP_HEADER_SIZE;
        retReader->insertStartCodes = config->insertStartCodes;
        retReader->outputIncompleteAu = config->outputIncompleteAu;
        retReader->auCallback = config->auCallback;
        retReader->custom = custom;
        retReader->currentAuBufferSize = auBufferSize;
        retReader->currentAuBuffer = auBuffer;
    }

    /* Setup internal mutexes/sems */
    if (internalError == ARSTREAM_OK)
    {
        int mutexInitRet = ARSAL_Mutex_Init(&(retReader->streamMutex));
        if (mutexInitRet != 0)
        {
            internalError = ARSTREAM_ERROR_ALLOC;
        }
        else
        {
            streamMutexWasInit = 1;
        }
    }

#ifdef ARSTREAM_READER2_DEBUG
    /* Setup debug */
    if (internalError == ARSTREAM_OK)
    {
        retReader->rdbg = ARSTREAM_Reader2Debug_New(1, 0);
        if (!retReader->rdbg)
        {
            internalError = ARSTREAM_ERROR_ALLOC;
        }
    }
#endif

    if ((internalError != ARSTREAM_OK) &&
        (retReader != NULL))
    {
        if (streamMutexWasInit == 1)
        {
            ARSAL_Mutex_Destroy(&(retReader->streamMutex));
        }
        if ((retReader) && (retReader->recvAddr))
        {
            free(retReader->recvAddr);
        }
        if ((retReader) && (retReader->ifaceAddr))
        {
            free(retReader->ifaceAddr);
        }
        free(retReader);
        retReader = NULL;
    }

    SET_WITH_CHECK(error, internalError);
    return retReader;
}


void ARSTREAM_Reader2_StopReader(ARSTREAM_Reader2_t *reader)
{
    if (reader != NULL)
    {
        ARSAL_Mutex_Lock(&(reader->streamMutex));
        reader->threadsShouldStop = 1;
        ARSAL_Mutex_Unlock(&(reader->streamMutex));
    }
}


eARSTREAM_ERROR ARSTREAM_Reader2_Delete(ARSTREAM_Reader2_t **reader)
{
    eARSTREAM_ERROR retVal = ARSTREAM_ERROR_BAD_PARAMETERS;
    if ((reader != NULL) &&
        (*reader != NULL))
    {
        int canDelete = 0;
        ARSAL_Mutex_Lock(&((*reader)->streamMutex));
        if (((*reader)->dataThreadStarted == 0) &&
            ((*reader)->ackThreadStarted == 0))
        {
            canDelete = 1;
        }
        ARSAL_Mutex_Unlock(&((*reader)->streamMutex));

        if (canDelete == 1)
        {
#ifdef ARSTREAM_READER2_DEBUG
            ARSTREAM_Reader2Debug_Delete(&(*reader)->rdbg);
#endif
            ARSAL_Mutex_Destroy(&((*reader)->streamMutex));
            if ((*reader)->sendSocket != -1)
            {
                ARSAL_Socket_Close((*reader)->sendSocket);
                (*reader)->sendSocket = -1;
            }
            if ((*reader)->recvSocket != -1)
            {
                ARSAL_Socket_Close((*reader)->recvSocket);
                (*reader)->recvSocket = -1;
            }
            if ((*reader)->recvAddr)
            {
                free((*reader)->recvAddr);
            }
            if ((*reader)->ifaceAddr)
            {
                free((*reader)->ifaceAddr);
            }
            free(*reader);
            *reader = NULL;
            retVal = ARSTREAM_OK;
        }
        else
        {
            ARSAL_PRINT(ARSAL_PRINT_ERROR, ARSTREAM_READER2_TAG, "Call ARSTREAM_Reader2_StopReader before calling this function");
            retVal = ARSTREAM_ERROR_BUSY;
        }
    }
    return retVal;
}


static int ARSTREAM_Reader2_SetSocketReceiveBufferSize(ARSTREAM_Reader2_t *reader, int size)
{
    int ret = 0, err;
    int size2 = sizeof(int);

    size /= 2;
    err = ARSAL_Socket_Setsockopt(reader->recvSocket, SOL_SOCKET, SO_RCVBUF, (void*)&size, sizeof(size));
    if (err != 0)
    {
        ret = -1;
        ARSAL_PRINT(ARSAL_PRINT_ERROR, ARSTREAM_READER2_TAG, "Failed to set receive socket buffer size to 2*%d bytes: error=%d (%s)", size, errno, strerror(errno));
    }

    size = -1;
    err = ARSAL_Socket_Getsockopt(reader->recvSocket, SOL_SOCKET, SO_RCVBUF, (void*)&size, &size2);
    if (err != 0)
    {
        ret = -1;
        ARSAL_PRINT(ARSAL_PRINT_ERROR, ARSTREAM_READER2_TAG, "Failed to get receive socket buffer size: error=%d (%s)", errno, strerror(errno));
    }
    else
    {
        ARSAL_PRINT(ARSAL_PRINT_ERROR, ARSTREAM_READER2_TAG, "Receive socket buffer size is %d bytes", size); //TODO: debug
    }

    return ret;
}


static int ARSTREAM_Reader2_Bind(ARSTREAM_Reader2_t *reader)
{
    int ret = 0;

    if (reader->networkMode == ARSTREAM_READER2_NETWORK_MODE_SOCKET)
    {
        struct sockaddr_in recvSin;
        struct timespec timeout;
        int err;

        /* check parameters */
        //TODO

        /* create socket */
        reader->recvSocket = ARSAL_Socket_Create(AF_INET, SOCK_DGRAM, 0);
        if (reader->recvSocket < 0)
        {
            ARSAL_PRINT(ARSAL_PRINT_ERROR, ARSTREAM_READER2_TAG, "Failed to create socket");
            ret = -1;
        }
        ARSAL_PRINT(ARSAL_PRINT_ERROR, ARSTREAM_READER2_TAG, "Socket OK"); //TODO: debug

        if (ret == 0)
        {
            /* initialize socket */

            /* set to non-blocking */
            int flags = fcntl(reader->recvSocket, F_GETFL, 0);
            fcntl(reader->recvSocket, F_SETFL, flags | O_NONBLOCK);

            memset(&recvSin, 0, sizeof(struct sockaddr_in));
            recvSin.sin_family = AF_INET;
            recvSin.sin_port = htons(reader->recvPort);
            recvSin.sin_addr.s_addr = htonl(INADDR_ANY);

            if ((reader->recvAddr) && (strlen(reader->recvAddr)))
            {
                int addrFirst = atoi(reader->recvAddr);
                if ((addrFirst >= 224) && (addrFirst <= 239))
                {
                    /* multicast */
                    struct ip_mreq mreq;
                    memset(&mreq, 0, sizeof(mreq));
                    err = inet_pton(AF_INET, reader->recvAddr, &(mreq.imr_multiaddr.s_addr));
                    if (err <= 0)
                    {
                        ARSAL_PRINT(ARSAL_PRINT_ERROR, ARSTREAM_READER2_TAG, "Failed to convert address '%s'", reader->recvAddr);
                        ret = -1;
                    }

                    if (ret == 0)
                    {
                        if ((reader->ifaceAddr) && (strlen(reader->ifaceAddr) > 0))
                        {
                            err = inet_pton(AF_INET, reader->ifaceAddr, &(mreq.imr_interface.s_addr));
                            if (err <= 0)
                            {
                                ARSAL_PRINT(ARSAL_PRINT_ERROR, ARSTREAM_READER2_TAG, "Failed to convert address '%s'", reader->ifaceAddr);
                                ret = -1;
                            }
                        }
                        else
                        {
                            mreq.imr_interface.s_addr = htonl(INADDR_ANY);
                        }
                    }

                    if (ret == 0)
                    {
                        /* join the multicast group */
                        err = ARSAL_Socket_Setsockopt(reader->recvSocket, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*)&mreq, sizeof(mreq));
                        if (err != 0)
                        {
                            ARSAL_PRINT(ARSAL_PRINT_ERROR, ARSTREAM_READER2_TAG, "Failed to join multacast group: error=%d (%s)", errno, strerror(errno));
                            ret = -1;
                        }
                        ARSAL_PRINT(ARSAL_PRINT_ERROR, ARSTREAM_READER2_TAG, "Multicast join OK"); //TODO: debug
                    }

                    reader->recvMulticast = 1;
                }
                else
                {
                    /* unicast */
                    if ((reader->ifaceAddr) && (strlen(reader->ifaceAddr) > 0))
                    {
                        err = inet_pton(AF_INET, reader->ifaceAddr, &(recvSin.sin_addr));
                        if (err <= 0)
                        {
                            ARSAL_PRINT(ARSAL_PRINT_ERROR, ARSTREAM_READER2_TAG, "Failed to convert address '%s'", reader->ifaceAddr);
                            ret = -1;
                        }
                    }
                }
            }
            else
            {
                /* unicast */
                if ((reader->ifaceAddr) && (strlen(reader->ifaceAddr) > 0))
                {
                    err = inet_pton(AF_INET, reader->ifaceAddr, &(recvSin.sin_addr));
                    if (err <= 0)
                    {
                        ARSAL_PRINT(ARSAL_PRINT_ERROR, ARSTREAM_READER2_TAG, "Failed to convert address '%s'", reader->ifaceAddr);
                        ret = -1;
                    }
                }
            }
        }

        if (ret == 0)
        {
            /* allow multiple sockets to use the same port */
            unsigned int yes = 1;
            err = ARSAL_Socket_Setsockopt(reader->recvSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&yes, sizeof(yes));
            if (err != 0)
            {
                ARSAL_PRINT(ARSAL_PRINT_ERROR, ARSTREAM_READER2_TAG, "Failed to set socket option SO_REUSEADDR: error=%d (%s)", errno, strerror(errno));
                ret = -1;
            }
            ARSAL_PRINT(ARSAL_PRINT_ERROR, ARSTREAM_READER2_TAG, "Reuse address OK"); //TODO: debug
        }

        if (ret == 0)
        {
            /* set the socket timeout */
            timeout.tv_sec = reader->recvTimeoutSec;
            timeout.tv_nsec = 0;
            err = ARSAL_Socket_Setsockopt(reader->recvSocket, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
            if (err != 0)
            {
                ARSAL_PRINT(ARSAL_PRINT_ERROR, ARSTREAM_READER2_TAG, "Failed to set socket receive timeout: error=%d (%s)", errno, strerror(errno));
                ret = -1;
            }
            ARSAL_PRINT(ARSAL_PRINT_ERROR, ARSTREAM_READER2_TAG, "Timeout OK"); //TODO: debug
        }

        if (ret == 0)
        {
            /* bind the socket */
            err = ARSAL_Socket_Bind(reader->recvSocket, (struct sockaddr*)&recvSin, sizeof(recvSin));
            if (err != 0)
            {
                ARSAL_PRINT(ARSAL_PRINT_ERROR, ARSTREAM_READER2_TAG, "Error on socket bind port=%d: error=%d (%s)", reader->recvPort, errno, strerror(errno));
                ret = -1;
            }
            ARSAL_PRINT(ARSAL_PRINT_ERROR, ARSTREAM_READER2_TAG, "Bind OK"); //TODO: debug
        }

        if (ret == 0)
        {
            /* set the socket buffer size */
            err = ARSTREAM_Reader2_SetSocketReceiveBufferSize(reader, 600 * 1024); //TODO
            if (err != 0)
            {
                ARSAL_PRINT(ARSAL_PRINT_ERROR, ARSTREAM_READER2_TAG, "Failed to set the socket buffer size (%d)", err);
                ret = -1;
            }
        }

        if (ret != 0)
        {
            if (reader->recvSocket >= 0)
            {
                ARSAL_Socket_Close(reader->recvSocket);
                ARSAL_PRINT(ARSAL_PRINT_ERROR, ARSTREAM_READER2_TAG, "Close!!!"); //TODO: debug
            }
            reader->recvSocket = -1;
        }
    }
    else if (reader->networkMode == ARSTREAM_READER2_NETWORK_MODE_ARNETWORK)
    {
        /* nothing to do */
    }

    return ret;
}


static int ARSTREAM_Reader2_ReadData(ARSTREAM_Reader2_t *reader, uint8_t *recvBuffer, int recvBufferSize, int *recvSize)
{
    int ret = 0;

    if ((!recvBuffer) || (!recvSize))
    {
        return -1;
    }

    if (reader->networkMode == ARSTREAM_READER2_NETWORK_MODE_SOCKET)
    {
        fd_set set;
        FD_ZERO(&set);
        FD_SET(reader->recvSocket, &set);
        int maxFd = reader->recvSocket + 1;
        struct timeval tv = { 0, ARSTREAM_READER2_DATAREAD_TIMEOUT_MS * 1000 };
        int err = select(maxFd, &set, NULL, NULL, &tv);
        if (err < 0)
        {
            /* read error */
            ARSAL_PRINT(ARSAL_PRINT_ERROR, ARSTREAM_READER2_TAG, "Error on select: error=%d (%s)", errno, strerror(errno));
            ret = -1;
            *recvSize = 0;
        }
        else
        {
            /* no read error (timeout or FD ready) */
            if (FD_ISSET(reader->recvSocket, &set))
            {
                int size;
                size = ARSAL_Socket_Recv(reader->recvSocket, recvBuffer, recvBufferSize, 0);
                if (size > 0)
                {
                    /* save the number of bytes read */
                    *recvSize = size;
                }
                else if (size == 0)
                {
                    /* this should never happen (if the socket is ready, data must be available) */
                    ARSAL_PRINT(ARSAL_PRINT_ERROR, ARSTREAM_READER2_TAG, "Socket received size is null");
                    ret = -2;
                    *recvSize = 0;
                }
                else
                {
                    ARSAL_PRINT(ARSAL_PRINT_ERROR, ARSTREAM_READER2_TAG, "Socket receive error %d ('%s')", errno, strerror(errno));
                    ret = -1;
                    *recvSize = 0;
                }
            }
            else
            {
                /* read timeout */
                ARSAL_PRINT(ARSAL_PRINT_ERROR, ARSTREAM_READER2_TAG, "Socket receive timeout");
                ret = -2;
                *recvSize = 0;
            }
        }
    }
    else if (reader->networkMode == ARSTREAM_READER2_NETWORK_MODE_ARNETWORK)
    {
        eARNETWORK_ERROR err = ARNETWORK_Manager_ReadDataWithTimeout(reader->manager, reader->dataBufferID, recvBuffer, recvBufferSize, recvSize, ARSTREAM_READER2_DATAREAD_TIMEOUT_MS);
        if (err != ARNETWORK_OK)
        {
            if (err == ARNETWORK_ERROR_BUFFER_EMPTY)
            {
                ret = -2;
                *recvSize = 0;
            }
            else
            {
                ARSAL_PRINT(ARSAL_PRINT_ERROR, ARSTREAM_READER2_TAG, "Error while reading stream data: %s", ARNETWORK_Error_ToString(err));
                ret = -1;
                *recvSize = 0;
            }
        }
    }

    return ret;
}


static int ARSTREAM_Reader2_CheckBufferSize(ARSTREAM_Reader2_t *reader, int payloadSize)
{
    int ret = 0;

    if (reader->currentAuSize + payloadSize > reader->currentAuBufferSize)
    {
        uint32_t nextAuBufferSize = reader->currentAuSize + payloadSize, dummy = 0;
        uint8_t *nextAuBuffer = reader->auCallback(ARSTREAM_READER2_CAUSE_AU_BUFFER_TOO_SMALL, reader->currentAuBuffer, 0, 0, 0, 0, &nextAuBufferSize, reader->custom);
        ret = -1;
        if ((nextAuBufferSize > 0) && (nextAuBufferSize >= reader->currentAuSize + payloadSize))
        {
            memcpy(nextAuBuffer, reader->currentAuBuffer, reader->currentAuSize);
            reader->auCallback(ARSTREAM_READER2_CAUSE_AU_COPY_COMPLETE, reader->currentAuBuffer, 0, 0, 0, 0, &dummy, reader->custom);
            ret = 0;
        }
        reader->currentAuBuffer = nextAuBuffer;
        reader->currentAuBufferSize = nextAuBufferSize;
    }

    return ret;
}


void* ARSTREAM_Reader2_RunDataThread(void *ARSTREAM_Reader2_t_Param)
{
    ARSTREAM_Reader2_t *reader = (ARSTREAM_Reader2_t *)ARSTREAM_Reader2_t_Param;
    uint8_t *recvBuffer = NULL;
    int recvBufferSize;
    int recvSize, payloadSize;
    int fuPending = 0;
    ARSTREAM_NetworkHeaders_DataHeader2_t *header = NULL;
    uint64_t currentTimestamp = 0, previousTimestamp = 0, receptionTs = 0;
    uint16_t currentFlags;
    int startSeqNum = -1, previousSeqNum = -1, currentSeqNum, seqNumDelta;
    int gapsInSeqNum = 0;
    uint32_t startCode = 0;
    int startCodeLength = 0;
    int shouldStop;
    int ret;

    /* Parameters check */
    if (reader == NULL)
    {
        ARSAL_PRINT(ARSAL_PRINT_ERROR, ARSTREAM_READER2_TAG, "Error while starting %s, bad parameters", __FUNCTION__);
        return (void *)0;
    }

    recvBufferSize = reader->maxPacketSize + sizeof(ARSTREAM_NetworkHeaders_DataHeader2_t);
    if (reader->insertStartCodes)
    {
        startCode = htonl(ARSTREAM_H264_STARTCODE);
        startCodeLength = ARSTREAM_H264_STARTCODE_LENGTH;
    }

    /* Alloc and check */
    recvBuffer = malloc(recvBufferSize);
    if (recvBuffer == NULL)
    {
        ARSAL_PRINT(ARSAL_PRINT_ERROR, ARSTREAM_READER2_TAG, "Error while starting %s, can not alloc memory", __FUNCTION__);
        return (void *)0;
    }
    header = (ARSTREAM_NetworkHeaders_DataHeader2_t *)recvBuffer;

    /* Bind */
    ret = ARSTREAM_Reader2_Bind(reader);
    if (ret != 0)
    {
        ARSAL_PRINT(ARSAL_PRINT_ERROR, ARSTREAM_READER2_TAG, "Failed to bind (error %d) - aborting", ret);
        free(recvBuffer);
        return (void*)0;
    }

    ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARSTREAM_READER2_TAG, "Stream reader thread running");
    ARSAL_Mutex_Lock(&(reader->streamMutex));
    reader->dataThreadStarted = 1;
    shouldStop = reader->threadsShouldStop;
    ARSAL_Mutex_Unlock(&(reader->streamMutex));

    while (shouldStop == 0)
    {
        ret = ARSTREAM_Reader2_ReadData(reader, recvBuffer, recvBufferSize, &recvSize);
        if (ret != 0)
        {
            if (ret != -2)
            {
                ARSAL_PRINT(ARSAL_PRINT_ERROR, ARSTREAM_READER2_TAG, "Failed to read data (%d)", ret);
            }
        }
        else if (recvSize >= sizeof(ARSTREAM_NetworkHeaders_DataHeader2_t))
        {
            currentTimestamp = (((uint64_t)ntohl(header->timestamp) * 1000) + 45) / 90; /* 90000 Hz clock to microseconds */
            currentSeqNum = (int)ntohs(header->seqNum);
            if (reader->currentAuSize == 0)
            {
                startSeqNum = currentSeqNum;
            }
            currentFlags = ntohs(header->flags);

            if (previousSeqNum != -1)
            {
                seqNumDelta = currentSeqNum - previousSeqNum;
                if (seqNumDelta < -32768) seqNumDelta += 65536; /* handle seqNum 16 bits loopback */
                gapsInSeqNum += seqNumDelta - 1;
            }
            else
            {
                seqNumDelta = 1;
            }

            if (seqNumDelta > 0)
            {
                if ((previousTimestamp != 0) && (currentTimestamp != previousTimestamp))
                {
                    if (gapsInSeqNum)
                    {
                        if (reader->outputIncompleteAu)
                        {
                            struct timespec res;
                            ARSAL_Time_GetLocalTime(&res, NULL);
                            receptionTs = (uint64_t)res.tv_sec * 1000000 + (uint64_t)res.tv_nsec / 1000;
                            ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARSTREAM_READER2_TAG, "Output incomplete access unit before seqNum %d, size %d bytes (missing %d packets)", currentSeqNum, reader->currentAuSize, gapsInSeqNum);
#ifdef ARSTREAM_READER2_DEBUG
                            ARSTREAM_Reader2Debug_ProcessAu(reader->rdbg, reader->currentAuBuffer, reader->currentAuSize, previousTimestamp, receptionTs, gapsInSeqNum, previousSeqNum - startSeqNum + 1);
#endif
                            reader->currentAuBuffer = reader->auCallback(ARSTREAM_READER2_CAUSE_AU_INCOMPLETE, reader->currentAuBuffer, reader->currentAuSize, previousTimestamp, gapsInSeqNum, previousSeqNum - startSeqNum + 1, &(reader->currentAuBufferSize), reader->custom);
                        }
                        else
                        {
                            ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARSTREAM_READER2_TAG, "Dropping incomplete access unit before seqNum %d, size %d bytes (missing %d packets)", currentSeqNum, reader->currentAuSize, gapsInSeqNum);
                        }
                    }
                    gapsInSeqNum = 0;
                    reader->currentAuSize = 0;
                }

                payloadSize = recvSize - sizeof(ARSTREAM_NetworkHeaders_DataHeader2_t);

                if (payloadSize >= 1)
                {
                    uint8_t headByte = *(recvBuffer + sizeof(ARSTREAM_NetworkHeaders_DataHeader2_t));

                    if ((headByte & 0x1F) == ARSTREAM_NETWORK_HEADERS2_NALU_TYPE_FUA)
                    {
                        /* Fragmentation (FU-A) */
                        if (payloadSize >= 2)
                        {
                            uint8_t fuIndicator, fuHeader, startBit, endBit;
                            int outputSize = payloadSize - 2;
                            fuIndicator = headByte;
                            fuHeader = *(recvBuffer + sizeof(ARSTREAM_NetworkHeaders_DataHeader2_t) + 1);
                            startBit = fuHeader & 0x80;
                            endBit = fuHeader & 0x40;

                            if ((fuPending) && (startBit))
                            {
                                //TODO: drop the previous incomplete FU-A?
                                fuPending = 0;
                                ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARSTREAM_READER2_TAG, "Incomplete FU-A packet before FU-A at seqNum %d ((fuPending) && (startBit))", currentSeqNum);
                            }
                            if (startBit)
                            {
                                fuPending = 1;
                            }
                            if (fuPending)
                            {
                                outputSize += (startBit) ? startCodeLength + 1 : 0;
                                if (!ARSTREAM_Reader2_CheckBufferSize(reader, outputSize))
                                {
                                    if ((startCodeLength > 0) && (startBit))
                                    {
                                        memcpy(reader->currentAuBuffer + reader->currentAuSize, &startCode, startCodeLength);
                                        reader->currentAuSize += startCodeLength;
                                    }
                                    memcpy(reader->currentAuBuffer + reader->currentAuSize + ((startBit) ? 1 : 0), recvBuffer + sizeof(ARSTREAM_NetworkHeaders_DataHeader2_t) + 2, payloadSize - 2);
                                    if (startBit)
                                    {
                                        /* restore the NALU header byte */
                                        *(reader->currentAuBuffer + reader->currentAuSize) = (fuIndicator & 0xE0) | (fuHeader & 0x1F);
                                        reader->currentAuSize++;
                                    }
                                    reader->currentAuSize += payloadSize - 2;
                                }
                                else
                                {
                                    ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARSTREAM_READER2_TAG, "Failed to realloc auBuffer for output size (%d) for FU-A packet at seqNum %d", outputSize, currentSeqNum);
                                }
                            }
                            if (endBit)
                            {
                                fuPending = 0;
                            }
                        }
                        else
                        {
                            ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARSTREAM_READER2_TAG, "Invalid payload size (%d) for FU-A packet at seqNum %d", payloadSize, currentSeqNum);
                        }
                    }
                    else if ((headByte & 0x31) == ARSTREAM_NETWORK_HEADERS2_NALU_TYPE_STAPA)
                    {
                        /* Aggregation (STAP-A) */
                        if (fuPending)
                        {
                            //TODO: drop the previous incomplete FU-A?
                            fuPending = 0;
                            ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARSTREAM_READER2_TAG, "Incomplete FU-A packet before STAP-A at seqNum %d (fuPending)", currentSeqNum);
                        }

                        //TODO
                    }
                    else
                    {
                        /* Single NAL unit */
                        if (fuPending)
                        {
                            //TODO: drop the previous incomplete FU-A?
                            fuPending = 0;
                            ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARSTREAM_READER2_TAG, "Incomplete FU-A packet before single NALU at seqNum %d (fuPending)", currentSeqNum);
                        }

                        if (!ARSTREAM_Reader2_CheckBufferSize(reader, payloadSize + startCodeLength))
                        {
                            if (startCodeLength > 0)
                            {
                                memcpy(reader->currentAuBuffer + reader->currentAuSize, &startCode, startCodeLength);
                                reader->currentAuSize += startCodeLength;
                            }
                            memcpy(reader->currentAuBuffer + reader->currentAuSize, recvBuffer + sizeof(ARSTREAM_NetworkHeaders_DataHeader2_t), payloadSize);
                            reader->currentAuSize += payloadSize;
                        }
                        else
                        {
                            ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARSTREAM_READER2_TAG, "Failed to realloc auBuffer for output size (%d) for single NALU packet at seqNum %d", payloadSize + startCodeLength, currentSeqNum);
                        }
                    }
                }
                else
                {
                    ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARSTREAM_READER2_TAG, "Invalid payload size (%d) for packet at seqNum %d", payloadSize, currentSeqNum);
                }

                if (currentFlags & (1 << 7))
                {
                    /* the marker bit is set: output the access unit */
                    struct timespec res;
                    ARSAL_Time_GetLocalTime(&res, NULL);
                    receptionTs = (uint64_t)res.tv_sec * 1000000 + (uint64_t)res.tv_nsec / 1000;
                    ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARSTREAM_READER2_TAG, "Output access unit at seqNum %d, size %d bytes (missing %d packets)", currentSeqNum, reader->currentAuSize, gapsInSeqNum);
#ifdef ARSTREAM_READER2_DEBUG
                    ARSTREAM_Reader2Debug_ProcessAu(reader->rdbg, reader->currentAuBuffer, reader->currentAuSize, currentTimestamp, receptionTs, gapsInSeqNum, currentSeqNum - startSeqNum + 1);
#endif
                    reader->currentAuBuffer = reader->auCallback(ARSTREAM_READER2_CAUSE_AU_COMPLETE, reader->currentAuBuffer, reader->currentAuSize, currentTimestamp, gapsInSeqNum, currentSeqNum - startSeqNum + 1, &(reader->currentAuBufferSize), reader->custom);
                    gapsInSeqNum = 0;
                    reader->currentAuSize = 0;
                }

                previousSeqNum = currentSeqNum;
                previousTimestamp = currentTimestamp;
            }
            else
            {
                /* out of order packet */
                //TODO
                ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARSTREAM_READER2_TAG, "Out of order sequence number (currentSeqNum=%d, previousSeqNum=%d, seqNumDelta=%d)", currentSeqNum, previousSeqNum, seqNumDelta);
            }
        }

        ARSAL_Mutex_Lock(&(reader->streamMutex));
        shouldStop = reader->threadsShouldStop;
        ARSAL_Mutex_Unlock(&(reader->streamMutex));
    }

    reader->auCallback(ARSTREAM_READER2_CAUSE_CANCEL, reader->currentAuBuffer, 0, 0, 0, 0, &(reader->currentAuBufferSize), reader->custom);

    ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARSTREAM_READER2_TAG, "Stream reader thread ended");
    ARSAL_Mutex_Lock(&(reader->streamMutex));
    reader->dataThreadStarted = 0;
    ARSAL_Mutex_Unlock(&(reader->streamMutex));

    if (recvBuffer)
    {
        free(recvBuffer);
        recvBuffer = NULL;
    }

    return (void *)0;
}


void* ARSTREAM_Reader2_RunAckThread(void *ARSTREAM_Reader2_t_Param)
{
    ARSTREAM_Reader2_t *reader = (ARSTREAM_Reader2_t *)ARSTREAM_Reader2_t_Param;

    ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARSTREAM_READER2_TAG, "Ack thread running");
    ARSAL_Mutex_Lock(&(reader->streamMutex));
    reader->ackThreadStarted = 1;
    ARSAL_Mutex_Unlock(&(reader->streamMutex));

    ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARSTREAM_READER2_TAG, "Ack thread ended");
    ARSAL_Mutex_Lock(&(reader->streamMutex));
    reader->ackThreadStarted = 0;
    ARSAL_Mutex_Unlock(&(reader->streamMutex));

    return (void *)0;
}


void* ARSTREAM_Reader2_GetCustom(ARSTREAM_Reader2_t *reader)
{
    void *ret = NULL;
    if (reader != NULL)
    {
        ret = reader->custom;
    }
    return ret;
}

