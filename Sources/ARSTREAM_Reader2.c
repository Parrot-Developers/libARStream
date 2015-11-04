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
 * @brief Stream reader on a network (v2)
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
#include <poll.h>
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

#define ARSTREAM_READER2_STREAM_DATAREAD_TIMEOUT_MS (500)
#define ARSTREAM_READER2_CLOCKSYNC_DATAREAD_TIMEOUT_MS (100)
#define ARSTREAM_READER2_CLOCKSYNC_PERIOD_MS (200) // 5 Hz

#define ARSTREAM_READER2_RESENDER_MAX_COUNT (4)
#define ARSTREAM_READER2_RESENDER_MAX_NALU_BUFFER_COUNT (1024) //TODO: tune this value
#define ARSTREAM_READER2_RESENDER_NALU_BUFFER_MALLOC_CHUNK_SIZE (4096)

#define ARSTREAM_READER2_MONITORING_MAX_POINTS (2048)

#define ARSTREAM_H264_STARTCODE 0x00000001
#define ARSTREAM_H264_STARTCODE_LENGTH 4

#define ARSTREAM_READER2_MONITORING_OUTPUT
#ifdef ARSTREAM_READER2_MONITORING_OUTPUT
    #include <stdio.h>

    #define ARSTREAM_READER2_MONITORING_OUTPUT_ALLOW_DRONE
    #define ARSTREAM_READER2_MONITORING_OUTPUT_PATH_DRONE "/data/ftp/internal_000/streamdebug"
    #define ARSTREAM_READER2_MONITORING_OUTPUT_ALLOW_NAP_USB
    #define ARSTREAM_READER2_MONITORING_OUTPUT_PATH_NAP_USB "/tmp/mnt/STREAMDEBUG/streamdebug"
    //#define ARSTREAM_READER2_MONITORING_OUTPUT_ALLOW_NAP_INTERNAL
    #define ARSTREAM_READER2_MONITORING_OUTPUT_PATH_NAP_INTERNAL "/data/skycontroller/streamdebug"
    #define ARSTREAM_READER2_MONITORING_OUTPUT_ALLOW_ANDROID_INTERNAL
    #define ARSTREAM_READER2_MONITORING_OUTPUT_PATH_ANDROID_INTERNAL "/sdcard/FF/streamdebug"
    #define ARSTREAM_READER2_MONITORING_OUTPUT_ALLOW_PCLINUX
    #define ARSTREAM_READER2_MONITORING_OUTPUT_PATH_PCLINUX "./streamdebug"

    #define ARSTREAM_READER2_MONITORING_OUTPUT_FILENAME "reader_monitor"
#endif

//#define ARSTREAM_READER2_CLOCKSYNC_DEBUG_FILE
#ifdef ARSTREAM_READER2_CLOCKSYNC_DEBUG_FILE
    #include <locale.h>
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
    uint64_t timestampShifted;
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
    char *serverAddr;
    char *mcastAddr;
    char *mcastIfaceAddr;
    int serverStreamPort;
    int serverControlPort;
    int clientStreamPort;
    int clientControlPort;
    ARSTREAM_Reader2_NaluCallback_t naluCallback;
    void *naluCallbackUserPtr;
    int maxPacketSize;
    int maxBitrate;
    int maxLatencyMs;
    int maxNetworkLatencyMs;
    int insertStartCodes;

    /* Current frame storage */
    int currentNaluBufferSize; // Usable length of the buffer
    int currentNaluSize;       // Actual data length
    uint8_t *currentNaluBuffer;
    uint64_t clockDelta;
    int scheduleNaluBufferChange;

    /* Thread status */
    ARSAL_Mutex_t streamMutex;
    ARSAL_Cond_t streamCond;
    int threadsShouldStop;
    int streamThreadStarted;
    int controlThreadStarted;

    /* Sockets */
    int isMulticast;
    int streamSocket;
    int controlSocket;

    /* Monitoring */
    ARSAL_Mutex_t monitoringMutex;
    int monitoringCount;
    int monitoringIndex;
    ARSTREAM_Reader2_MonitoringPoint_t monitoringPoint[ARSTREAM_READER2_MONITORING_MAX_POINTS];

    /* Resenders */
    ARSTREAM_Reader2_Resender_t *resender[ARSTREAM_READER2_RESENDER_MAX_COUNT];
    int resenderCount;
    ARSAL_Mutex_t resenderMutex;
    ARSAL_Mutex_t naluBufferMutex;
    ARSTREAM_Reader2_NaluBuffer_t naluBuffer[ARSTREAM_READER2_RESENDER_MAX_NALU_BUFFER_COUNT];
    int naluBufferCount;

#ifdef ARSTREAM_READER2_MONITORING_OUTPUT
    FILE* fMonitorOut;
#endif
};


static void ARSTREAM_Reader2_Resender_NaluCallback(eARSTREAM_SENDER2_STATUS status, void *naluUserPtr, void *userPtr)
{
    ARSTREAM_Reader2_Resender_t *resender = (ARSTREAM_Reader2_Resender_t *)userPtr;
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
                    ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARSTREAM_READER2_TAG, "Reallocated NALU buffer (size: %d) - naluBufferCount = %d", newSize, reader->naluBufferCount);
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
            ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARSTREAM_READER2_TAG, "Allocated new NALU buffer (size: %d) - naluBufferCount = %d", newSize, reader->naluBufferCount);
            naluBuf->naluBufferSize = newSize;
            retNaluBuf = naluBuf;
        }
    }

    return retNaluBuf;
}


static int ARSTREAM_Reader2_ResendNalu(ARSTREAM_Reader2_t *reader, uint8_t *naluBuffer, uint32_t naluSize, uint64_t auTimestamp, int isLastNaluInAu, int missingPacketsBefore)
{
    int ret = 0, i;
    ARSTREAM_Reader2_NaluBuffer_t *naluBuf = NULL;
    ARSTREAM_Sender2_NaluDesc_t nalu;

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
        ARSAL_PRINT(ARSAL_PRINT_WARNING, ARSTREAM_READER2_TAG, "Failed to get available NALU buffer (naluSize=%d, naluBufferCount=%d)", naluSize, reader->naluBufferCount);
        return -1;
    }
    naluBuf->useCount++;
    ARSAL_Mutex_Unlock(&(reader->naluBufferMutex));

    memcpy(naluBuf->naluBuffer, naluBuffer, naluSize);
    naluBuf->naluSize = naluSize;
    naluBuf->auTimestamp = auTimestamp;
    naluBuf->isLastNaluInAu = isLastNaluInAu;

    nalu.naluBuffer = naluBuf->naluBuffer;
    nalu.naluSize = naluSize;
    nalu.auTimestamp = auTimestamp;
    nalu.isLastNaluInAu = isLastNaluInAu;
    nalu.seqNumForcedDiscontinuity = missingPacketsBefore;
    nalu.auUserPtr = NULL;
    nalu.naluUserPtr = naluBuf;

    /* send the NALU to all resenders */
    ARSAL_Mutex_Lock(&(reader->resenderMutex));
    for (i = 0; i < reader->resenderCount; i++)
    {
        ARSTREAM_Reader2_Resender_t *resender = reader->resender[i];
        eARSTREAM_ERROR err;

        if ((resender) && (resender->senderRunning))
        {
            err = ARSTREAM_Sender2_SendNewNalu(resender->sender, &nalu);
            if (err == ARSTREAM_OK)
            {
                ARSAL_Mutex_Lock(&(reader->naluBufferMutex));
                naluBuf->useCount++;
                ARSAL_Mutex_Unlock(&(reader->naluBufferMutex));
            }
            else
            {
                ARSAL_PRINT(ARSAL_PRINT_WARNING, ARSTREAM_READER2_TAG, "Failed to resend NALU on resender #%d (error %d)", i, err);
            }
        }
    }
    ARSAL_Mutex_Unlock(&(reader->resenderMutex));

    ARSAL_Mutex_Lock(&(reader->naluBufferMutex));
    naluBuf->useCount--;
    ARSAL_Mutex_Unlock(&(reader->naluBufferMutex));

    return ret;
}


