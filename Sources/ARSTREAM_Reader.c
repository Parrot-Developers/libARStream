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
 * @file ARSTREAM_Reader.c
 * @brief Stream reader on network
 * @date 03/22/2013
 * @author nicolas.brulez@parrot.com
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

#include <libARStream/ARSTREAM_Reader.h>
#include <libARStream/ARSTREAM_Reader2.h>
#include <libARSAL/ARSAL_Print.h>
#include <libARSAL/ARSAL_Mutex.h>
#include <libARSAL/ARSAL_Endianness.h>
#include <libARSAL/ARSAL_Thread.h>

/*
 * Macros
 */

#define ARSTREAM_READER_TAG "ARSTREAM_Reader"

#define ARSTREAM_READER_RESENDER_MAX_COUNT (4)

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

struct ARSTREAM_Reader_t {
    /* Configuration on New */
    ARNETWORK_Manager_t *manager;
    int dataBufferID;
    int ackBufferID;
    uint32_t maxFragmentSize;
    int32_t maxAckInterval;
    ARSTREAM_Reader_FrameCompleteCallback_t callback;
    void *custom;

    /* Buffers */
    int currentAuBufferSize;
    int currentAuSize;
    uint8_t *currentAuBuffer;
    int currentNaluBufferSize;
    uint8_t *currentNaluBuffer;

    /* ARStream_Reader2 */
    ARSTREAM_Reader2_t *reader2;
    int reader2running;

    /* ARStream_Reader2_Resender */
    ARSTREAM_Reader2_Resender_t *resender[ARSTREAM_READER_RESENDER_MAX_COUNT];
    ARSAL_Thread_t resenderSendThread[ARSTREAM_READER_RESENDER_MAX_COUNT];
    ARSAL_Thread_t resenderRecvThread[ARSTREAM_READER_RESENDER_MAX_COUNT];
};


/*
 * Implementation
 */

static void* ARSTREAM_Reader_RunResenderSendThread (void *ARSTREAM_Reader2_Resender_t_Param)
{
    ARSTREAM_Reader2_Resender_t *resender = (ARSTREAM_Reader2_Resender_t *)ARSTREAM_Reader2_Resender_t_Param;

    return ARSTREAM_Reader2_Resender_RunSendThread ((void *)resender);
}

static void* ARSTREAM_Reader_RunResenderRecvThread (void *ARSTREAM_Reader2_Resender_t_Param)
{
    ARSTREAM_Reader2_Resender_t *resender = (ARSTREAM_Reader2_Resender_t *)ARSTREAM_Reader2_Resender_t_Param;

    return ARSTREAM_Reader2_Resender_RunRecvThread ((void *)resender);
}

