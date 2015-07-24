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
#include <math.h>

/*
 * Private Headers
 */

#include "ARSTREAM_NetworkHeaders.h"

/*
 * ARSDK Headers
 */

#include <libARStream/ARSTREAM_Reader2.h>
#include <libARStream/ARSTREAM_Sender2.h>
#include <libARSAL/ARSAL_Print.h>
#include <libARSAL/ARSAL_Mutex.h>
#include <libARSAL/ARSAL_Endianness.h>
#include <libARSAL/ARSAL_Socket.h>

/*
 * Macros
 */

#define ARSTREAM_READER2_TAG "ARSTREAM_Reader2"
#define ARSTREAM_READER2_DATAREAD_TIMEOUT_MS (500)
#define ARSTREAM_READER2_RESENDER_MAX_COUNT (4)
#define ARSTREAM_READER2_RESENDER_MAX_NALU_BUFFER_COUNT (1024) //TODO: tune this value
#define ARSTREAM_READER2_RESENDER_NALU_BUFFER_MALLOC_CHUNK_SIZE (4096)
#define ARSTREAM_READER2_MONITORING_MAX_POINTS (2048)

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

typedef struct ARSTREAM_Reader2_MonitoringPoint_s {
    uint64_t recvTimestamp;
    uint32_t timestamp;
    uint16_t seqNum;
    uint16_t markerBit;
    uint32_t bytes;
} ARSTREAM_Reader2_MonitoringPoint_t;


struct ARSTREAM_Reader2_Resender_t {
    ARSTREAM_Reader2_t *reader;
    ARSTREAM_Sender2_t *sender;
    int senderRunning;
};


typedef struct ARSTREAM_Reader2_NaluBuffer_s {
    int useCount;
    uint8_t *naluBuffer;
    int naluBufferSize;
    int naluSize;
    uint64_t auTimestamp;
    int isLastNaluInAu;
} ARSTREAM_Reader2_NaluBuffer_t;


struct ARSTREAM_Reader2_t {
    /* Configuration on New */
    char *ifaceAddr;
    char *recvAddr;
    int recvPort;
    int maxPacketSize;
    int insertStartCodes;
    ARSTREAM_Reader2_NaluCallback_t naluCallback;
    void *custom;

    /* Current frame storage */
    int currentNaluBufferSize; // Usable length of the buffer
    int currentNaluSize;       // Actual data length
    uint8_t *currentNaluBuffer;
    uint32_t firstTimestamp;

    /* Thread status */
    ARSAL_Mutex_t streamMutex;
    int threadsShouldStop;
    int recvThreadStarted;
    int sendThreadStarted;

    /* Sockets */
    int sendSocket;
    int recvSocket;
    int recvMulticast;

    /* Monitoring */
    ARSAL_Mutex_t monitoringMutex;
    int monitoringCount;
    int monitoringIndex;
    ARSTREAM_Reader2_MonitoringPoint_t monitoringPoint[ARSTREAM_READER2_MONITORING_MAX_POINTS];

    /* Resenders */
    ARSTREAM_Reader2_Resender_t *resender[ARSTREAM_READER2_RESENDER_MAX_COUNT];
    int resenderCount;
    ARSAL_Mutex_t naluBufferMutex;
    ARSTREAM_Reader2_NaluBuffer_t naluBuffer[ARSTREAM_READER2_RESENDER_MAX_NALU_BUFFER_COUNT];
    int naluBufferCount;
    

#ifdef ARSTREAM_READER2_DEBUG
    /* Debug */
    ARSTREAM_Reader2Debug_t* rdbg;
#endif
};


static void ARSTREAM_Reader2_Resender_NaluCallback(eARSTREAM_SENDER2_STATUS status, void *naluUserPtr, void *custom)
{
    ARSTREAM_Reader2_Resender_t *resender = (ARSTREAM_Reader2_Resender_t *)custom;
    ARSTREAM_Reader2_t *reader;
    ARSTREAM_Reader2_NaluBuffer_t *naluBuf = (ARSTREAM_Reader2_NaluBuffer_t *)naluUserPtr;

    if ((resender == NULL) || (naluUserPtr == NULL))
    {
        return;
    }

    reader = resender->reader;
    if (reader == NULL)
    {
        return;
    }
    
    ARSAL_Mutex_Lock(&(reader->naluBufferMutex));
    naluBuf->useCount--;
    ARSAL_Mutex_Unlock(&(reader->naluBufferMutex));
}