ARSTREAM_Reader2_t* ARSTREAM_Reader2_New(ARSTREAM_Reader2_Config_t *config, eARSTREAM_ERROR *error)
{
    ARSTREAM_Reader2_t *retReader = NULL;
    int streamMutexWasInit = 0, streamCondWasInit = 0;
    int monitoringMutexWasInit = 0;
    int resenderMutexWasInit = 0;
    int naluBufferMutexWasInit = 0;
    eARSTREAM_ERROR internalError = ARSTREAM_OK;

    /* ARGS Check */
    if (config == NULL)
    {
        ARSAL_PRINT(ARSAL_PRINT_ERROR, ARSTREAM_READER2_TAG, "No config provided");
        SET_WITH_CHECK(error, ARSTREAM_ERROR_BAD_PARAMETERS);
        return retReader;
    }
    if ((config->serverAddr == NULL) || (!strlen(config->serverAddr)))
    {
        ARSAL_PRINT(ARSAL_PRINT_ERROR, ARSTREAM_READER2_TAG, "Config: no server address provided");
        SET_WITH_CHECK(error, ARSTREAM_ERROR_BAD_PARAMETERS);
        return retReader;
    }
    if ((config->serverStreamPort <= 0) || (config->serverControlPort <= 0))
    {
        ARSAL_PRINT(ARSAL_PRINT_ERROR, ARSTREAM_READER2_TAG, "Config: no server ports provided");
        SET_WITH_CHECK(error, ARSTREAM_ERROR_BAD_PARAMETERS);
        return retReader;
    }
    if (config->naluCallback == NULL)
    {
        ARSAL_PRINT(ARSAL_PRINT_ERROR, ARSTREAM_READER2_TAG, "Config: no NAL unit callback provided");
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
        retReader->isMulticast = 0;
        retReader->streamSocket = -1;
        retReader->controlSocket = -1;

        if (config->serverAddr)
        {
            retReader->serverAddr = strndup(config->serverAddr, 16);
        }
        if (config->mcastAddr)
        {
            retReader->mcastAddr = strndup(config->mcastAddr, 16);
        }
        if (config->mcastIfaceAddr)
        {
            retReader->mcastIfaceAddr = strndup(config->mcastIfaceAddr, 16);
        }
        retReader->serverStreamPort = config->serverStreamPort;
        retReader->serverControlPort = config->serverControlPort;
        retReader->clientStreamPort = (config->clientStreamPort > 0) ? config->clientStreamPort : ARSTREAM_READER2_DEFAULT_CLIENT_STREAM_PORT;
        retReader->clientControlPort = (config->clientControlPort > 0) ? config->clientControlPort : ARSTREAM_READER2_DEFAULT_CLIENT_CONTROL_PORT;
        retReader->naluCallback = config->naluCallback;
        retReader->naluCallbackUserPtr = config->naluCallbackUserPtr;
        retReader->maxPacketSize = (config->maxPacketSize > 0) ? config->maxPacketSize - sizeof(ARSTREAM_NetworkHeaders_DataHeader2_t) - ARSTREAM_NETWORK_UDP_HEADER_SIZE - ARSTREAM_NETWORK_IP_HEADER_SIZE : ARSTREAM_NETWORK_MAX_RTP_PAYLOAD_SIZE;
        retReader->maxBitrate = (config->maxBitrate > 0) ? config->maxBitrate : 0;
        retReader->maxLatencyMs = (config->maxLatencyMs > 0) ? config->maxLatencyMs : 0;
        retReader->maxNetworkLatencyMs = (config->maxNetworkLatencyMs > 0) ? config->maxNetworkLatencyMs : 0;
        retReader->insertStartCodes = (config->insertStartCodes > 0) ? 1 : 0;
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
        int condInitRet = ARSAL_Cond_Init(&(retReader->streamCond));
        if (condInitRet != 0)
        {
            internalError = ARSTREAM_ERROR_ALLOC;
        }
        else
        {
            streamCondWasInit = 1;
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
        int mutexInitRet = ARSAL_Mutex_Init(&(retReader->resenderMutex));
        if (mutexInitRet != 0)
        {
            internalError = ARSTREAM_ERROR_ALLOC;
        }
        else
        {
            resenderMutexWasInit = 1;
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

#ifdef ARSTREAM_READER2_MONITORING_OUTPUT
    if (internalError == ARSTREAM_OK)
    {
        int i;
        char szOutputFileName[128];
        char *pszFilePath = NULL;
        szOutputFileName[0] = '\0';
        if (0)
        {
        }
#ifdef ARSTREAM_READER2_MONITORING_OUTPUT_ALLOW_DRONE
        else if ((access(ARSTREAM_READER2_MONITORING_OUTPUT_PATH_DRONE, F_OK) == 0) && (access(ARSTREAM_READER2_MONITORING_OUTPUT_PATH_DRONE, W_OK) == 0))
        {
            pszFilePath = ARSTREAM_READER2_MONITORING_OUTPUT_PATH_DRONE;
        }
#endif
#ifdef ARSTREAM_READER2_MONITORING_OUTPUT_ALLOW_NAP_USB
        else if ((access(ARSTREAM_READER2_MONITORING_OUTPUT_PATH_NAP_USB, F_OK) == 0) && (access(ARSTREAM_READER2_MONITORING_OUTPUT_PATH_NAP_USB, W_OK) == 0))
        {
            pszFilePath = ARSTREAM_READER2_MONITORING_OUTPUT_PATH_NAP_USB;
        }
#endif
#ifdef ARSTREAM_READER2_MONITORING_OUTPUT_ALLOW_NAP_INTERNAL
        else if ((access(ARSTREAM_READER2_MONITORING_OUTPUT_PATH_NAP_INTERNAL, F_OK) == 0) && (access(ARSTREAM_READER2_MONITORING_OUTPUT_PATH_NAP_INTERNAL, W_OK) == 0))
        {
            pszFilePath = ARSTREAM_READER2_MONITORING_OUTPUT_PATH_NAP_INTERNAL;
        }
#endif
#ifdef ARSTREAM_READER2_MONITORING_OUTPUT_ALLOW_ANDROID_INTERNAL
        else if ((access(ARSTREAM_READER2_MONITORING_OUTPUT_PATH_ANDROID_INTERNAL, F_OK) == 0) && (access(ARSTREAM_READER2_MONITORING_OUTPUT_PATH_ANDROID_INTERNAL, W_OK) == 0))
        {
            pszFilePath = ARSTREAM_READER2_MONITORING_OUTPUT_PATH_ANDROID_INTERNAL;
        }
#endif
#ifdef ARSTREAM_READER2_MONITORING_OUTPUT_ALLOW_PCLINUX
        else if ((access(ARSTREAM_READER2_MONITORING_OUTPUT_PATH_PCLINUX, F_OK) == 0) && (access(ARSTREAM_READER2_MONITORING_OUTPUT_PATH_PCLINUX, W_OK) == 0))
        {
            pszFilePath = ARSTREAM_READER2_MONITORING_OUTPUT_PATH_PCLINUX;
        }
#endif
        if (pszFilePath)
        {
            for (i = 0; i < 1000; i++)
            {
                snprintf(szOutputFileName, 128, "%s/%s_%03d.dat", pszFilePath, ARSTREAM_READER2_MONITORING_OUTPUT_FILENAME, i);
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
            retReader->fMonitorOut = fopen(szOutputFileName, "w");
            if (!retReader->fMonitorOut)
            {
                ARSAL_PRINT(ARSAL_PRINT_WARNING, ARSTREAM_READER2_TAG, "Unable to open monitor output file '%s'", szOutputFileName);
            }
        }

        if (retReader->fMonitorOut)
        {
            fprintf(retReader->fMonitorOut, "recvTimestamp rtpTimestamp rtpTimestampShifted rtpSeqNum rtpMarkerBit bytes\n");
        }
    }
#endif //#ifdef ARSTREAM_READER2_MONITORING_OUTPUT

    if ((internalError != ARSTREAM_OK) &&
        (retReader != NULL))
    {
        if (streamMutexWasInit == 1)
        {
            ARSAL_Mutex_Destroy(&(retReader->streamMutex));
        }
        if (streamCondWasInit == 1)
        {
            ARSAL_Cond_Destroy(&(retReader->streamCond));
        }
        if (monitoringMutexWasInit == 1)
        {
            ARSAL_Mutex_Destroy(&(retReader->monitoringMutex));
        }
        if (resenderMutexWasInit == 1)
        {
            ARSAL_Mutex_Destroy(&(retReader->resenderMutex));
        }
        if (naluBufferMutexWasInit == 1)
        {
            ARSAL_Mutex_Destroy(&(retReader->naluBufferMutex));
        }
        if ((retReader) && (retReader->serverAddr))
        {
            free(retReader->serverAddr);
        }
        if ((retReader) && (retReader->mcastAddr))
        {
            free(retReader->mcastAddr);
        }
        if ((retReader) && (retReader->mcastIfaceAddr))
        {
            free(retReader->mcastIfaceAddr);
        }
#ifdef ARSTREAM_READER2_MONITORING_OUTPUT
        if ((retReader) && (retReader->fMonitorOut))
        {
            fclose(retReader->fMonitorOut);
        }
#endif
        free(retReader);
        retReader = NULL;
    }

    SET_WITH_CHECK(error, internalError);
    return retReader;
}


void ARSTREAM_Reader2_InvalidateNaluBuffer(ARSTREAM_Reader2_t *reader)
{
    if (reader != NULL)
    {
        ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARSTREAM_READER2_TAG, "Invalidating NALU buffer...");
        ARSAL_Mutex_Lock(&(reader->streamMutex));
        if (reader->streamThreadStarted)
        {
            reader->scheduleNaluBufferChange = 1;
            ARSAL_Cond_Wait(&(reader->streamCond), &(reader->streamMutex));
        }
        ARSAL_Mutex_Unlock(&(reader->streamMutex));
        ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARSTREAM_READER2_TAG, "NALU buffer invalidated");
    }
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
        ARSAL_Cond_Signal(&(reader->streamCond));

        /* stop all resenders */
        ARSAL_Mutex_Lock(&(reader->resenderMutex));
        for (i = 0; i < reader->resenderCount; i++)
        {
            if (reader->resender[i] != NULL)
            {
                ARSTREAM_Sender2_StopSender(reader->resender[i]->sender);
                reader->resender[i]->senderRunning = 0;
            }
        }
        ARSAL_Mutex_Unlock(&(reader->resenderMutex));
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
        if (((*reader)->streamThreadStarted == 0) &&
            ((*reader)->controlThreadStarted == 0))
        {
            ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARSTREAM_READER2_TAG, "All threads stopped");
            canDelete = 1;
        }
        ARSAL_Mutex_Unlock(&((*reader)->streamMutex));

        if (canDelete == 1)
        {
            int i;

            /* delete all resenders */
            ARSAL_Mutex_Lock(&((*reader)->resenderMutex));
            for (i = 0; i < (*reader)->resenderCount; i++)
            {
                ARSTREAM_Reader2_Resender_t *resender = (*reader)->resender[i];
                if (resender == NULL)
                {
                    continue;
                }

                if (!resender->senderRunning)
                {
                    retVal = ARSTREAM_Sender2_Delete(&(resender->sender));
                    if (retVal == ARSTREAM_OK)
                    {
                        free(resender);
                        (*reader)->resender[i] = NULL;
                    }
                    else
                    {
                        ARSAL_PRINT(ARSAL_PRINT_ERROR, ARSTREAM_READER2_TAG, "Resender: failed to delete Sender2 (%d)", retVal);
                    }
                }
                else
                {
                    ARSAL_PRINT(ARSAL_PRINT_ERROR, ARSTREAM_READER2_TAG, "Resender #%d is still running", i);
                }
            }
            ARSAL_Mutex_Unlock(&((*reader)->resenderMutex));

            for (i = 0; i < (*reader)->naluBufferCount; i++)
            {
                ARSTREAM_Reader2_NaluBuffer_t *naluBuf = &(*reader)->naluBuffer[i];
                if (naluBuf->naluBuffer)
                {
                    free(naluBuf->naluBuffer);
                }
            }

#ifdef ARSTREAM_READER2_MONITORING_OUTPUT
            if ((*reader)->fMonitorOut)
            {
                fclose((*reader)->fMonitorOut);
            }
#endif

            ARSAL_Mutex_Destroy(&((*reader)->streamMutex));
            ARSAL_Cond_Destroy(&((*reader)->streamCond));
            ARSAL_Mutex_Destroy(&((*reader)->monitoringMutex));
            ARSAL_Mutex_Destroy(&((*reader)->resenderMutex));
            ARSAL_Mutex_Destroy(&((*reader)->naluBufferMutex));
            if ((*reader)->controlSocket != -1)
            {
                ARSAL_Socket_Close((*reader)->controlSocket);
                (*reader)->controlSocket = -1;
            }
            if ((*reader)->streamSocket != -1)
            {
                ARSAL_Socket_Close((*reader)->streamSocket);
                (*reader)->streamSocket = -1;
            }
            if ((*reader)->serverAddr)
            {
                free((*reader)->serverAddr);
            }
            if ((*reader)->mcastAddr)
            {
                free((*reader)->mcastAddr);
            }
            if ((*reader)->mcastIfaceAddr)
            {
                free((*reader)->mcastIfaceAddr);
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


static int ARSTREAM_Reader2_SetSocketReceiveBufferSize(ARSTREAM_Reader2_t *reader, int socket, int size)
{
    int ret = 0, err;
    socklen_t size2 = sizeof(size2);

    size /= 2;
    err = ARSAL_Socket_Setsockopt(socket, SOL_SOCKET, SO_RCVBUF, (void*)&size, sizeof(size));
    if (err != 0)
    {
        ret = -1;
        ARSAL_PRINT(ARSAL_PRINT_ERROR, ARSTREAM_READER2_TAG, "Failed to set socket receive buffer size to 2*%d bytes: error=%d (%s)", size, errno, strerror(errno));
    }

    size = -1;
    err = ARSAL_Socket_Getsockopt(socket, SOL_SOCKET, SO_RCVBUF, (void*)&size, &size2);
    if (err != 0)
    {
        ret = -1;
        ARSAL_PRINT(ARSAL_PRINT_ERROR, ARSTREAM_READER2_TAG, "Failed to get socket receive buffer size: error=%d (%s)", errno, strerror(errno));
    }
    else
    {
        ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARSTREAM_READER2_TAG, "Socket receive buffer size is %d bytes", size);
    }

    return ret;
}


static int ARSTREAM_Reader2_StreamSocketSetup(ARSTREAM_Reader2_t *reader)
{
    int ret = 0;
    struct sockaddr_in recvSin;
    int err;

    /* create socket */
    reader->streamSocket = ARSAL_Socket_Create(AF_INET, SOCK_DGRAM, 0);
    if (reader->streamSocket < 0)
    {
        ARSAL_PRINT(ARSAL_PRINT_ERROR, ARSTREAM_READER2_TAG, "Failed to create stream socket");
        ret = -1;
    }

#if HAVE_DECL_SO_NOSIGPIPE
    if (ret == 0)
    {
        /* remove SIGPIPE */
        int set = 1;
        err = ARSAL_Socket_Setsockopt(reader->streamSocket, SOL_SOCKET, SO_NOSIGPIPE, (void*)&set, sizeof(int));
        if (err != 0)
        {
            ARSAL_PRINT(ARSAL_PRINT_ERROR, ARSTREAM_READER2_TAG, "Error on setsockopt: error=%d (%s)", errno, strerror(errno));
        }
    }
#endif

    if (ret == 0)
    {
        /* set to non-blocking */
        int flags = fcntl(reader->streamSocket, F_GETFL, 0);
        fcntl(reader->streamSocket, F_SETFL, flags | O_NONBLOCK);

        memset(&recvSin, 0, sizeof(struct sockaddr_in));
        recvSin.sin_family = AF_INET;
        recvSin.sin_port = htons(reader->clientStreamPort);
        recvSin.sin_addr.s_addr = htonl(INADDR_ANY);

        if ((reader->mcastAddr) && (strlen(reader->mcastAddr)))
        {
            int addrFirst = atoi(reader->mcastAddr);
            if ((addrFirst >= 224) && (addrFirst <= 239))
            {
                /* multicast */
                struct ip_mreq mreq;
                memset(&mreq, 0, sizeof(mreq));
                err = inet_pton(AF_INET, reader->mcastAddr, &(mreq.imr_multiaddr.s_addr));
                if (err <= 0)
                {
                    ARSAL_PRINT(ARSAL_PRINT_ERROR, ARSTREAM_READER2_TAG, "Failed to convert address '%s'", reader->mcastAddr);
                    ret = -1;
                }

                if (ret == 0)
                {
                    if ((reader->mcastIfaceAddr) && (strlen(reader->mcastIfaceAddr) > 0))
                    {
                        err = inet_pton(AF_INET, reader->mcastIfaceAddr, &(mreq.imr_interface.s_addr));
                        if (err <= 0)
                        {
                            ARSAL_PRINT(ARSAL_PRINT_ERROR, ARSTREAM_READER2_TAG, "Failed to convert address '%s'", reader->mcastIfaceAddr);
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
                    err = ARSAL_Socket_Setsockopt(reader->streamSocket, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*)&mreq, sizeof(mreq));
                    if (err != 0)
                    {
                        ARSAL_PRINT(ARSAL_PRINT_ERROR, ARSTREAM_READER2_TAG, "Failed to join multacast group: error=%d (%s)", errno, strerror(errno));
                        ret = -1;
                    }
                }

                reader->isMulticast = 1;
            }
            else
            {
                ARSAL_PRINT(ARSAL_PRINT_ERROR, ARSTREAM_READER2_TAG, "Invalid multicast address '%s'", reader->mcastAddr);
                ret = -1;
            }
        }
    }

    if (ret == 0)
    {
        /* allow multiple sockets to use the same port */
        unsigned int yes = 1;
        err = ARSAL_Socket_Setsockopt(reader->streamSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&yes, sizeof(yes));
        if (err != 0)
        {
            ARSAL_PRINT(ARSAL_PRINT_ERROR, ARSTREAM_READER2_TAG, "Failed to set socket option SO_REUSEADDR: error=%d (%s)", errno, strerror(errno));
            ret = -1;
        }
    }

    if (ret == 0)
    {
        /* bind the socket */
        err = ARSAL_Socket_Bind(reader->streamSocket, (struct sockaddr*)&recvSin, sizeof(recvSin));
        if (err != 0)
        {
            ARSAL_PRINT(ARSAL_PRINT_ERROR, ARSTREAM_READER2_TAG, "Error on stream socket bind port=%d: error=%d (%s)", reader->clientStreamPort, errno, strerror(errno));
            ret = -1;
        }
    }

    if (ret == 0)
    {
        /* set the socket buffer size */
        if ((reader->maxNetworkLatencyMs) && (reader->maxBitrate))
        {
            err = ARSTREAM_Reader2_SetSocketReceiveBufferSize(reader, reader->streamSocket, (reader->maxNetworkLatencyMs * reader->maxBitrate * 2) / 8000); //TODO: should not be x2
            if (err != 0)
            {
                ARSAL_PRINT(ARSAL_PRINT_ERROR, ARSTREAM_READER2_TAG, "Failed to set the socket buffer size (%d)", err);
                ret = -1;
            }
        }
    }

    if (ret != 0)
    {
        if (reader->streamSocket >= 0)
        {
            ARSAL_Socket_Close(reader->streamSocket);
        }
        reader->streamSocket = -1;
    }

    return ret;
}


static int ARSTREAM_Reader2_ControlSocketSetup(ARSTREAM_Reader2_t *reader)
{
    int ret = 0;
    struct sockaddr_in sendSin;
    struct sockaddr_in recvSin;
    int err;

    if (ret == 0)
    {
        /* create socket */
        reader->controlSocket = ARSAL_Socket_Create(AF_INET, SOCK_DGRAM, 0);
        if (reader->controlSocket < 0)
        {
            ARSAL_PRINT(ARSAL_PRINT_ERROR, ARSTREAM_READER2_TAG, "Failed to create control socket");
            ret = -1;
        }
    }

#if HAVE_DECL_SO_NOSIGPIPE
    if (ret == 0)
    {
        /* remove SIGPIPE */
        int set = 1;
        err = ARSAL_Socket_Setsockopt(reader->controlSocket, SOL_SOCKET, SO_NOSIGPIPE, (void*)&set, sizeof(int));
        if (err != 0)
        {
            ARSAL_PRINT(ARSAL_PRINT_ERROR, ARSTREAM_READER2_TAG, "Error on setsockopt: error=%d (%s)", errno, strerror(errno));
        }
    }
#endif

    if (ret == 0)
    {
        /* set to non-blocking */
        int flags = fcntl(reader->controlSocket, F_GETFL, 0);
        fcntl(reader->controlSocket, F_SETFL, flags | O_NONBLOCK);

        /* receive address */
        memset(&recvSin, 0, sizeof(struct sockaddr_in));
        recvSin.sin_family = AF_INET;
        recvSin.sin_port = htons(reader->clientControlPort);
        recvSin.sin_addr.s_addr = htonl(INADDR_ANY);
    }

    if (ret == 0)
    {
        /* allow multiple sockets to use the same port */
        unsigned int yes = 1;
        err = ARSAL_Socket_Setsockopt(reader->controlSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&yes, sizeof(yes));
        if (err != 0)
        {
            ARSAL_PRINT(ARSAL_PRINT_ERROR, ARSTREAM_READER2_TAG, "Failed to set socket option SO_REUSEADDR: error=%d (%s)", errno, strerror(errno));
            ret = -1;
        }
    }

    if (ret == 0)
    {
        /* bind the socket */
        err = ARSAL_Socket_Bind(reader->controlSocket, (struct sockaddr*)&recvSin, sizeof(recvSin));
        if (err != 0)
        {
            ARSAL_PRINT(ARSAL_PRINT_ERROR, ARSTREAM_READER2_TAG, "Error on control socket bind port=%d: error=%d (%s)", reader->clientControlPort, errno, strerror(errno));
            ret = -1;
        }
    }

    if (ret == 0)
    {
        /* send address */
        memset(&sendSin, 0, sizeof(struct sockaddr_in));
        sendSin.sin_family = AF_INET;
        sendSin.sin_port = htons(reader->serverControlPort);
        err = inet_pton(AF_INET, reader->serverAddr, &(sendSin.sin_addr));
        if (err <= 0)
        {
            ARSAL_PRINT(ARSAL_PRINT_ERROR, ARSTREAM_READER2_TAG, "Failed to convert address '%s'", reader->serverAddr);
            ret = -1;
        }
    }

    if (ret == 0)
    {
        /* connect the socket */
        err = ARSAL_Socket_Connect(reader->controlSocket, (struct sockaddr*)&sendSin, sizeof(sendSin));
        if (err != 0)
        {
            ARSAL_PRINT(ARSAL_PRINT_ERROR, ARSTREAM_READER2_TAG, "Error on control socket connect to addr='%s' port=%d: error=%d (%s)", reader->serverAddr, reader->serverControlPort, errno, strerror(errno));
            ret = -1;
        }
    }

    if (ret != 0)
    {
        if (reader->controlSocket >= 0)
        {
            ARSAL_Socket_Close(reader->controlSocket);
        }
        reader->controlSocket = -1;
    }

    return ret;
}


static void ARSTREAM_Reader2_UpdateMonitoring(ARSTREAM_Reader2_t *reader, uint32_t timestamp, uint16_t seqNum, uint16_t markerBit, uint32_t bytes)
{
    uint64_t curTime, timestampShifted;
    struct timespec t1;
    ARSAL_Time_GetTime(&t1);
    curTime = (uint64_t)t1.tv_sec * 1000000 + (uint64_t)t1.tv_nsec / 1000;

    //TODO: handle the timestamp 32 bits loopback
    timestampShifted = (reader->clockDelta != 0) ? (((((uint64_t)timestamp * 1000) + 45) / 90) - reader->clockDelta) : 0; /* 90000 Hz clock to microseconds */

    ARSAL_Mutex_Lock(&(reader->monitoringMutex));

    if (reader->monitoringCount < ARSTREAM_READER2_MONITORING_MAX_POINTS)
    {
        reader->monitoringCount++;
    }
    reader->monitoringIndex = (reader->monitoringIndex + 1) % ARSTREAM_READER2_MONITORING_MAX_POINTS;
    reader->monitoringPoint[reader->monitoringIndex].bytes = bytes;
    reader->monitoringPoint[reader->monitoringIndex].timestamp = timestamp;
    reader->monitoringPoint[reader->monitoringIndex].timestampShifted = timestampShifted;
    reader->monitoringPoint[reader->monitoringIndex].seqNum = seqNum;
    reader->monitoringPoint[reader->monitoringIndex].markerBit = markerBit;
    reader->monitoringPoint[reader->monitoringIndex].recvTimestamp = curTime;

    ARSAL_Mutex_Unlock(&(reader->monitoringMutex));

#ifdef ARSTREAM_READER2_MONITORING_OUTPUT
    if (reader->fMonitorOut)
    {
        fprintf(reader->fMonitorOut, "%llu %lu %llu %u %u %lu\n", (long long unsigned int)curTime, (long unsigned int)timestamp, (long long unsigned int)timestampShifted, seqNum, markerBit, (long unsigned int)bytes);
    }
#endif
}


static int ARSTREAM_Reader2_ReadData(ARSTREAM_Reader2_t *reader, uint8_t *recvBuffer, int recvBufferSize, int *recvSize)
{
    int ret = 0, pollRet;
    ssize_t bytes;
    struct pollfd p;

    if ((!recvBuffer) || (!recvSize))
    {
        return -1;
    }

    bytes = ARSAL_Socket_Recv(reader->streamSocket, recvBuffer, recvBufferSize, 0);
    if (bytes < 0)
    {
        /* socket receive failed */
        switch (errno)
        {
            case EAGAIN:
                /* poll */
                p.fd = reader->streamSocket;
                p.events = POLLIN;
                p.revents = 0;
                pollRet = poll(&p, 1, ARSTREAM_READER2_STREAM_DATAREAD_TIMEOUT_MS);
                if (pollRet == 0)
                {
                    /* failed: poll timeout */
                    ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARSTREAM_READER2_TAG, "Polling timed out");
                    ret = -2;
                    *recvSize = 0;
                }
                else if (pollRet < 0)
                {
                    /* failed: poll error */
                    ARSAL_PRINT(ARSAL_PRINT_ERROR, ARSTREAM_READER2_TAG, "Poll error: error=%d (%s)", errno, strerror(errno));
                    ret = -1;
                    *recvSize = 0;
                }
                else if (p.revents & POLLIN)
                {
                    bytes = ARSAL_Socket_Recv(reader->streamSocket, recvBuffer, recvBufferSize, 0);
                    if (bytes >= 0)
                    {
                        /* success: save the number of bytes read */
                        *recvSize = bytes;
                    }
                    else if (errno == EAGAIN)
                    {
                        /* failed: socket not ready (this should not happen) */
                        ARSAL_PRINT(ARSAL_PRINT_WARNING, ARSTREAM_READER2_TAG, "Socket not ready for reading");
                        ret = -2;
                        *recvSize = 0;
                    }
                    else
                    {
                        /* failed: socket error */
                        ARSAL_PRINT(ARSAL_PRINT_ERROR, ARSTREAM_READER2_TAG, "Socket receive error #2 %d ('%s')", errno, strerror(errno));
                        ret = -1;
                        *recvSize = 0;
                    }
                }
                else
                {
                    /* no poll error, no timeout, but socket is not ready */
                    int error = 0;
                    socklen_t errlen = sizeof(error);
                    ARSAL_Socket_Getsockopt(reader->streamSocket, SOL_SOCKET, SO_ERROR, (void *)&error, &errlen);
                    ARSAL_PRINT(ARSAL_PRINT_ERROR, ARSTREAM_READER2_TAG, "No poll error, no timeout, but socket is not ready (revents = %d, error = %d)", p.revents, error);
                    ret = -1;
                    *recvSize = 0;
                }
                break;
            default:
                /* failed: socket error */
                ARSAL_PRINT(ARSAL_PRINT_ERROR, ARSTREAM_READER2_TAG, "Socket receive error %d ('%s')", errno, strerror(errno));
                ret = -1;
                *recvSize = 0;
                break;
        }
    }
    else
    {
        /* success: save the number of bytes read */
        *recvSize = bytes;
    }

    return ret;
}


static int ARSTREAM_Reader2_CheckBufferSize(ARSTREAM_Reader2_t *reader, int payloadSize)
{
    int ret = 0;

    if ((reader->currentNaluBuffer == NULL) || (reader->currentNaluSize + payloadSize > reader->currentNaluBufferSize))
    {
        int32_t nextNaluBufferSize = reader->currentNaluSize + payloadSize, dummy = 0;
        uint8_t *nextNaluBuffer = reader->naluCallback(ARSTREAM_READER2_CAUSE_NALU_BUFFER_TOO_SMALL, reader->currentNaluBuffer, 0, 0, 0, 0, 0, 0, &nextNaluBufferSize, reader->naluCallbackUserPtr);
        ret = -1;
        if ((nextNaluBuffer != NULL) && (nextNaluBufferSize > 0) && (nextNaluBufferSize >= reader->currentNaluSize + payloadSize))
        {
            if ((reader->currentNaluBuffer != NULL) && (reader->currentNaluSize != 0))
            {
                memcpy(nextNaluBuffer, reader->currentNaluBuffer, reader->currentNaluSize);
            }
            reader->naluCallback(ARSTREAM_READER2_CAUSE_NALU_COPY_COMPLETE, reader->currentNaluBuffer, 0, 0, 0, 0, 0, 0, &dummy, reader->naluCallbackUserPtr);
            ret = 0;
        }
        reader->currentNaluBuffer = nextNaluBuffer;
        reader->currentNaluBufferSize = nextNaluBufferSize;
    }

    return ret;
}


static void ARSTREAM_Reader2_OutputNalu(ARSTREAM_Reader2_t *reader, uint32_t timestamp, int isFirstNaluInAu, int isLastNaluInAu, int missingPacketsBefore)
{
    uint64_t timestampScaled, timestampScaledShifted;

    timestampScaled = ((((uint64_t)timestamp * 1000) + 45) / 90); /* 90000 Hz clock to microseconds */
    //TODO: handle the timestamp 32 bits loopback
    timestampScaledShifted = (reader->clockDelta != 0) ? (timestampScaled - reader->clockDelta) : 0; /* 90000 Hz clock to microseconds */

    if (reader->resenderCount > 0)
    {
        ARSTREAM_Reader2_ResendNalu(reader, reader->currentNaluBuffer, reader->currentNaluSize, timestampScaled, isLastNaluInAu, missingPacketsBefore);
    }

    reader->currentNaluBuffer = reader->naluCallback(ARSTREAM_READER2_CAUSE_NALU_COMPLETE, reader->currentNaluBuffer, reader->currentNaluSize,
                                                     timestampScaled, timestampScaledShifted,
                                                     isFirstNaluInAu, isLastNaluInAu, missingPacketsBefore, &(reader->currentNaluBufferSize), reader->naluCallbackUserPtr);
}


void* ARSTREAM_Reader2_RunStreamThread(void *ARSTREAM_Reader2_t_Param)
{
    ARSTREAM_Reader2_t *reader = (ARSTREAM_Reader2_t *)ARSTREAM_Reader2_t_Param;
    uint8_t *recvBuffer = NULL;
    int recvBufferSize;
    int recvSize, payloadSize;
    int fuPending = 0, currentAuSize = 0;
    ARSTREAM_NetworkHeaders_DataHeader2_t *header = NULL;
    uint32_t rtpTimestamp = 0, previousTimestamp = 0;
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
        ARSAL_PRINT(ARSAL_PRINT_ERROR, ARSTREAM_READER2_TAG, "Error while starting %s, cannot allocate memory", __FUNCTION__);
        return (void *)0;
    }
    header = (ARSTREAM_NetworkHeaders_DataHeader2_t *)recvBuffer;

    /* Bind */
    ret = ARSTREAM_Reader2_StreamSocketSetup(reader);
    if (ret != 0)
    {
        ARSAL_PRINT(ARSAL_PRINT_ERROR, ARSTREAM_READER2_TAG, "Failed to setup the stream socket (error %d) - aborting", ret);
        free(recvBuffer);
        return (void*)0;
    }

    ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARSTREAM_READER2_TAG, "Reader stream thread running");
    ARSAL_Mutex_Lock(&(reader->streamMutex));
    reader->streamThreadStarted = 1;
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
            currentSeqNum = (int)ntohs(header->seqNum);
            currentFlags = ntohs(header->flags);
            ARSTREAM_Reader2_UpdateMonitoring(reader, rtpTimestamp, currentSeqNum, (currentFlags & (1 << 7)) ? 1 : 0, (uint32_t)recvSize);

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
                if ((previousTimestamp != 0) && (rtpTimestamp != previousTimestamp))
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
                                        ARSTREAM_Reader2_OutputNalu(reader, rtpTimestamp, isFirst, isLast, gapsInSeqNum);
                                        gapsInSeqNum = 0;
                                    }
                                }
                                else
                                {
                                    ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARSTREAM_READER2_TAG, "Failed to get a larger buffer (output size %d) for FU-A NALU at seqNum %d", outputSize, currentSeqNum);
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
                                    ARSTREAM_Reader2_OutputNalu(reader, rtpTimestamp, isFirst, isLast, gapsInSeqNum);
                                    gapsInSeqNum = 0;
                                }
                                else
                                {
                                    ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARSTREAM_READER2_TAG, "Failed to get a larger buffer (output size %d) for STAP-A NALU at seqNum %d", naluSize + startCodeLength, currentSeqNum);
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
                            ARSTREAM_Reader2_OutputNalu(reader, rtpTimestamp, isFirst, isLast, gapsInSeqNum);
                            gapsInSeqNum = 0;
                        }
                        else
                        {
                            ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARSTREAM_READER2_TAG, "Failed to get a larger buffer (output size %d) for single NALU at seqNum %d", payloadSize + startCodeLength, currentSeqNum);
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
                previousTimestamp = rtpTimestamp;
            }
            else
            {
                /* out of order packet */
                //TODO
                ARSAL_PRINT(ARSAL_PRINT_WARNING, ARSTREAM_READER2_TAG, "Out of order sequence number (currentSeqNum=%d, previousSeqNum=%d, seqNumDelta=%d)", currentSeqNum, previousSeqNum, seqNumDelta); //TODO: debug
            }
        }

        ARSAL_Mutex_Lock(&(reader->streamMutex));
        shouldStop = reader->threadsShouldStop;
        if (reader->scheduleNaluBufferChange)
        {
            reader->currentNaluBuffer = NULL;
            reader->currentNaluBufferSize = 0;
            reader->scheduleNaluBufferChange = 0;
            ARSAL_Mutex_Unlock(&(reader->streamMutex));
            ARSAL_Cond_Signal(&(reader->streamCond));
        }
        else
        {
            ARSAL_Mutex_Unlock(&(reader->streamMutex));
        }
    }

    reader->naluCallback(ARSTREAM_READER2_CAUSE_CANCEL, reader->currentNaluBuffer, 0, 0, 0, 0, 0, 0, &(reader->currentNaluBufferSize), reader->naluCallbackUserPtr);

    ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARSTREAM_READER2_TAG, "Reader stream thread ended");
    ARSAL_Mutex_Lock(&(reader->streamMutex));
    reader->streamThreadStarted = 0;
    ARSAL_Mutex_Unlock(&(reader->streamMutex));

    if (recvBuffer)
    {
        free(recvBuffer);
        recvBuffer = NULL;
    }

    return (void *)0;
}