uint8_t* ARSTREAM_Reader_Reader2NaluCallback(eARSTREAM_READER2_CAUSE cause, uint8_t *naluBuffer, int naluSize, uint64_t auTimestamp, int isFirstNaluInAu, int isLastNaluInAu, int missingPacketsBefore, int *newNaluBufferSize, void *custom)
{
    ARSTREAM_Reader_t *reader = (ARSTREAM_Reader_t *)custom;
    int iFrame = 0;
    uint8_t *retPtr = NULL;
    uint32_t newAuBufferSize, dummy = 0;
    uint8_t *newAuBuffer = NULL;

    switch (cause)
    {
        default:
        case ARSTREAM_READER2_CAUSE_NALU_COMPLETE:
            if (isLastNaluInAu)
            {
                /* Hack to declare an I-Frame if the AU starts with an SPS NALU */
                iFrame = (((*(reader->currentAuBuffer+4)) & 0x1F) == 7) ? 1 : 0;
                reader->currentAuSize += naluSize;
                reader->currentAuBuffer = reader->callback(ARSTREAM_READER_CAUSE_FRAME_COMPLETE, reader->currentAuBuffer, reader->currentAuSize, 0, iFrame, &(reader->currentAuBufferSize), reader->custom);
                reader->currentAuSize = 0;
                reader->currentNaluBuffer = reader->currentAuBuffer + reader->currentAuSize;
                reader->currentNaluBufferSize = reader->currentAuBufferSize - reader->currentAuSize;
                *newNaluBufferSize = reader->currentNaluBufferSize;
                retPtr = reader->currentNaluBuffer;
            }
            else
            {
                if ((isFirstNaluInAu) && (reader->currentAuSize > 0))
                {
                    uint8_t *tmpBuf = malloc(naluSize);
                    if (tmpBuf)
                    {
                        memcpy(tmpBuf, naluBuffer, naluSize);
                    }
                    /* Hack to declare an I-Frame if the AU starts with an SPS NALU */
                    iFrame = (((*(reader->currentAuBuffer+4)) & 0x1F) == 7) ? 1 : 0;
                    reader->currentAuBuffer = reader->callback(ARSTREAM_READER_CAUSE_FRAME_COMPLETE, reader->currentAuBuffer, reader->currentAuSize, 0, iFrame, &(reader->currentAuBufferSize), reader->custom);
                    reader->currentAuSize = 0;
                    reader->currentNaluBuffer = reader->currentAuBuffer + reader->currentAuSize;
                    reader->currentNaluBufferSize = reader->currentAuBufferSize - reader->currentAuSize;
                    if (tmpBuf)
                    {
                        memcpy(reader->currentNaluBuffer, tmpBuf, naluSize);
                        free(tmpBuf);
                    }
                }
                reader->currentAuSize += naluSize;
                reader->currentNaluBuffer = reader->currentAuBuffer + reader->currentAuSize;
                reader->currentNaluBufferSize = reader->currentAuBufferSize - reader->currentAuSize;
                *newNaluBufferSize = reader->currentNaluBufferSize;
                retPtr = reader->currentNaluBuffer;
            }
            break;
        case ARSTREAM_READER2_CAUSE_NALU_BUFFER_TOO_SMALL:
            newAuBufferSize = reader->currentAuBufferSize + *newNaluBufferSize;
            newAuBuffer = reader->callback(ARSTREAM_READER_CAUSE_FRAME_TOO_SMALL, reader->currentAuBuffer, 0, 0, 0, &newAuBufferSize, reader->custom);
            if (newAuBuffer)
            {
                if ((newAuBufferSize > 0) && (newAuBufferSize >= reader->currentAuBufferSize + *newNaluBufferSize))
                {
                    memcpy(newAuBuffer, reader->currentAuBuffer, reader->currentAuSize);
                    reader->callback(ARSTREAM_READER_CAUSE_COPY_COMPLETE, reader->currentAuBuffer, 0, 0, 0, &dummy, reader->custom);
                }
                reader->currentAuBuffer = newAuBuffer;
                reader->currentAuBufferSize = newAuBufferSize;
                reader->currentNaluBuffer = reader->currentAuBuffer + reader->currentAuSize;
                reader->currentNaluBufferSize = reader->currentAuBufferSize - reader->currentAuSize;
            }
            *newNaluBufferSize = reader->currentNaluBufferSize;
            retPtr = reader->currentNaluBuffer;
            break;
        case ARSTREAM_READER2_CAUSE_NALU_COPY_COMPLETE:
            *newNaluBufferSize = reader->currentNaluBufferSize;
            retPtr = reader->currentNaluBuffer;
            break;
        case ARSTREAM_READER2_CAUSE_CANCEL:
            break;
    }

    return retPtr;
}

void ARSTREAM_Reader_InitStreamDataBuffer (ARNETWORK_IOBufferParam_t *bufferParams, int bufferID, int maxFragmentSize, uint32_t maxNumberOfFragment)
{
    ARSTREAM_Buffers_InitStreamDataBuffer (bufferParams, bufferID, sizeof(ARSTREAM_NetworkHeaders_DataHeader_t), maxFragmentSize, maxNumberOfFragment);
}

void ARSTREAM_Reader_InitStreamAckBuffer (ARNETWORK_IOBufferParam_t *bufferParams, int bufferID)
{
    ARSTREAM_Buffers_InitStreamAckBuffer (bufferParams, bufferID);
}