static ARSTREAM_Reader2_NaluBuffer_t* ARSTREAM_Reader2_Resender_GetAvailableNaluBuffer(ARSTREAM_Reader2_t *reader, int naluSize)
{
    int i, availableCount;
    ARSTREAM_Reader2_NaluBuffer_t *retNaluBuf = NULL;

    for (i = 0, availableCount = 0; i < reader->naluBufferCount; i++)
    {
        ARSTREAM_Reader2_NaluBuffer_t *naluBuf = &reader->naluBuffer[i];
        
        if (naluBuf->useCount <= 0)
        {
            availableCount++;
            if (naluBuf->naluBufferSize >= naluSize)
            {
                retNaluBuf = naluBuf;
                break;
            }
        }
    }

    if ((retNaluBuf == NULL) && (availableCount > 0))
    {
        for (i = 0; i < reader->naluBufferCount; i++)
        {
            ARSTREAM_Reader2_NaluBuffer_t *naluBuf = &reader->naluBuffer[i];
            
            if (naluBuf->useCount <= 0)
            {
                /* round naluSize up to nearest multiple of ARSTREAM_READER2_RESENDER_NALU_BUFFER_MALLOC_CHUNK_SIZE */
                int newSize = ((naluSize + ARSTREAM_READER2_RESENDER_NALU_BUFFER_MALLOC_CHUNK_SIZE - 1) / ARSTREAM_READER2_RESENDER_NALU_BUFFER_MALLOC_CHUNK_SIZE) * ARSTREAM_READER2_RESENDER_NALU_BUFFER_MALLOC_CHUNK_SIZE;
                free(naluBuf->naluBuffer);
                naluBuf->naluBuffer = malloc(newSize);
                if (naluBuf->naluBuffer == NULL)
                {
                    naluBuf->naluBufferSize = 0;
                }
                else
                {
                    ARSAL_PRINT(ARSAL_PRINT_ERROR, ARSTREAM_READER2_TAG, "Reallocated NALU buffer (size: %d) - naluBufferCount = %d", newSize, reader->naluBufferCount); //TODO: debug
                    naluBuf->naluBufferSize = newSize;
                    retNaluBuf = naluBuf;
                    break;
                }
            }
        }
    }

    if ((retNaluBuf == NULL) && (reader->naluBufferCount < ARSTREAM_READER2_RESENDER_MAX_NALU_BUFFER_COUNT))
    {
        ARSTREAM_Reader2_NaluBuffer_t *naluBuf = &reader->naluBuffer[reader->naluBufferCount];
        reader->naluBufferCount++;
        naluBuf->useCount = 0;

        /* round naluSize up to nearest multiple of ARSTREAM_READER2_RESENDER_NALU_BUFFER_MALLOC_CHUNK_SIZE */
        int newSize = ((naluSize + ARSTREAM_READER2_RESENDER_NALU_BUFFER_MALLOC_CHUNK_SIZE - 1) / ARSTREAM_READER2_RESENDER_NALU_BUFFER_MALLOC_CHUNK_SIZE) * ARSTREAM_READER2_RESENDER_NALU_BUFFER_MALLOC_CHUNK_SIZE;
        naluBuf->naluBuffer = malloc(newSize);
        if (naluBuf->naluBuffer == NULL)
        {
            naluBuf->naluBufferSize = 0;
        }
        else
        {
            ARSAL_PRINT(ARSAL_PRINT_ERROR, ARSTREAM_READER2_TAG, "Allocated new NALU buffer (size: %d) - naluBufferCount = %d", newSize, reader->naluBufferCount); //TODO: debug
            naluBuf->naluBufferSize = newSize;
            retNaluBuf = naluBuf;
        }
    }

    return retNaluBuf;
}


static int ARSTREAM_Reader2_ResendNalu(ARSTREAM_Reader2_t *reader, uint8_t *naluBuffer, uint32_t naluSize, uint64_t auTimestamp, int isLastNaluInAu)
{
    int ret = 0, i;
    ARSTREAM_Reader2_NaluBuffer_t *naluBuf = NULL;

    /* remove the byte stream format start code */
    if (reader->insertStartCodes)
    {
        naluBuffer += ARSTREAM_H264_STARTCODE_LENGTH;
        naluSize -= ARSTREAM_H264_STARTCODE_LENGTH;
    }

    ARSAL_Mutex_Lock(&(reader->naluBufferMutex));

    /* get a buffer from the pool */
    naluBuf = ARSTREAM_Reader2_Resender_GetAvailableNaluBuffer(reader, (int)naluSize);
    if (naluBuf == NULL)
    {
        ARSAL_Mutex_Unlock(&(reader->naluBufferMutex));
        ARSAL_PRINT(ARSAL_PRINT_WARNING, ARSTREAM_READER2_TAG, "Failed to get available NALU buffer (naluSize=%d, naluBufferCount=%d)", naluSize, reader->naluBufferCount); //TODO: debug
        return -1;
    }
    memcpy(naluBuf->naluBuffer, naluBuffer, naluSize);
    naluBuf->naluSize = naluSize;
    naluBuf->auTimestamp = auTimestamp;
    naluBuf->isLastNaluInAu = isLastNaluInAu;

    /* esnd the NALU to all resenders */
    for (i = 0; i < reader->resenderCount; i++)
    {
        ARSTREAM_Reader2_Resender_t *resender = reader->resender[i];
        eARSTREAM_ERROR err;

        naluBuf->useCount++;
        ARSAL_Mutex_Unlock(&(reader->naluBufferMutex));
        
        err = ARSTREAM_Sender2_SendNewNalu(resender->sender, naluBuf->naluBuffer, naluSize, auTimestamp, isLastNaluInAu, NULL, naluBuf);

        ARSAL_Mutex_Lock(&(reader->naluBufferMutex));

        if (err != ARSTREAM_OK)
        {
            naluBuf->useCount--;
            ARSAL_PRINT(ARSAL_PRINT_WARNING, ARSTREAM_READER2_TAG, "Failed to resend NALU (%d)", err); //TODO: debug
        }
    }

    ARSAL_Mutex_Unlock(&(reader->naluBufferMutex));

    return ret;
}