void* ARSTREAM_Reader2_RunControlThread(void *ARSTREAM_Reader2_t_Param)
{
    ARSTREAM_Reader2_t *reader = (ARSTREAM_Reader2_t *)ARSTREAM_Reader2_t_Param;
    uint8_t *msgBuffer;
    int msgBufferSize = sizeof(ARSTREAM_NetworkHeaders_ClockFrame_t);
    ARSTREAM_NetworkHeaders_ClockFrame_t *clockFrame;
    uint64_t originateTimestamp = 0;
    uint64_t receiveTimestamp = 0;
    uint64_t transmitTimestamp = 0;
    uint64_t receiveTimestamp2 = 0;
    uint64_t originateTimestamp2 = 0;
    uint64_t loopStartTime = 0;
    int64_t clockDelta = 0;
    int64_t rtDelay = 0;
    uint32_t tsH, tsL;
    ssize_t bytes;
    struct timespec t1;
    struct pollfd p;
    int shouldStop, ret, pollRet, timeElapsed, sleepDuration;

#ifdef ARSTREAM_READER2_CLOCKSYNC_DEBUG_FILE
    FILE *fDebug;
    fDebug = fopen("clock.dat", "w");
#endif

    /* Parameters check */
    if (reader == NULL)
    {
        ARSAL_PRINT(ARSAL_PRINT_ERROR, ARSTREAM_READER2_TAG, "Error while starting %s, bad parameters", __FUNCTION__);
        return (void *)0;
    }

    /* Alloc and check */
    msgBuffer = malloc(msgBufferSize);
    if (msgBuffer == NULL)
    {
        ARSAL_PRINT(ARSAL_PRINT_ERROR, ARSTREAM_READER2_TAG, "Error while starting %s, cannot allocate memory", __FUNCTION__);
        return (void *)0;
    }
    clockFrame = (ARSTREAM_NetworkHeaders_ClockFrame_t*)msgBuffer;

    /* Socket setup */
    ret = ARSTREAM_Reader2_ControlSocketSetup(reader);
    if (ret != 0)
    {
        ARSAL_PRINT(ARSAL_PRINT_ERROR, ARSTREAM_READER2_TAG, "Failed to setup the control socket (error %d) - aborting", ret);
        free(msgBuffer);
        return (void*)0;
    }

    ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARSTREAM_READER2_TAG, "Reader control thread running");
    ARSAL_Mutex_Lock(&(reader->streamMutex));
    reader->controlThreadStarted = 1;
    shouldStop = reader->threadsShouldStop;
    ARSAL_Mutex_Unlock(&(reader->streamMutex));

    while (shouldStop == 0)
    {
        ARSAL_Time_GetTime(&t1);
        loopStartTime = transmitTimestamp = (uint64_t)t1.tv_sec * 1000000 + (uint64_t)t1.tv_nsec / 1000;

        memset(clockFrame, 0, sizeof(ARSTREAM_NetworkHeaders_ClockFrame_t));

        tsH = (uint32_t)(transmitTimestamp >> 32);
        tsL = (uint32_t)(transmitTimestamp & 0xFFFFFFFF);
        clockFrame->transmitTimestampH = htonl(tsH);
        clockFrame->transmitTimestampL = htonl(tsL);

        bytes = ARSAL_Socket_Send(reader->controlSocket, msgBuffer, msgBufferSize, 0);
        if (bytes < 0)
        {
            ARSAL_PRINT(ARSAL_PRINT_ERROR, ARSTREAM_READER2_TAG, "Socket send error: error=%d (%s)", errno, strerror(errno));
        }
        else
        {
            originateTimestamp2 = transmitTimestamp;

            /* poll */
            p.fd = reader->controlSocket;
            p.events = POLLIN;
            p.revents = 0;
            pollRet = poll(&p, 1, ARSTREAM_READER2_CLOCKSYNC_DATAREAD_TIMEOUT_MS);
            if (pollRet == 0)
            {
                /* failed: poll timeout */
                ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARSTREAM_READER2_TAG, "Polling timed out");
            }
            else if (pollRet < 0)
            {
                /* failed: poll error */
                ARSAL_PRINT(ARSAL_PRINT_ERROR, ARSTREAM_READER2_TAG, "Poll error: error=%d (%s)", errno, strerror(errno));
            }
            else if (p.revents & POLLIN)
            {
                do
                {
                    bytes = ARSAL_Socket_Recv(reader->controlSocket, msgBuffer, msgBufferSize, 0);
                    if (bytes >= 0)
                    {
                        /* success */
                        ARSAL_Time_GetTime(&t1);
                        receiveTimestamp2 = (uint64_t)t1.tv_sec * 1000000 + (uint64_t)t1.tv_nsec / 1000;
                        originateTimestamp = ((uint64_t)ntohl(clockFrame->originateTimestampH) << 32) + (uint64_t)ntohl(clockFrame->originateTimestampL);
                        receiveTimestamp = ((uint64_t)ntohl(clockFrame->receiveTimestampH) << 32) + (uint64_t)ntohl(clockFrame->receiveTimestampL);
                        transmitTimestamp = ((uint64_t)ntohl(clockFrame->transmitTimestampH) << 32) + (uint64_t)ntohl(clockFrame->transmitTimestampL);

                        if (originateTimestamp == originateTimestamp2)
                        {
                            clockDelta = (int64_t)(receiveTimestamp + transmitTimestamp) / 2 - (int64_t)(originateTimestamp + receiveTimestamp2) / 2;
                            reader->clockDelta = clockDelta;
                            rtDelay = (receiveTimestamp2 - originateTimestamp) - (transmitTimestamp - receiveTimestamp);

#ifdef ARSTREAM_READER2_CLOCKSYNC_DEBUG_FILE
                            setlocale(LC_ALL, "C");
                            fprintf(fDebug, "%llu %llu %llu %llu %lld %lld\n",
                                    (long long unsigned int)originateTimestamp, (long long unsigned int)receiveTimestamp, (long long unsigned int)transmitTimestamp, (long long unsigned int)receiveTimestamp2,
                                    (long long int)clockDelta, (long long int)rtDelay);
                            setlocale(LC_ALL, "");
#endif

                            /*ARSAL_PRINT(ARSAL_PRINT_WARNING, ARSTREAM_READER2_TAG, "Clock - originateTS: %llu | receiveTS: %llu | transmitTS: %llu | receiveTS2: %llu | delta: %lld | rtDelay: %lld",
                                        (long long unsigned int)originateTimestamp, (long long unsigned int)receiveTimestamp, (long long unsigned int)transmitTimestamp, (long long unsigned int)receiveTimestamp2,
                                        (long long int)clockDelta, (long long int)rtDelay);*/ //TODO: debug
                        }
                        else
                        {
                            /*ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARSTREAM_READER2_TAG, "Originate timestamp missmatch (%llu vs. %llu)", (long long unsigned int)originateTimestamp2, (long long unsigned int)originateTimestamp);*/ //TODO: debug
                        }
                    }
                    else if (errno != EAGAIN)
                    {
                        /* failed: socket error */
                        ARSAL_PRINT(ARSAL_PRINT_ERROR, ARSTREAM_READER2_TAG, "Socket receive error %d ('%s')", errno, strerror(errno));
                    }
                }
                while (bytes >= 0);
            }
            else
            {
                /* no poll error, no timeout, but socket is not ready */
                int error = 0;
                socklen_t errlen = sizeof(error);
                ARSAL_Socket_Getsockopt(reader->controlSocket, SOL_SOCKET, SO_ERROR, (void *)&error, &errlen);
                ARSAL_PRINT(ARSAL_PRINT_ERROR, ARSTREAM_READER2_TAG, "No poll error, no timeout, but socket is not ready (revents = %d, error = %d)", p.revents, error);
            }
        }

        ARSAL_Time_GetTime(&t1);
        timeElapsed = (int)((uint64_t)t1.tv_sec * 1000000 + (uint64_t)t1.tv_nsec / 1000 - loopStartTime);

        sleepDuration = ARSTREAM_READER2_CLOCKSYNC_PERIOD_MS * 1000 - timeElapsed;
        if (sleepDuration > 0)
        {
            usleep(sleepDuration);
        }

        ARSAL_Mutex_Lock(&(reader->streamMutex));
        shouldStop = reader->threadsShouldStop;
        ARSAL_Mutex_Unlock(&(reader->streamMutex));
    }

    ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARSTREAM_READER2_TAG, "Reader control thread ended");
    ARSAL_Mutex_Lock(&(reader->streamMutex));
    reader->controlThreadStarted = 0;
    ARSAL_Mutex_Unlock(&(reader->streamMutex));

    if (msgBuffer)
    {
        free(msgBuffer);
        msgBuffer = NULL;
    }