ARSTREAM_Reader_t* ARSTREAM_Reader_New (ARNETWORK_Manager_t *manager, int dataBufferID, int ackBufferID, ARSTREAM_Reader_FrameCompleteCallback_t callback, uint8_t *frameBuffer, uint32_t frameBufferSize, uint32_t maxFragmentSize, int32_t maxAckInterval, void *custom, eARSTREAM_ERROR *error)
{
    ARSTREAM_Reader_t *retReader = NULL;
    eARSTREAM_ERROR internalError = ARSTREAM_OK;
    /* ARGS Check */
    if ((manager == NULL) ||
        (callback == NULL) ||
        (frameBuffer == NULL) ||
        (frameBufferSize == 0) ||
        (maxFragmentSize == 0) ||
        (maxAckInterval < -1))
    {
        SET_WITH_CHECK (error, ARSTREAM_ERROR_BAD_PARAMETERS);
        return retReader;
    }

    /* Alloc new reader */
    retReader = malloc (sizeof (ARSTREAM_Reader_t));
    if (retReader == NULL)
    {
        internalError = ARSTREAM_ERROR_ALLOC;
    }

    /* Copy parameters */
    if (internalError == ARSTREAM_OK)
    {
        memset (retReader, 0, sizeof (ARSTREAM_Reader_t));
        retReader->manager = manager;
        retReader->dataBufferID = dataBufferID;
        retReader->ackBufferID = ackBufferID;
        retReader->maxFragmentSize = maxFragmentSize;
        retReader->maxAckInterval = maxAckInterval;
        retReader->callback = callback;
        retReader->custom = custom;
        retReader->currentAuBufferSize = frameBufferSize;
        retReader->currentAuBuffer = frameBuffer;
        retReader->currentNaluBufferSize = frameBufferSize;
        retReader->currentNaluBuffer = frameBuffer;
    }

    /* Setup ARStream_Reader2 */
    if (internalError == ARSTREAM_OK)
    {
        eARSTREAM_ERROR error2;
        ARSTREAM_Reader2_Config_t config;
        memset(&config, 0, sizeof(config));
        config.ifaceAddr = NULL;
        config.recvAddr = "192.168.42.1";
        config.recvPort = 5004;
        config.recvTimeoutSec = 5;
        config.naluCallback = ARSTREAM_Reader_Reader2NaluCallback;
        config.maxPacketSize = maxFragmentSize;
        config.insertStartCodes = 1;

        retReader->reader2 = ARSTREAM_Reader2_New (&config, retReader->currentNaluBuffer, retReader->currentNaluBufferSize, (void*)retReader, &error2);
        if (error2 != ARSTREAM_OK)
        {
            internalError = error2;
        }
    }

    if (internalError == ARSTREAM_OK)
    {
        retReader->reader2running = 1;
    }

    /* Setup ARStream_Reader2_Resender */
    if (internalError == ARSTREAM_OK)
    {
        eARSTREAM_ERROR error2;
        int i;
        for (i = 0; i < ARSTREAM_READER_RESENDER_MAX_COUNT; i++)
        {
            ARSTREAM_Reader2_Resender_Config_t config;
            FILE *fConf;
            char resenderName[16], propName[20], propVal[16];
            int resenderIdx;

            memset(&config, 0, sizeof(config));
            config.ifaceAddr = NULL;
            config.sendAddr = NULL;
            config.sendPort = 5004;
            config.maxPacketSize = 1500;
            config.targetPacketSize = 1000;
            config.maxBitrate = 1500000;
            config.maxLatencyMs = 200;

            fConf = fopen("/data/skycontroller/resenders.conf", "r");
            if (fConf)
            {
                while (!feof(fConf))
                {
                    if (fscanf(fConf, "%8s%d.%s %s", resenderName, &resenderIdx, propName, propVal) == 4)
                    {
                        if ((!strncmp(resenderName, "resender", 8)) && (resenderIdx == i + 1))
                        {
                            if (!strncmp(propName, "ifaceAddr", 9))
                            {
                                config.ifaceAddr = strdup(propVal);
                            }
                            else if (!strncmp(propName, "sendAddr", 8))
                            {
                                config.sendAddr = strdup(propVal);
                            }
                            else if (!strncmp(propName, "sendPort", 8))
                            {
                                config.sendPort = atoi(propVal);
                            }
                            else if (!strncmp(propName, "maxPacketSize", 13))
                            {
                                config.maxPacketSize = atoi(propVal);
                            }
                            else if (!strncmp(propName, "targetPacketSize", 16))
                            {
                                config.targetPacketSize = atoi(propVal);
                            }
                            else if (!strncmp(propName, "maxBitrate", 10))
                            {
                                config.maxBitrate = atoi(propVal);
                            }
                            else if (!strncmp(propName, "maxLatencyMs", 12))
                            {
                                config.maxLatencyMs = atoi(propVal);
                            }
                        }
                    }
                }
                fclose(fConf);
            }

            if (config.sendAddr)
            {
                ARSAL_PRINT(ARSAL_PRINT_ERROR, ARSTREAM_READER_TAG, "Creating new resender to %s (sendPort=%d, maxPacketSize=%d, targetPacketSize=%d, maxBitrate=%d, maxLatencyMs=%d)",
                            config.sendAddr, config.sendPort, config.maxPacketSize, config.targetPacketSize, config.maxBitrate, config.maxLatencyMs);
                retReader->resender[i] = ARSTREAM_Reader2_Resender_New (retReader->reader2, &config, &error2);
                if (error2 != ARSTREAM_OK)
                {
                    internalError = error2;
                }
                else
                {
                    int ret = 0;

                    if (!ret)
                    {
                        ret = ARSAL_Thread_Create(&retReader->resenderSendThread[i], ARSTREAM_Reader_RunResenderSendThread, retReader->resender[i]);
                        if (ret)
                        {
                            internalError = ARSTREAM_ERROR_ALLOC;
                        }
                    }

                    if (!ret)
                    {
                        ret = ARSAL_Thread_Create(&retReader->resenderRecvThread[i], ARSTREAM_Reader_RunResenderRecvThread, retReader->resender[i]);
                        if (ret)
                        {
                            internalError = ARSTREAM_ERROR_ALLOC;
                        }
                    }
                }
                if (internalError != ARSTREAM_OK)
                {
                    break;
                }
            }

            if (config.ifaceAddr) free((void*)config.ifaceAddr);
            if (config.sendAddr) free((void*)config.sendAddr);
        }
    }

    if ((internalError != ARSTREAM_OK) &&
        (retReader != NULL))
    {
        ARSTREAM_Reader2_Delete(&retReader->reader2);
        int i;
        for (i = 0; i < ARSTREAM_READER_RESENDER_MAX_COUNT; i++)
        {
            if (retReader->resenderSendThread[i])
            {
                ARSAL_Thread_Destroy(&retReader->resenderSendThread[i]);
            }
            if (retReader->resenderRecvThread[i])
            {
                ARSAL_Thread_Destroy(&retReader->resenderRecvThread[i]);
            }
        }
        free(retReader);
        retReader = NULL;
    }

    SET_WITH_CHECK (error, internalError);
    return retReader;
}

