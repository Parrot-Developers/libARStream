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
    ARNETWORK_Manager_t *manager;
    int dataBufferID;
    int ackBufferID;
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
};


void ARSTREAM_Reader2_InitStreamDataBuffer(ARNETWORK_IOBufferParam_t *bufferParams, int bufferID, int maxPacketSize)
{
    ARSTREAM_Buffers_InitStreamDataBuffer(bufferParams, bufferID, maxPacketSize, ARSTREAM_BUFFERS_DATA_BUFFER_NUMBER_OF_CELLS);
}


void ARSTREAM_Reader2_InitStreamAckBuffer(ARNETWORK_IOBufferParam_t *bufferParams, int bufferID)
{
    ARSTREAM_Buffers_InitStreamAckBuffer(bufferParams, bufferID);
}


ARSTREAM_Reader2_t* ARSTREAM_Reader2_New(ARNETWORK_Manager_t *manager, int dataBufferID, int ackBufferID, ARSTREAM_Reader2_AuCallback_t auCallback, uint8_t *auBuffer, int auBufferSize, int maxPacketSize, int insertStartCodes, int outputIncompleteAu, void *custom, eARSTREAM_ERROR *error)
{
    ARSTREAM_Reader2_t *retReader = NULL;
    int streamMutexWasInit = 0;
    eARSTREAM_ERROR internalError = ARSTREAM_OK;
    /* ARGS Check */
    if ((manager == NULL) ||
        (auCallback == NULL) ||
        (auBuffer == NULL) ||
        (auBufferSize == 0) ||
        (maxPacketSize == 0))
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
        retReader->manager = manager;
        retReader->dataBufferID = dataBufferID;
        retReader->ackBufferID = ackBufferID;
        retReader->maxPacketSize = maxPacketSize;
        retReader->insertStartCodes = insertStartCodes;
        retReader->outputIncompleteAu = outputIncompleteAu;
        retReader->auCallback = auCallback;
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

    if ((internalError != ARSTREAM_OK) &&
        (retReader != NULL))
    {
        if (streamMutexWasInit == 1)
        {
            ARSAL_Mutex_Destroy(&(retReader->streamMutex));
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
            ARSAL_Mutex_Destroy(&((*reader)->streamMutex));
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


void* ARSTREAM_Reader2_RunDataThread(void *ARSTREAM_Reader2_t_Param)
{
    ARSTREAM_Reader2_t *reader = (ARSTREAM_Reader2_t *)ARSTREAM_Reader2_t_Param;
    uint8_t *recvBuffer = NULL;
    int recvBufferSize;
    int recvSize, payloadSize;
    ARSTREAM_NetworkHeaders_DataHeader2_t *header = NULL;
    uint64_t currentTimestamp = 0, previousTimestamp = 0, receptionTs = 0;
    uint16_t currentFlags;
    int startSeqNum = -1, previousSeqNum = -1, currentSeqNum, seqNumDelta;
    int gapsInSeqNum = 0;
    uint32_t startCode = 0;
    int startCodeLength = 0;
    int shouldStop;

    /* Parameters check */
    if (reader == NULL)
    {
        ARSAL_PRINT(ARSAL_PRINT_ERROR, ARSTREAM_READER2_TAG, "Error while starting %s, bad parameters", __FUNCTION__);
        return (void *)0;
    }

    recvBufferSize = reader->maxPacketSize;
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

    ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARSTREAM_READER2_TAG, "Stream reader thread running");
    ARSAL_Mutex_Lock(&(reader->streamMutex));
    reader->dataThreadStarted = 1;
    shouldStop = reader->threadsShouldStop;
    ARSAL_Mutex_Unlock(&(reader->streamMutex));

    while (shouldStop == 0)
    {
        eARNETWORK_ERROR err = ARNETWORK_Manager_ReadDataWithTimeout(reader->manager, reader->dataBufferID, recvBuffer, recvBufferSize, &recvSize, ARSTREAM_READER2_DATAREAD_TIMEOUT_MS);
        if (ARNETWORK_OK != err)
        {
            if (ARNETWORK_ERROR_BUFFER_EMPTY != err)
            {
                ARSAL_PRINT(ARSAL_PRINT_ERROR, ARSTREAM_READER2_TAG, "Error while reading stream data: %s", ARNETWORK_Error_ToString (err));
            }
        }
        else
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
            if (reader->currentAuSize + payloadSize + startCodeLength > reader->currentAuBufferSize)
            {
                uint32_t nextAuBufferSize = reader->currentAuSize + payloadSize + startCodeLength, dummy = 0;
                uint8_t *nextAuBuffer = reader->auCallback(ARSTREAM_READER2_CAUSE_AU_BUFFER_TOO_SMALL, reader->currentAuBuffer, 0, 0, 0, 0, &nextAuBufferSize, reader->custom);
                if ((nextAuBufferSize > 0) && (nextAuBufferSize >= reader->currentAuSize + payloadSize + startCodeLength))
                {
                    memcpy(nextAuBuffer, reader->currentAuBuffer, reader->currentAuSize);
                    reader->auCallback(ARSTREAM_READER2_CAUSE_AU_COPY_COMPLETE, reader->currentAuBuffer, 0, 0, 0, 0, &dummy, reader->custom);
                }
                reader->currentAuBuffer = nextAuBuffer;
                reader->currentAuBufferSize = nextAuBufferSize;
            }

            if (reader->currentAuSize + payloadSize + startCodeLength <= reader->currentAuBufferSize)
            {
                if (startCodeLength > 0)
                {
                    memcpy(reader->currentAuBuffer + reader->currentAuSize, &startCode, startCodeLength);
                }
                memcpy(reader->currentAuBuffer + reader->currentAuSize + startCodeLength, recvBuffer + sizeof(ARSTREAM_NetworkHeaders_DataHeader2_t), payloadSize);
                reader->currentAuSize += payloadSize + startCodeLength;

                if (currentFlags & (1 << 7))
                {
                    /* the marker bit is set: output the access unit */
                    struct timespec res;
                    ARSAL_Time_GetLocalTime(&res, NULL);
                    receptionTs = (uint64_t)res.tv_sec * 1000000 + (uint64_t)res.tv_nsec / 1000;
                    ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARSTREAM_READER2_TAG, "Output access unit at seqNum %d, size %d bytes (missing %d packets)", currentSeqNum, reader->currentAuSize, gapsInSeqNum);
                    reader->currentAuBuffer = reader->auCallback(ARSTREAM_READER2_CAUSE_AU_COMPLETE, reader->currentAuBuffer, reader->currentAuSize, currentTimestamp, gapsInSeqNum, currentSeqNum - startSeqNum + 1, &(reader->currentAuBufferSize), reader->custom);
                    gapsInSeqNum = 0;
                    reader->currentAuSize = 0;
                }
            }

            previousSeqNum = currentSeqNum;
            previousTimestamp = currentTimestamp;
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