#ifdef ARSTREAM_READER2_CLOCKSYNC_DEBUG_FILE
    fclose(fDebug);
#endif

    return (void *)0;
}


void* ARSTREAM_Reader2_GetNaluCallbackUserPtr(ARSTREAM_Reader2_t *reader)
{
    void *ret = NULL;
    if (reader != NULL)
    {
        ret = reader->naluCallbackUserPtr;
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
            auTimestamp = ((((uint64_t)(reader->monitoringPoint[idx].timestamp /*TODO*/) * 1000) + 45) / 90) - reader->clockDelta; /* 90000 Hz clock to microseconds */
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
                auTimestamp = ((((uint64_t)(reader->monitoringPoint[idx].timestamp /*TODO*/) * 1000) + 45) / 90) - reader->clockDelta; /* 90000 Hz clock to microseconds */
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
    }

    /* Setup ARStream_Sender2 */
    if (internalError == ARSTREAM_OK)
    {
        eARSTREAM_ERROR error2;
        ARSTREAM_Sender2_Config_t senderConfig;
        memset(&senderConfig, 0, sizeof(senderConfig));
        senderConfig.clientAddr = config->clientAddr;
        senderConfig.mcastAddr = config->mcastAddr;
        senderConfig.mcastIfaceAddr = config->mcastIfaceAddr;
        senderConfig.serverStreamPort = config->serverStreamPort;
        senderConfig.serverControlPort = config->serverControlPort;
        senderConfig.clientStreamPort = config->clientStreamPort;
        senderConfig.clientControlPort = config->clientControlPort;
        senderConfig.naluCallback = ARSTREAM_Reader2_Resender_NaluCallback;
        senderConfig.naluCallbackUserPtr = (void*)retResender;
        senderConfig.naluFifoSize = ARSTREAM_READER2_RESENDER_MAX_NALU_BUFFER_COUNT;
        senderConfig.maxPacketSize = config->maxPacketSize;
        senderConfig.targetPacketSize = config->targetPacketSize;
        senderConfig.maxBitrate = reader->maxBitrate;
        senderConfig.maxLatencyMs = config->maxLatencyMs;
        senderConfig.maxNetworkLatencyMs = config->maxNetworkLatencyMs;
        retResender->sender = ARSTREAM_Sender2_New(&senderConfig, &error2);
        if (error2 != ARSTREAM_OK)
        {
            internalError = error2;
        }
    }

    if (internalError == ARSTREAM_OK)
    {
        ARSAL_Mutex_Lock(&(reader->resenderMutex));
        if (reader->resenderCount < ARSTREAM_READER2_RESENDER_MAX_COUNT)
        {
            retResender->reader = reader;
            retResender->senderRunning = 1;
            reader->resender[reader->resenderCount] = retResender;
            reader->resenderCount++;
        }
        else
        {
            internalError = ARSTREAM_ERROR_BAD_PARAMETERS;
        }
        ARSAL_Mutex_Unlock(&(reader->resenderMutex));
    }

    if ((internalError != ARSTREAM_OK) &&
        (retResender != NULL))
    {
        if (retResender->sender) ARSTREAM_Sender2_Delete(&(retResender->sender));
        free(retResender);
        retResender = NULL;
    }

    SET_WITH_CHECK(error, internalError);
    return retResender;
}