void ARSTREAM_Reader_StopReader (ARSTREAM_Reader_t *reader)
{
    if ((reader != NULL) && (reader->reader2running))
    {
        ARSTREAM_Reader2_StopReader (reader->reader2);
        reader->reader2running = 0;
        int i;
        for (i = 0; i < ARSTREAM_READER_RESENDER_MAX_COUNT; i++)
        {
            if (reader->resenderSendThread[i])
            {
                ARSAL_Thread_Join(reader->resenderSendThread[i], NULL);
            }
            if (reader->resenderRecvThread[i])
            {
                ARSAL_Thread_Join(reader->resenderRecvThread[i], NULL);
            }
        }
    }
}

eARSTREAM_ERROR ARSTREAM_Reader_Delete (ARSTREAM_Reader_t **reader)
{
    eARSTREAM_ERROR retVal = ARSTREAM_ERROR_BAD_PARAMETERS;
    if ((reader != NULL) &&
        (*reader != NULL))
    {
        if (!(*reader)->reader2running)
        {
            retVal = ARSTREAM_Reader2_Delete (&((*reader)->reader2));
            int i;
            for (i = 0; i < ARSTREAM_READER_RESENDER_MAX_COUNT; i++)
            {
                if ((*reader)->resenderSendThread[i])
                {
                    ARSAL_Thread_Destroy(&(*reader)->resenderSendThread[i]);
                }
                if ((*reader)->resenderRecvThread[i])
                {
                    ARSAL_Thread_Destroy(&(*reader)->resenderRecvThread[i]);
                }
            }
            if (retVal == ARSTREAM_OK)
            {
                free (*reader);
                *reader = NULL;
            }
            else
            {
                ARSAL_PRINT(ARSAL_PRINT_ERROR, ARSTREAM_READER_TAG, "Failed to delete Reader2");
            }
        }
        else
        {
            ARSAL_PRINT(ARSAL_PRINT_ERROR, ARSTREAM_READER_TAG, "Reader2 is still running");
        }
    }
    return retVal;
}

void* ARSTREAM_Reader_RunDataThread (void *ARSTREAM_Reader_t_Param)
{
    ARSTREAM_Reader_t *reader = (ARSTREAM_Reader_t *)ARSTREAM_Reader_t_Param;

    return ARSTREAM_Reader2_RunRecvThread ((void *)reader->reader2);
}

void* ARSTREAM_Reader_RunAckThread (void *ARSTREAM_Reader_t_Param)
{
    ARSTREAM_Reader_t *reader = (ARSTREAM_Reader_t *)ARSTREAM_Reader_t_Param;

    return ARSTREAM_Reader2_RunSendThread ((void *)reader->reader2);
}

float ARSTREAM_Reader_GetEstimatedEfficiency (ARSTREAM_Reader_t *reader)
{
    if (reader == NULL)
    {
        return -1.0f;
    }
    return 1.0f;
}

void* ARSTREAM_Reader_GetCustom (ARSTREAM_Reader_t *reader)
{
    void *ret = NULL;
    if ((reader != NULL) && (reader->reader2running))
    {
        ret = ARSTREAM_Reader2_GetCustom (reader->reader2);
    }
    return ret;
}
