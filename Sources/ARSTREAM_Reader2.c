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

#define ARSTREAM_VIDEO_OUTPUT_INCOMPLETE_FRAMES

//#define ARSTREAM_VIDEO_OUTPUT_DUMP
#ifdef ARSTREAM_VIDEO_OUTPUT_DUMP
    const char* ARSTREAM_VIDEO_OUTPUT_DUMP_PATH_NAP_USB = "/tmp/mnt/STREAMDUMP";
    const char* ARSTREAM_VIDEO_OUTPUT_DUMP_PATH_NAP_INTERNAL = "/data/skycontroller/streams";
    const char* ARSTREAM_VIDEO_OUTPUT_DUMP_PATH_ANDROID_INTERNAL = "/storage/emulated/legacy/FF";
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
    ARNETWORK_Manager_t *manager;
    int dataBufferID;
    int ackBufferID;
    uint32_t maxFragmentSize;
    ARSTREAM_Reader2_FrameCompleteCallback_t callback;
    void *custom;

    /* Current frame storage */
    uint32_t currentFrameBufferSize; // Usable length of the buffer
    uint32_t currentFrameSize;       // Actual data length
    uint8_t *currentFrameBuffer;

    /* Acknowledge storage */
    ARSTREAM_NetworkHeaders_AckPacket_t ackPacket; //TODO: delete?

    /* Thread status */
    int threadsShouldStop;
    int dataThreadStarted;
    int ackThreadStarted;

#ifdef ARSTREAM_VIDEO_OUTPUT_DUMP
    FILE* outputDumpFile;
#endif
};

/*
 * Internal functions declarations
 */

/**
 * @brief ARNETWORK_Manager_Callback_t for ARNETWORK_... calls
 * @param IoBufferId Unused as we always send on one unique buffer
 * @param dataPtr Unused as we don't need to free the memory
 * @param customData Unused
 * @param status Unused
 * @return ARNETWORK_MANAGER_CALLBACK_RETURN_DEFAULT
 */
eARNETWORK_MANAGER_CALLBACK_RETURN ARSTREAM_Reader2_NetworkCallback (int IoBufferId, uint8_t *dataPtr, void *customData, eARNETWORK_MANAGER_CALLBACK_STATUS status);

/*
 * Internal functions implementation
 */

//TODO: Network, NULL callback should be ok ?
eARNETWORK_MANAGER_CALLBACK_RETURN ARSTREAM_Reader2_NetworkCallback (int IoBufferId, uint8_t *dataPtr, void *customData, eARNETWORK_MANAGER_CALLBACK_STATUS status)
{
    /* Avoid "unused parameters" warnings */
    (void)IoBufferId;
    (void)dataPtr;
    (void)customData;
    (void)status;

    /* Dummy return value */
    return ARNETWORK_MANAGER_CALLBACK_RETURN_DEFAULT;
}

/*
 * Implementation
 */

void ARSTREAM_Reader2_InitStreamDataBuffer (ARNETWORK_IOBufferParam_t *bufferParams, int bufferID, int maxFragmentSize, uint32_t maxNumberOfFragment)
{
    ARSTREAM_Buffers_InitStreamDataBuffer (bufferParams, bufferID, maxFragmentSize, maxNumberOfFragment);
}

void ARSTREAM_Reader2_InitStreamAckBuffer (ARNETWORK_IOBufferParam_t *bufferParams, int bufferID)
{
    ARSTREAM_Buffers_InitStreamAckBuffer (bufferParams, bufferID);
}

ARSTREAM_Reader2_t* ARSTREAM_Reader2_New (ARNETWORK_Manager_t *manager, int dataBufferID, int ackBufferID, ARSTREAM_Reader2_FrameCompleteCallback_t callback, uint8_t *frameBuffer, uint32_t frameBufferSize, uint32_t maxFragmentSize, void *custom, eARSTREAM_ERROR *error)
{
    ARSTREAM_Reader2_t *retReader = NULL;
    eARSTREAM_ERROR internalError = ARSTREAM_OK;
    /* ARGS Check */
    if ((manager == NULL) ||
        (callback == NULL) ||
        (frameBuffer == NULL) ||
        (frameBufferSize == 0) ||
        (maxFragmentSize == 0))
    {
        SET_WITH_CHECK (error, ARSTREAM_ERROR_BAD_PARAMETERS);
        return retReader;
    }

    /* Alloc new reader */
    retReader = malloc (sizeof (ARSTREAM_Reader2_t));
    if (retReader == NULL)
    {
        internalError = ARSTREAM_ERROR_ALLOC;
    }

    /* Copy parameters */
    if (internalError == ARSTREAM_OK)
    {
        retReader->manager = manager;
        retReader->dataBufferID = dataBufferID;
        retReader->ackBufferID = ackBufferID;
        retReader->maxFragmentSize = maxFragmentSize;
        retReader->callback = callback;
        retReader->custom = custom;
        retReader->currentFrameBufferSize = frameBufferSize;
        retReader->currentFrameBuffer = frameBuffer;
    }

    /* Setup internal variables */
    if (internalError == ARSTREAM_OK)
    {
        int i;
        retReader->currentFrameSize = 0;
        retReader->threadsShouldStop = 0;
        retReader->dataThreadStarted = 0;
        retReader->ackThreadStarted = 0;
#ifdef ARSTREAM_VIDEO_OUTPUT_DUMP
        retReader->outputDumpFile = NULL;
#endif
    }

    if ((internalError != ARSTREAM_OK) &&
        (retReader != NULL))
    {
        free (retReader);
        retReader = NULL;
    }

    SET_WITH_CHECK (error, internalError);
    return retReader;
}