void ARSTREAM_Reader2_Resender_Stop(ARSTREAM_Reader2_Resender_t *resender)
{
    if (resender != NULL)
    {
        ARSAL_Mutex_Lock(&(resender->reader->resenderMutex));
        if (resender->sender != NULL)
        {
            ARSTREAM_Sender2_StopSender(resender->sender);
            resender->senderRunning = 0;
        }
        ARSAL_Mutex_Unlock(&(resender->reader->resenderMutex));
    }
}


eARSTREAM_ERROR ARSTREAM_Reader2_Resender_Delete(ARSTREAM_Reader2_Resender_t **resender)
{
    ARSTREAM_Reader2_t *reader;
    int i, idx;
    eARSTREAM_ERROR retVal = ARSTREAM_ERROR_BAD_PARAMETERS;

    if ((resender != NULL) &&
        (*resender != NULL))
    {
        reader = (*resender)->reader;
        if (reader != NULL)
        {
            ARSAL_Mutex_Lock(&(reader->resenderMutex));

            /* find the resender in the array */
            for (i = 0, idx = -1; i < reader->resenderCount; i++)
            {
                if (*resender == reader->resender[i])
                {
                    idx = i;
                    break;
                }
            }
            if (idx < 0)
            {
                ARSAL_Mutex_Unlock(&(reader->resenderMutex));
                ARSAL_PRINT(ARSAL_PRINT_ERROR, ARSTREAM_READER2_TAG, "Failed to find the resender");
                return retVal;
            }

            if (!(*resender)->senderRunning)
            {
                retVal = ARSTREAM_Sender2_Delete(&((*resender)->sender));
                if (retVal == ARSTREAM_OK)
                {
                    /* remove the resender: shift the values in the resender array */
                    reader->resender[idx] = NULL;
                    for (i = idx; i < reader->resenderCount - 1; i++)
                    {
                        reader->resender[i] = reader->resender[i + 1];
                    }
                    reader->resenderCount--;

                    free(*resender);
                    *resender = NULL;
                }
                else
                {
                    ARSAL_PRINT(ARSAL_PRINT_ERROR, ARSTREAM_READER2_TAG, "Resender: failed to delete Sender2 (%d)", retVal);
                }
            }
            else
            {
                ARSAL_PRINT(ARSAL_PRINT_ERROR, ARSTREAM_READER2_TAG, "Resender: sender2 is still running");
            }

            ARSAL_Mutex_Unlock(&(reader->resenderMutex));
        }
    }
    else
    {
        ARSAL_PRINT(ARSAL_PRINT_ERROR, ARSTREAM_READER2_TAG, "Invalid resender");
    }

    return retVal;
}


void* ARSTREAM_Reader2_Resender_RunStreamThread (void *ARSTREAM_Reader2_Resender_t_Param)
{
    ARSTREAM_Reader2_Resender_t *resender = (ARSTREAM_Reader2_Resender_t *)ARSTREAM_Reader2_Resender_t_Param;

    if (resender == NULL)
    {
        return (void *)0;
    }

    return ARSTREAM_Sender2_RunStreamThread((void *)resender->sender);
}


void* ARSTREAM_Reader2_Resender_RunControlThread (void *ARSTREAM_Reader2_Resender_t_Param)
{
    ARSTREAM_Reader2_Resender_t *resender = (ARSTREAM_Reader2_Resender_t *)ARSTREAM_Reader2_Resender_t_Param;

    if (resender == NULL)
    {
        return (void *)0;
    }

    return ARSTREAM_Sender2_RunControlThread((void *)resender->sender);
}