ARSTREAM_Reader2_t* ARSTREAM_Reader2_New(ARSTREAM_Reader2_Config_t *config, uint8_t *naluBuffer, int naluBufferSize, void *custom, eARSTREAM_ERROR *error)
{
    ARSTREAM_Reader2_t *retReader = NULL;
    int streamMutexWasInit = 0;
    int monitoringMutexWasInit = 0;
    int naluBufferMutexWasInit = 0;
    eARSTREAM_ERROR internalError = ARSTREAM_OK;
    /* ARGS Check */
    if ((config == NULL) ||
        (config->recvPort <= 0) || 
        (config->naluCallback == NULL) ||
        (naluBuffer == NULL) ||
        (naluBufferSize == 0))
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
        if (config->recvAddr)
        {
            retReader->recvAddr = strndup(config->recvAddr, 16);
        }
        if (config->ifaceAddr)
        {
            retReader->ifaceAddr = strndup(config->ifaceAddr, 16);
        }
        retReader->recvPort = config->recvPort;
        retReader->maxPacketSize = (config->maxPacketSize > 0) ? config->maxPacketSize - sizeof(ARSTREAM_NetworkHeaders_DataHeader2_t) - ARSTREAM_NETWORK_UDP_HEADER_SIZE - ARSTREAM_NETWORK_IP_HEADER_SIZE : ARSTREAM_NETWORK_MAX_RTP_PAYLOAD_SIZE;
        retReader->insertStartCodes = config->insertStartCodes;
        retReader->naluCallback = config->naluCallback;
        retReader->custom = custom;
        retReader->currentNaluBufferSize = naluBufferSize;
        retReader->currentNaluBuffer = naluBuffer;
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
    if (internalError == ARSTREAM_OK)
    {
        int mutexInitRet = ARSAL_Mutex_Init(&(retReader->monitoringMutex));
        if (mutexInitRet != 0)
        {
            internalError = ARSTREAM_ERROR_ALLOC;
        }
        else
        {
            monitoringMutexWasInit = 1;
        }
    }
    if (internalError == ARSTREAM_OK)
    {
        int mutexInitRet = ARSAL_Mutex_Init(&(retReader->naluBufferMutex));
        if (mutexInitRet != 0)
        {
            internalError = ARSTREAM_ERROR_ALLOC;
        }
        else
        {
            naluBufferMutexWasInit = 1;
        }
    }

#ifdef ARSTREAM_READER2_DEBUG
    /* Setup debug */
    if (internalError == ARSTREAM_OK)
    {
        retReader->rdbg = ARSTREAM_Reader2Debug_New(1, 1, 1);
        if (!retReader->rdbg)
        {
            ARSAL_PRINT(ARSAL_PRINT_WARNING, ARSTREAM_READER2_TAG, "ARSTREAM_Reader2Debug_New() failed");
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
        if (monitoringMutexWasInit == 1)
        {
            ARSAL_Mutex_Destroy(&(retReader->monitoringMutex));
        }
        if (naluBufferMutexWasInit == 1)
        {
            ARSAL_Mutex_Destroy(&(retReader->naluBufferMutex));
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
    int i;

    if (reader != NULL)
    {
        ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARSTREAM_READER2_TAG, "Stopping reader...");
        ARSAL_Mutex_Lock(&(reader->streamMutex));
        reader->threadsShouldStop = 1;
        ARSAL_Mutex_Unlock(&(reader->streamMutex));

        for (i = 0; i < reader->resenderCount; i++)
        {
            ARSTREAM_Reader2_Resender_Stop(reader->resender[i]);
        }
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
        if (((*reader)->recvThreadStarted == 0) &&
            ((*reader)->sendThreadStarted == 0))
        {
            ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARSTREAM_READER2_TAG, "All threads stopped");
            canDelete = 1;
        }
        ARSAL_Mutex_Unlock(&((*reader)->streamMutex));

        if (canDelete == 1)
        {
            int i;
            for (i = 0; i < (*reader)->resenderCount; i++)
            {
                retVal = ARSTREAM_Reader2_Resender_Delete(&(*reader)->resender[i]);
                if (retVal != ARSTREAM_OK)
                {
                    ARSAL_PRINT(ARSAL_PRINT_ERROR, ARSTREAM_READER2_TAG, "ARSTREAM_Reader2_Resender_Delete failed (%d)", retVal);
                }
            }
            for (i = 0; i < (*reader)->naluBufferCount; i++)
            {
                ARSTREAM_Reader2_NaluBuffer_t *naluBuf = &(*reader)->naluBuffer[i];
                if (naluBuf->naluBuffer)
                {
                    free(naluBuf->naluBuffer);
                }
            }
#ifdef ARSTREAM_READER2_DEBUG
            ARSTREAM_Reader2Debug_Delete(&(*reader)->rdbg);
#endif
            ARSAL_Mutex_Destroy(&((*reader)->streamMutex));
            ARSAL_Mutex_Destroy(&((*reader)->monitoringMutex));
            ARSAL_Mutex_Destroy(&((*reader)->naluBufferMutex));
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
        ARSAL_PRINT(ARSAL_PRINT_WARNING, ARSTREAM_READER2_TAG, "Receive socket buffer size is %d bytes", size); //TODO: debug
    }

    return ret;
}


static int ARSTREAM_Reader2_Bind(ARSTREAM_Reader2_t *reader)
{
    int ret = 0;

    struct sockaddr_in recvSin;
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
        }
        reader->recvSocket = -1;
    }

    return ret;
}


static void ARSTREAM_Reader2_UpdateMonitoring(ARSTREAM_Reader2_t *reader, uint32_t timestamp, uint16_t seqNum, uint16_t markerBit, uint32_t bytes, uint64_t *receptionTs)
{
    uint64_t curTime;
    struct timespec t1;
    ARSAL_Time_GetTime(&t1);
    curTime = (uint64_t)t1.tv_sec * 1000000 + (uint64_t)t1.tv_nsec / 1000;

    ARSAL_Mutex_Lock(&(reader->monitoringMutex));

    if (reader->monitoringCount < ARSTREAM_READER2_MONITORING_MAX_POINTS)
    {
        reader->monitoringCount++;
    }
    reader->monitoringIndex = (reader->monitoringIndex + 1) % ARSTREAM_READER2_MONITORING_MAX_POINTS;
    reader->monitoringPoint[reader->monitoringIndex].bytes = bytes;
    reader->monitoringPoint[reader->monitoringIndex].timestamp = timestamp;
    reader->monitoringPoint[reader->monitoringIndex].seqNum = seqNum;
    reader->monitoringPoint[reader->monitoringIndex].markerBit = markerBit;
    reader->monitoringPoint[reader->monitoringIndex].recvTimestamp = curTime;

    ARSAL_Mutex_Unlock(&(reader->monitoringMutex));

    if (receptionTs)
    {
        *receptionTs = curTime;
    }

#ifdef ARSTREAM_READER2_DEBUG
    ARSTREAM_Reader2Debug_ProcessPacket(reader->rdbg, curTime, timestamp, seqNum, markerBit, bytes);
#endif
}