void ARSTREAM_Reader2_StopReader (ARSTREAM_Reader2_t *reader)
{
    if (reader != NULL)
    {
        reader->threadsShouldStop = 1;
    }
}

eARSTREAM_ERROR ARSTREAM_Reader2_Delete (ARSTREAM_Reader2_t **reader)
{
    eARSTREAM_ERROR retVal = ARSTREAM_ERROR_BAD_PARAMETERS;
    if ((reader != NULL) &&
        (*reader != NULL))
    {
        int canDelete = 0;
        if (((*reader)->dataThreadStarted == 0) &&
            ((*reader)->ackThreadStarted == 0))
        {
            canDelete = 1;
        }

        if (canDelete == 1)
        {
            free (*reader);
            *reader = NULL;
            retVal = ARSTREAM_OK;
        }
        else
        {
            ARSAL_PRINT (ARSAL_PRINT_ERROR, ARSTREAM_READER2_TAG, "Call ARSTREAM_Reader2_StopReader before calling this function");
            retVal = ARSTREAM_ERROR_BUSY;
        }
    }
    return retVal;
}

void* ARSTREAM_Reader2_RunDataThread (void *ARSTREAM_Reader2_t_Param)
{
    uint8_t *recvData = NULL;
    int recvSize, endIndex = 0;
    uint16_t previousFNum = UINT16_MAX;
    uint8_t previousFrameFlags = 0, previousFragmentsPerFrame = 0;
    int skipCurrentFrame = 0;
    ARSTREAM_Reader2_t *reader = (ARSTREAM_Reader2_t *)ARSTREAM_Reader2_t_Param;
    ARSTREAM_NetworkHeaders_DataHeader_t *header = NULL;
    int recvDataLen = reader->maxFragmentSize + sizeof (ARSTREAM_NetworkHeaders_DataHeader_t);

    /* Parameters check */
    if (reader == NULL)
    {
        ARSAL_PRINT (ARSAL_PRINT_ERROR, ARSTREAM_READER2_TAG, "Error while starting %s, bad parameters", __FUNCTION__);
        return (void *)0;
    }

    /* Alloc and check */
    recvData = malloc (recvDataLen);
    if (recvData == NULL)
    {
        ARSAL_PRINT (ARSAL_PRINT_ERROR, ARSTREAM_READER2_TAG, "Error while starting %s, can not alloc memory", __FUNCTION__);
        return (void *)0;
    }
    header = (ARSTREAM_NetworkHeaders_DataHeader_t *)recvData;

#ifdef ARSTREAM_VIDEO_OUTPUT_DUMP
    int i;
    char szFilename[128];
    const char* pszFilePath = NULL;
    szFilename[0] = '\0';

    if ((access(ARSTREAM_VIDEO_OUTPUT_DUMP_PATH_NAP_USB, F_OK) == 0) && (access(ARSTREAM_VIDEO_OUTPUT_DUMP_PATH_NAP_USB, W_OK) == 0))
    {
        pszFilePath = ARSTREAM_VIDEO_OUTPUT_DUMP_PATH_NAP_USB;
    }
    else if ((access(ARSTREAM_VIDEO_OUTPUT_DUMP_PATH_NAP_INTERNAL, F_OK) == 0) && (access(ARSTREAM_VIDEO_OUTPUT_DUMP_PATH_NAP_INTERNAL, W_OK) == 0))
    {
        pszFilePath = ARSTREAM_VIDEO_OUTPUT_DUMP_PATH_NAP_INTERNAL;
    }
    else if ((access(ARSTREAM_VIDEO_OUTPUT_DUMP_PATH_ANDROID_INTERNAL, F_OK) == 0) && (access(ARSTREAM_VIDEO_OUTPUT_DUMP_PATH_ANDROID_INTERNAL, W_OK) == 0))
    {
        pszFilePath = ARSTREAM_VIDEO_OUTPUT_DUMP_PATH_ANDROID_INTERNAL;
    }
    if (pszFilePath)
    {
        for (i = 0; i < 100; i++)
        {
            snprintf(szFilename, 128, "%s/stream_%02d.264", pszFilePath, i);
            if (access(szFilename, F_OK) == -1)
            {
                // file does not exist
                break;
            }
            szFilename[0] = '\0';
        }
        if (strlen(szFilename))
        {
            reader->outputDumpFile = fopen(szFilename, "wb");
        }
    }
#endif

    ARSAL_PRINT (ARSAL_PRINT_DEBUG, ARSTREAM_READER2_TAG, "Stream reader thread running");
    reader->dataThreadStarted = 1;

    while (reader->threadsShouldStop == 0)
    {
        eARNETWORK_ERROR err = ARNETWORK_Manager_ReadDataWithTimeout (reader->manager, reader->dataBufferID, recvData, recvDataLen, &recvSize, ARSTREAM_READER2_DATAREAD_TIMEOUT_MS);
        if (ARNETWORK_OK != err)
        {
            if (ARNETWORK_ERROR_BUFFER_EMPTY != err)
            {
                ARSAL_PRINT (ARSAL_PRINT_ERROR, ARSTREAM_READER2_TAG, "Error while reading stream data: %s", ARNETWORK_Error_ToString (err));
            }
        }
        else
        {
            int cpIndex, cpSize;
ARSAL_PRINT (ARSAL_PRINT_ERROR, ARSTREAM_READER2_TAG, "==== Received frameNumber %d, fragment %d/%d", header->frameNumber, header->fragmentNumber, header->fragmentsPerFrame);
            if (header->frameNumber != reader->ackPacket.frameNumber)
            {
                uint32_t nackPackets = ARSTREAM_NetworkHeaders_AckPacketCountNotSet (&(reader->ackPacket), previousFragmentsPerFrame);
                if (nackPackets != 0)
                {
#ifdef ARSTREAM_VIDEO_OUTPUT_INCOMPLETE_FRAMES
                    int nbMissedFrame = 0;
                    int isFlushFrame = ((previousFrameFlags & ARSTREAM_NETWORK_HEADERS_FLAG_FLUSH_FRAME) != 0) ? 1 : 0;
                    if (reader->ackPacket.frameNumber != previousFNum + 1)
                    {
                        nbMissedFrame = header->frameNumber - previousFNum - 1;
                        ARSAL_PRINT (ARSAL_PRINT_ERROR, ARSTREAM_READER2_TAG, "======== Missed %d frames before frame #%d", nbMissedFrame, reader->ackPacket.frameNumber);
                    }
                    previousFNum = reader->ackPacket.frameNumber;
                    ARSAL_PRINT (ARSAL_PRINT_ERROR, ARSTREAM_READER2_TAG, "======== Output incomplete frame #%d (missing %d fragments)", reader->ackPacket.frameNumber, nackPackets);
#ifdef ARSTREAM_VIDEO_OUTPUT_DUMP
                    if (reader->outputDumpFile)
                    {
                        fwrite(reader->currentFrameBuffer, reader->currentFrameSize, 1, reader->outputDumpFile);
                    }
#endif
                    reader->currentFrameBuffer = reader->callback (ARSTREAM_READER2_CAUSE_FRAME_INCOMPLETE, reader->currentFrameBuffer, reader->currentFrameSize, nbMissedFrame, (int)nackPackets, previousFragmentsPerFrame, isFlushFrame, &(reader->currentFrameBufferSize), reader->custom);
#else
                    ARSAL_PRINT (ARSAL_PRINT_ERROR, ARSTREAM_READER2_TAG, "======== Dropping incomplete frame #%d (missing %d fragments)", reader->ackPacket.frameNumber, nackPackets);
#endif
                }
                skipCurrentFrame = 0;
                reader->currentFrameSize = 0;
                endIndex = 0;
                reader->ackPacket.frameNumber = header->frameNumber;
                ARSTREAM_NetworkHeaders_AckPacketResetUpTo (&(reader->ackPacket), header->fragmentsPerFrame);
            }
            ARSTREAM_NetworkHeaders_AckPacketSetFlag (&(reader->ackPacket), header->fragmentNumber);
            previousFrameFlags = header->frameFlags;
            previousFragmentsPerFrame = header->fragmentsPerFrame;

            cpSize = recvSize - sizeof (ARSTREAM_NetworkHeaders_DataHeader_t);
            cpIndex = endIndex;
            endIndex += cpSize;
            while ((endIndex > reader->currentFrameBufferSize) &&
                   (skipCurrentFrame == 0))
            {
                uint32_t nextFrameBufferSize = endIndex;
                uint32_t dummy;
                uint8_t *nextFrameBuffer = reader->callback (ARSTREAM_READER2_CAUSE_FRAME_TOO_SMALL, reader->currentFrameBuffer, reader->currentFrameSize, 0, 0, 0, 0, &nextFrameBufferSize, reader->custom);
                if (nextFrameBufferSize >= reader->currentFrameSize && nextFrameBufferSize > 0)
                {
                    memcpy (nextFrameBuffer, reader->currentFrameBuffer, reader->currentFrameSize);
                }
                else
                {
                    skipCurrentFrame = 1;
                }
                //TODO: Add "SKIP_FRAME"
                reader->callback (ARSTREAM_READER2_CAUSE_COPY_COMPLETE, reader->currentFrameBuffer, reader->currentFrameSize, 0, 0, 0, skipCurrentFrame, &dummy, reader->custom);
                reader->currentFrameBuffer = nextFrameBuffer;
                reader->currentFrameBufferSize = nextFrameBufferSize;
            }

            if (skipCurrentFrame == 0)
            {
                memcpy (&(reader->currentFrameBuffer)[cpIndex], &recvData[sizeof (ARSTREAM_NetworkHeaders_DataHeader_t)], cpSize);
                reader->currentFrameSize = endIndex;

                if (ARSTREAM_NetworkHeaders_AckPacketAllFlagsSet (&(reader->ackPacket), header->fragmentsPerFrame))
                {
                    if (header->frameNumber != previousFNum)
                    {
                        int nbMissedFrame = 0;
                        int isFlushFrame = ((header->frameFlags & ARSTREAM_NETWORK_HEADERS_FLAG_FLUSH_FRAME) != 0) ? 1 : 0;
                        ARSAL_PRINT (ARSAL_PRINT_DEBUG, ARSTREAM_READER2_TAG, "Ack all in frame %d (isFlush : %d)", header->frameNumber, isFlushFrame);
                        if (header->frameNumber != previousFNum + 1)
                        {
                            nbMissedFrame = header->frameNumber - previousFNum - 1;
                            ARSAL_PRINT (ARSAL_PRINT_ERROR, ARSTREAM_READER2_TAG, "======== Missed %d frames before frame #%d", nbMissedFrame, header->frameNumber);
                        }
                        previousFNum = header->frameNumber;
                        skipCurrentFrame = 1;
#ifdef ARSTREAM_VIDEO_OUTPUT_DUMP
                        if (reader->outputDumpFile)
                        {
                            fwrite(reader->currentFrameBuffer, reader->currentFrameSize, 1, reader->outputDumpFile);
                        }
#endif
                        reader->currentFrameBuffer = reader->callback (ARSTREAM_READER2_CAUSE_FRAME_COMPLETE, reader->currentFrameBuffer, reader->currentFrameSize, nbMissedFrame, 0, header->fragmentsPerFrame, isFlushFrame, &(reader->currentFrameBufferSize), reader->custom);
                    }
                }
            }
        }
    }

    free (recvData);

    reader->callback (ARSTREAM_READER2_CAUSE_CANCEL, reader->currentFrameBuffer, reader->currentFrameSize, 0, 0, 0, 0, &(reader->currentFrameBufferSize), reader->custom);

#ifdef ARSTREAM_VIDEO_OUTPUT_DUMP
    if (reader->outputDumpFile)
    {
        fclose(reader->outputDumpFile);
        reader->outputDumpFile = NULL;
    }
#endif

    ARSAL_PRINT (ARSAL_PRINT_DEBUG, ARSTREAM_READER2_TAG, "Stream reader thread ended");
    reader->dataThreadStarted = 0;
    return (void *)0;
}

void* ARSTREAM_Reader2_RunAckThread (void *ARSTREAM_Reader2_t_Param)
{
    ARSTREAM_Reader2_t *reader = (ARSTREAM_Reader2_t *)ARSTREAM_Reader2_t_Param;

    reader->ackThreadStarted = 0;
    return (void *)0;
}

void* ARSTREAM_Reader2_GetCustom (ARSTREAM_Reader2_t *reader)
{
    void *ret = NULL;
    if (reader != NULL)
    {
        ret = reader->custom;
    }
    return ret;
}