static int ARSTREAM_Reader2_ReadData(ARSTREAM_Reader2_t *reader, uint8_t *recvBuffer, int recvBufferSize, int *recvSize)
{
    int ret = 0;

    if ((!recvBuffer) || (!recvSize))
    {
        return -1;
    }

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
            ARSAL_PRINT(ARSAL_PRINT_WARNING, ARSTREAM_READER2_TAG, "Socket receive timeout");
            ret = -2;
            *recvSize = 0;
        }
    }

    return ret;
}


static int ARSTREAM_Reader2_CheckBufferSize(ARSTREAM_Reader2_t *reader, int payloadSize)
{
    int ret = 0;

    if (reader->currentNaluSize + payloadSize > reader->currentNaluBufferSize)
    {
        uint32_t nextNaluBufferSize = reader->currentNaluSize + payloadSize, dummy = 0;
        uint8_t *nextNaluBuffer = reader->naluCallback(ARSTREAM_READER2_CAUSE_NALU_BUFFER_TOO_SMALL, reader->currentNaluBuffer, 0, 0, 0, 0, 0, &nextNaluBufferSize, reader->custom);
        ret = -1;
        if ((nextNaluBufferSize > 0) && (nextNaluBufferSize >= reader->currentNaluSize + payloadSize))
        {
            memcpy(nextNaluBuffer, reader->currentNaluBuffer, reader->currentNaluSize);
            reader->naluCallback(ARSTREAM_READER2_CAUSE_NALU_COPY_COMPLETE, reader->currentNaluBuffer, 0, 0, 0, 0, 0, &dummy, reader->custom);
            ret = 0;
        }
        reader->currentNaluBuffer = nextNaluBuffer;
        reader->currentNaluBufferSize = nextNaluBufferSize;
    }

    return ret;
}


void* ARSTREAM_Reader2_RunRecvThread(void *ARSTREAM_Reader2_t_Param)
{
    ARSTREAM_Reader2_t *reader = (ARSTREAM_Reader2_t *)ARSTREAM_Reader2_t_Param;
    uint8_t *recvBuffer = NULL;
    int recvBufferSize;
    int recvSize, payloadSize;
    int fuPending = 0, currentAuSize = 0;
    ARSTREAM_NetworkHeaders_DataHeader2_t *header = NULL;
    uint32_t rtpTimestamp = 0;
    uint64_t currentTimestamp = 0, previousTimestamp = 0, receptionTs = 0;
    uint16_t currentFlags;
    int auStartSeqNum = -1, naluStartSeqNum = -1, previousSeqNum = -1, currentSeqNum, seqNumDelta;
    int gapsInSeqNum = 0, gapsInSeqNumAu = 0, uncertainAuChange = 0;
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

    ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARSTREAM_READER2_TAG, "Stream reader receiving thread running");
    ARSAL_Mutex_Lock(&(reader->streamMutex));
    reader->recvThreadStarted = 1;
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
            rtpTimestamp = ntohl(header->timestamp);
            currentTimestamp = (((uint64_t)rtpTimestamp * 1000) + 45) / 90; /* 90000 Hz clock to microseconds */
            //TODO: handle the timestamp 32 bits loopback
            if (reader->firstTimestamp == 0)
            {
                reader->firstTimestamp = rtpTimestamp;
            }
            currentSeqNum = (int)ntohs(header->seqNum);
            currentFlags = ntohs(header->flags);
            ARSTREAM_Reader2_UpdateMonitoring(reader, rtpTimestamp, currentSeqNum, (currentFlags & (1 << 7)) ? 1 : 0, (uint32_t)recvSize, &receptionTs);

            if (previousSeqNum != -1)
            {
                seqNumDelta = currentSeqNum - previousSeqNum;
                if (seqNumDelta < -32768)
                {
                    seqNumDelta += 65536; /* handle seqNum 16 bits loopback */
                }
                if (seqNumDelta > 0)
                {
                    gapsInSeqNum += seqNumDelta - 1;
                    gapsInSeqNumAu += seqNumDelta - 1;
                }
            }
            else
            {
                seqNumDelta = 1;
            }

            if (seqNumDelta > 0)
            {
                if ((previousTimestamp != 0) && (currentTimestamp != previousTimestamp))
                {
                    if (gapsInSeqNumAu)
                    {
                        /*ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARSTREAM_READER2_TAG, "Incomplete access unit before seqNum %d, size %d bytes (missing %d of %d packets)", 
                                    currentSeqNum, currentAuSize, gapsInSeqNumAu, currentSeqNum - auStartSeqNum + 1);*/
                    }
                    if ((currentAuSize != 0) || (gapsInSeqNum != 0))
                    {
                        uncertainAuChange = 1;
                    }
                    gapsInSeqNumAu = 0;
                    currentAuSize = 0;
                }

                if (currentAuSize == 0)
                {
                    auStartSeqNum = currentSeqNum;
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
                                //ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARSTREAM_READER2_TAG, "Incomplete FU-A packet before FU-A at seqNum %d ((fuPending) && (startBit))", currentSeqNum);
                            }
                            if (startBit)
                            {
                                fuPending = 1;
                                reader->currentNaluSize = 0;
                                naluStartSeqNum = currentSeqNum;
                            }
                            if (fuPending)
                            {
                                outputSize += (startBit) ? startCodeLength + 1 : 0;
                                if (!ARSTREAM_Reader2_CheckBufferSize(reader, outputSize))
                                {
                                    if ((startCodeLength > 0) && (startBit))
                                    {
                                        memcpy(reader->currentNaluBuffer + reader->currentNaluSize, &startCode, startCodeLength);
                                        reader->currentNaluSize += startCodeLength;
                                        currentAuSize += startCodeLength;
                                    }
                                    memcpy(reader->currentNaluBuffer + reader->currentNaluSize + ((startBit) ? 1 : 0), recvBuffer + sizeof(ARSTREAM_NetworkHeaders_DataHeader2_t) + 2, payloadSize - 2);
                                    if (startBit)
                                    {
                                        /* restore the NALU header byte */
                                        *(reader->currentNaluBuffer + reader->currentNaluSize) = (fuIndicator & 0xE0) | (fuHeader & 0x1F);
                                        reader->currentNaluSize++;
                                        currentAuSize++;
                                    }
                                    reader->currentNaluSize += payloadSize - 2;
                                    currentAuSize += payloadSize - 2;
                                    if (endBit)
                                    {
                                        int isFirst = 0, isLast = 0;
                                        if ((!uncertainAuChange) && (auStartSeqNum == currentSeqNum))
                                        {
                                            isFirst = 1;
                                        }
                                        if (currentFlags & (1 << 7))
                                        {
                                            isLast = 1;
                                        }
                                        /*ARSAL_PRINT(ARSAL_PRINT_ERROR, ARSTREAM_READER2_TAG, "Output FU-A NALU (seqNum %d->%d) isFirst=%d isLast=%d gapsInSeqNum=%d",
                                                    naluStartSeqNum, currentSeqNum, isFirst, isLast, gapsInSeqNum);*/ //TODO debug
#ifdef ARSTREAM_READER2_DEBUG
                                        ARSTREAM_Reader2Debug_ProcessNalu(reader->rdbg, reader->currentNaluBuffer, reader->currentNaluSize, currentTimestamp, receptionTs,
                                                                          gapsInSeqNum, currentSeqNum - naluStartSeqNum + 1, isFirst, isLast);
#endif
                                        if (reader->resenderCount > 0)
                                        {
                                            int resendRet = ARSTREAM_Reader2_ResendNalu(reader, reader->currentNaluBuffer, reader->currentNaluSize, currentTimestamp, isLast);
                                            if (resendRet != 0)
                                            {
                                                //ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARSTREAM_READER2_TAG, "Failed to resend NALU (%d)", resendRet); //TODO: debug
                                            }
                                        }
                                        reader->currentNaluBuffer = reader->naluCallback(ARSTREAM_READER2_CAUSE_NALU_COMPLETE, reader->currentNaluBuffer, reader->currentNaluSize, currentTimestamp,
                                                                                         isFirst, isLast, gapsInSeqNum, &(reader->currentNaluBufferSize), reader->custom);
                                        gapsInSeqNum = 0;
                                    }
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
                    else if ((headByte & 0x1F) == ARSTREAM_NETWORK_HEADERS2_NALU_TYPE_STAPA)
                    {
                        /* Aggregation (STAP-A) */
                        if (fuPending)
                        {
                            //TODO: drop the previous incomplete FU-A?
                            fuPending = 0;
                            //ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARSTREAM_READER2_TAG, "Incomplete FU-A packet before STAP-A at seqNum %d (fuPending)", currentSeqNum);
                        }

//ARSAL_PRINT(ARSAL_PRINT_ERROR, ARSTREAM_READER2_TAG, "STAP-A at seqNum %d, payloadSize=%d", currentSeqNum, payloadSize);
                        if (payloadSize >= 3)
                        {
                            uint8_t *curBuf = recvBuffer + sizeof(ARSTREAM_NetworkHeaders_DataHeader2_t) + 1;
                            int sizeLeft = payloadSize - 1, naluCount = 0;
                            uint16_t naluSize = ((uint16_t)(*curBuf) << 8) | ((uint16_t)(*(curBuf + 1))), nextNaluSize = 0;
                            curBuf += 2;
                            sizeLeft -= 2;
                            while ((naluSize > 0) && (sizeLeft >= naluSize))
                            {
                                naluCount++;
                                nextNaluSize = (sizeLeft >= naluSize + 2) ? ((uint16_t)(*(curBuf + naluSize)) << 8) | ((uint16_t)(*(curBuf + naluSize + 1))) : 0;
//ARSAL_PRINT(ARSAL_PRINT_ERROR, ARSTREAM_READER2_TAG, "-- STAP-A at seqNum %d, naluSize=%d, sizeLeft=%d, nextNaluSize=%d", currentSeqNum, naluSize, sizeLeft, nextNaluSize);
                                reader->currentNaluSize = 0;
                                if (!ARSTREAM_Reader2_CheckBufferSize(reader, naluSize + startCodeLength))
                                {
                                    int isFirst = 0, isLast = 0;
                                    if ((!uncertainAuChange) && (auStartSeqNum == currentSeqNum) && (naluCount == 1))
                                    {
                                        isFirst = 1;
                                    }
                                    if ((currentFlags & (1 << 7)) && (nextNaluSize == 0))
                                    {
                                        isLast = 1;
                                    }
                                    if (startCodeLength > 0)
                                    {
                                        memcpy(reader->currentNaluBuffer, &startCode, startCodeLength);
                                        reader->currentNaluSize += startCodeLength;
                                        currentAuSize += startCodeLength;
                                    }
                                    memcpy(reader->currentNaluBuffer + reader->currentNaluSize, curBuf, naluSize);
                                    reader->currentNaluSize += naluSize;
                                    currentAuSize += naluSize;
                                    /*ARSAL_PRINT(ARSAL_PRINT_ERROR, ARSTREAM_READER2_TAG, "Output STAP-A NALU (seqNum %d) isFirst=%d isLast=%d gapsInSeqNum=%d",
                                                currentSeqNum, isFirst, isLast, gapsInSeqNum);*/ //TODO debug
#ifdef ARSTREAM_READER2_DEBUG
                                    ARSTREAM_Reader2Debug_ProcessNalu(reader->rdbg, reader->currentNaluBuffer, reader->currentNaluSize, currentTimestamp, receptionTs,
                                                                      gapsInSeqNum, 1, isFirst, isLast);
#endif
                                    if (reader->resenderCount > 0)
                                    {
                                        int resendRet = ARSTREAM_Reader2_ResendNalu(reader, reader->currentNaluBuffer, reader->currentNaluSize, currentTimestamp, isLast);
                                        if (resendRet != 0)
                                        {
                                            //ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARSTREAM_READER2_TAG, "Failed to resend NALU (%d)", resendRet); //TODO debug
                                        }
                                    }
                                    reader->currentNaluBuffer = reader->naluCallback(ARSTREAM_READER2_CAUSE_NALU_COMPLETE, reader->currentNaluBuffer, reader->currentNaluSize, currentTimestamp,
                                                                                     isFirst, isLast, gapsInSeqNum, &(reader->currentNaluBufferSize), reader->custom);
                                    gapsInSeqNum = 0;
                                }
                                else
                                {
                                    ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARSTREAM_READER2_TAG, "Failed to realloc auBuffer for output size (%d) for single NALU packet at seqNum %d", payloadSize + startCodeLength, currentSeqNum);
                                }
                                curBuf += naluSize;
                                sizeLeft -= naluSize;
                                curBuf += 2;
                                sizeLeft -= 2;
                                naluSize = nextNaluSize;
                            }
                        }
                    }
                    else
                    {
                        /* Single NAL unit */
                        if (fuPending)
                        {
                            //TODO: drop the previous incomplete FU-A?
                            fuPending = 0;
                            //ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARSTREAM_READER2_TAG, "Incomplete FU-A packet before single NALU at seqNum %d (fuPending)", currentSeqNum);
                        }

                        reader->currentNaluSize = 0;
                        if (!ARSTREAM_Reader2_CheckBufferSize(reader, payloadSize + startCodeLength))
                        {
                            int isFirst = 0, isLast = 0;
                            if ((!uncertainAuChange) && (auStartSeqNum == currentSeqNum))
                            {
                                isFirst = 1;
                            }
                            if (currentFlags & (1 << 7))
                            {
                                isLast = 1;
                            }
                            if (startCodeLength > 0)
                            {
                                memcpy(reader->currentNaluBuffer, &startCode, startCodeLength);
                                reader->currentNaluSize += startCodeLength;
                                currentAuSize += startCodeLength;
                            }
                            memcpy(reader->currentNaluBuffer + reader->currentNaluSize, recvBuffer + sizeof(ARSTREAM_NetworkHeaders_DataHeader2_t), payloadSize);
                            reader->currentNaluSize += payloadSize;
                            currentAuSize += payloadSize;
                            /*ARSAL_PRINT(ARSAL_PRINT_ERROR, ARSTREAM_READER2_TAG, "Output single NALU (seqNum %d) isFirst=%d isLast=%d gapsInSeqNum=%d",
                                        currentSeqNum, isFirst, isLast, gapsInSeqNum);*/ //TODO debug
#ifdef ARSTREAM_READER2_DEBUG
                            ARSTREAM_Reader2Debug_ProcessNalu(reader->rdbg, reader->currentNaluBuffer, reader->currentNaluSize, currentTimestamp, receptionTs,
                                                              gapsInSeqNum, 1, isFirst, isLast);
#endif
                            if (reader->resenderCount > 0)
                            {
                                int resendRet = ARSTREAM_Reader2_ResendNalu(reader, reader->currentNaluBuffer, reader->currentNaluSize, currentTimestamp, isLast);
                                if (resendRet != 0)
                                {
                                    //ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARSTREAM_READER2_TAG, "Failed to resend NALU (%d)", resendRet); //TODO debug
                                }
                            }
                            reader->currentNaluBuffer = reader->naluCallback(ARSTREAM_READER2_CAUSE_NALU_COMPLETE, reader->currentNaluBuffer, reader->currentNaluSize, currentTimestamp,
                                                                             isFirst, isLast, gapsInSeqNum, &(reader->currentNaluBufferSize), reader->custom);
                            gapsInSeqNum = 0;
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
                    /* the marker bit is set: complete access unit */
                    //ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARSTREAM_READER2_TAG, "Complete access unit at seqNum %d, size %d bytes (missing %d of %d packets)", currentSeqNum, currentAuSize, gapsInSeqNumAu, currentSeqNum - auStartSeqNum + 1);
                    uncertainAuChange = 0;
                    gapsInSeqNumAu = 0;
                    currentAuSize = 0;
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

    reader->naluCallback(ARSTREAM_READER2_CAUSE_CANCEL, reader->currentNaluBuffer, 0, 0, 0, 0, 0, &(reader->currentNaluBufferSize), reader->custom);

    ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARSTREAM_READER2_TAG, "Stream reader receiving thread ended");
    ARSAL_Mutex_Lock(&(reader->streamMutex));
    reader->recvThreadStarted = 0;
    ARSAL_Mutex_Unlock(&(reader->streamMutex));

    if (recvBuffer)
    {
        free(recvBuffer);
        recvBuffer = NULL;
    }

    return (void *)0;
}


void* ARSTREAM_Reader2_RunSendThread(void *ARSTREAM_Reader2_t_Param)
{
    ARSTREAM_Reader2_t *reader = (ARSTREAM_Reader2_t *)ARSTREAM_Reader2_t_Param;

    ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARSTREAM_READER2_TAG, "Stream reader sending thread running");
    ARSAL_Mutex_Lock(&(reader->streamMutex));
    reader->sendThreadStarted = 1;
    ARSAL_Mutex_Unlock(&(reader->streamMutex));

    ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARSTREAM_READER2_TAG, "Stream reader sending thread ended");
    ARSAL_Mutex_Lock(&(reader->streamMutex));
    reader->sendThreadStarted = 0;
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


eARSTREAM_ERROR ARSTREAM_Reader2_GetMonitoring(ARSTREAM_Reader2_t *reader, uint64_t startTime, uint32_t timeIntervalUs, uint32_t *realTimeIntervalUs, uint32_t *receptionTimeJitter,
                                               uint32_t *bytesReceived, uint32_t *meanPacketSize, uint32_t *packetSizeStdDev, uint32_t *packetsReceived, uint32_t *packetsMissed)
{
    eARSTREAM_ERROR ret = ARSTREAM_OK;
    uint64_t endTime, curTime, previousTime, auTimestamp, receptionTimeSum = 0, receptionTimeVarSum = 0, packetSizeVarSum = 0;
    uint32_t bytes, bytesSum = 0, _meanPacketSize = 0, receptionTime = 0, meanReceptionTime = 0, _receptionTimeJitter = 0, _packetSizeStdDev = 0;
    int currentSeqNum, previousSeqNum = -1, seqNumDelta, gapsInSeqNum;
    int points = 0, usefulPoints = 0, idx, i, firstUsefulIdx = -1;

    if ((reader == NULL) || (timeIntervalUs == 0))
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

    ARSAL_Mutex_Lock(&(reader->monitoringMutex));

    if (reader->monitoringCount > 0)
    {
        idx = reader->monitoringIndex;
        previousTime = startTime;

        while (points < reader->monitoringCount)
        {
            curTime = reader->monitoringPoint[idx].recvTimestamp;
            if (curTime > startTime)
            {
                points++;
                idx = (idx - 1 >= 0) ? idx - 1 : ARSTREAM_READER2_MONITORING_MAX_POINTS - 1;
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
            idx = (idx - 1 >= 0) ? idx - 1 : ARSTREAM_READER2_MONITORING_MAX_POINTS - 1;
            curTime = reader->monitoringPoint[idx].recvTimestamp;
            bytes = reader->monitoringPoint[idx].bytes;
            bytesSum += bytes;
            auTimestamp = (((uint64_t)(reader->monitoringPoint[idx].timestamp - reader->firstTimestamp) * 1000) + 45) / 90; /* 90000 Hz clock to microseconds */
            receptionTime = curTime - auTimestamp;
            receptionTimeSum += receptionTime;
            currentSeqNum = reader->monitoringPoint[idx].seqNum;
            seqNumDelta = (previousSeqNum != -1) ? (previousSeqNum - currentSeqNum) : 1;
            if (seqNumDelta < -32768) seqNumDelta += 65536; /* handle seqNum 16 bits loopback */
            gapsInSeqNum += seqNumDelta - 1;
            previousSeqNum = currentSeqNum;
            previousTime = curTime;
            usefulPoints++;
            points++;
            idx = (idx - 1 >= 0) ? idx - 1 : ARSTREAM_READER2_MONITORING_MAX_POINTS - 1;
        }

        endTime = previousTime;
        _meanPacketSize = (usefulPoints) ? (bytesSum / usefulPoints) : 0;
        meanReceptionTime = (usefulPoints) ? (uint32_t)(receptionTimeSum / usefulPoints) : 0;

        if ((receptionTimeJitter) || (packetSizeStdDev))
        {
            for (i = 0, idx = firstUsefulIdx; i < usefulPoints; i++)
            {
                idx = (idx - 1 >= 0) ? idx - 1 : ARSTREAM_READER2_MONITORING_MAX_POINTS - 1;
                curTime = reader->monitoringPoint[idx].recvTimestamp;
                bytes = reader->monitoringPoint[idx].bytes;
                auTimestamp = (((uint64_t)(reader->monitoringPoint[idx].timestamp - reader->firstTimestamp) * 1000) + 45) / 90; /* 90000 Hz clock to microseconds */
                receptionTime = curTime - auTimestamp;
                packetSizeVarSum += ((bytes - _meanPacketSize) * (bytes - _meanPacketSize));
                receptionTimeVarSum += ((receptionTime - meanReceptionTime) * (receptionTime - meanReceptionTime));
            }
            _receptionTimeJitter = (usefulPoints) ? (uint32_t)(sqrt((double)receptionTimeVarSum / usefulPoints)) : 0;
            _packetSizeStdDev = (usefulPoints) ? (uint32_t)(sqrt((double)packetSizeVarSum / usefulPoints)) : 0;
        }
    }

    ARSAL_Mutex_Unlock(&(reader->monitoringMutex));

    if (realTimeIntervalUs)
    {
        *realTimeIntervalUs = (startTime - endTime);
    }
    if (receptionTimeJitter)
    {
        *receptionTimeJitter = _receptionTimeJitter;
    }
    if (bytesReceived)
    {
        *bytesReceived = bytesSum;
    }
    if (meanPacketSize)
    {
        *meanPacketSize = _meanPacketSize;
    }
    if (packetSizeStdDev)
    {
        *packetSizeStdDev = _packetSizeStdDev;
    }
    if (packetsReceived)
    {
        *packetsReceived = usefulPoints;
    }
    if (packetsMissed)
    {
        *packetsMissed = gapsInSeqNum;
    }

    return ret;
}


ARSTREAM_Reader2_Resender_t* ARSTREAM_Reader2_Resender_New(ARSTREAM_Reader2_t *reader, ARSTREAM_Reader2_Resender_Config_t *config, eARSTREAM_ERROR *error)
{
    ARSTREAM_Reader2_Resender_t *retResender = NULL;
    int streamMutexWasInit = 0;
    int monitoringMutexWasInit = 0;
    eARSTREAM_ERROR internalError = ARSTREAM_OK;
    /* ARGS Check */
    if ((reader == NULL) ||
        (config == NULL) ||
        (reader->resenderCount >= ARSTREAM_READER2_RESENDER_MAX_COUNT))
    {
        SET_WITH_CHECK(error, ARSTREAM_ERROR_BAD_PARAMETERS);
        return retResender;
    }

    /* Alloc new resender */
    retResender = malloc(sizeof(ARSTREAM_Reader2_Resender_t));
    if (retResender == NULL)
    {
        internalError = ARSTREAM_ERROR_ALLOC;
    }

    /* Initialize the resender and copy parameters */
    if (internalError == ARSTREAM_OK)
    {
        memset(retResender, 0, sizeof(ARSTREAM_Reader2_Resender_t));
        retResender->reader = reader;
    }

    /* Setup ARStream_Sender2 */
    if (internalError == ARSTREAM_OK)
    {
        eARSTREAM_ERROR error2;
        ARSTREAM_Sender2_Config_t senderConfig;
        memset(&senderConfig, 0, sizeof(senderConfig));
        senderConfig.ifaceAddr = config->ifaceAddr;
        senderConfig.sendAddr = config->sendAddr;
        senderConfig.sendPort = config->sendPort;
        senderConfig.naluCallback = ARSTREAM_Reader2_Resender_NaluCallback;
        senderConfig.naluFifoSize = ARSTREAM_READER2_RESENDER_MAX_NALU_BUFFER_COUNT;
        senderConfig.maxPacketSize = config->maxPacketSize;
        senderConfig.targetPacketSize = config->targetPacketSize;
        senderConfig.maxBitrate = config->maxBitrate;
        senderConfig.maxLatencyMs = config->maxLatencyMs;
        senderConfig.maxNetworkLatencyMs = config->maxNetworkLatencyMs;
        retResender->sender = ARSTREAM_Sender2_New(&senderConfig, (void*)retResender, &error2);
        if (error2 != ARSTREAM_OK)
        {
            internalError = error2;
        }
    }

    if (internalError == ARSTREAM_OK)
    {
        retResender->senderRunning = 1;
        reader->resender[reader->resenderCount] = retResender;
        reader->resenderCount++;
    }

    if ((internalError != ARSTREAM_OK) &&
        (retResender != NULL))
    {
        free(retResender);
        retResender = NULL;
    }

    SET_WITH_CHECK(error, internalError);
    return retResender;
}


void ARSTREAM_Reader2_Resender_Stop(ARSTREAM_Reader2_Resender_t *resender)
{
    if ((resender != NULL) && (resender->sender != NULL))
    {
        ARSTREAM_Sender2_StopSender(resender->sender);
        resender->senderRunning = 0;
    }
}


eARSTREAM_ERROR ARSTREAM_Reader2_Resender_Delete(ARSTREAM_Reader2_Resender_t **resender)
{
    int i, idx;
    eARSTREAM_ERROR retVal = ARSTREAM_ERROR_BAD_PARAMETERS;
    if ((resender != NULL) &&
        (*resender != NULL)&&
        ((*resender)->reader != NULL))
    {
        for (i = 0, idx = -1; i < (*resender)->reader->resenderCount; i++)
        {
            if (*resender == (*resender)->reader->resender[i])
            {
                idx = i;
                break;
            }
        }
        if (idx < 0)
        {
            return retVal;
        }

        if (!(*resender)->senderRunning)
        {
            retVal = ARSTREAM_Sender2_Delete(&((*resender)->sender));
            if (retVal == ARSTREAM_OK)
            {
                (*resender)->reader->resender[idx] = NULL;
                for (i = idx; i < (*resender)->reader->resenderCount; i++)
                {
                    (*resender)->reader->resender[i] = (*resender)->reader->resender[i + 1];
                }
                (*resender)->reader->resenderCount--;

                free(*resender);
                *resender = NULL;
            }
            else
            {
                ARSAL_PRINT(ARSAL_PRINT_ERROR, ARSTREAM_READER2_TAG, "Failed to delete Sender2");
            }
        }
        else
        {
            ARSAL_PRINT(ARSAL_PRINT_ERROR, ARSTREAM_READER2_TAG, "Sender2 is still running");
        }
    }
    return retVal;
}


void* ARSTREAM_Reader2_Resender_RunSendThread (void *ARSTREAM_Reader2_Resender_t_Param)
{
    ARSTREAM_Reader2_Resender_t *resender = (ARSTREAM_Reader2_Resender_t *)ARSTREAM_Reader2_Resender_t_Param;

    if (resender == NULL)
    {
        return (void *)0;
    }

    return ARSTREAM_Sender2_RunSendThread((void *)resender->sender);
}


void* ARSTREAM_Reader2_Resender_RunRecvThread (void *ARSTREAM_Reader2_Resender_t_Param)
{
    ARSTREAM_Reader2_Resender_t *resender = (ARSTREAM_Reader2_Resender_t *)ARSTREAM_Reader2_Resender_t_Param;

    if (resender == NULL)
    {
        return (void *)0;
    }

    return ARSTREAM_Sender2_RunRecvThread((void *)resender->sender);
}

