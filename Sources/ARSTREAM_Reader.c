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

/*
 * Macros
 */

#define ARSTREAM_READER_TAG "ARSTREAM_Reader"

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

    /* ARStream_Reader2 */
    ARSTREAM_Reader2_t *reader2;
};


/*
 * Implementation
 */

uint8_t* ARSTREAM_Reader_Reader2AuCallback(eARSTREAM_READER2_CAUSE cause2, uint8_t *auBuffer, int auSize, uint64_t auTimestamp, int missingPackets, int totalPackets, int *newAuBufferSize, void *custom)
{
    ARSTREAM_Reader_t *reader = (ARSTREAM_Reader_t *)custom;
    eARSTREAM_READER_CAUSE cause;
    int iFrame = 0;

    switch (cause2)
    {
        default:
        case ARSTREAM_READER2_CAUSE_AU_COMPLETE:
        case ARSTREAM_READER2_CAUSE_AU_INCOMPLETE:
            cause = ARSTREAM_READER_CAUSE_FRAME_COMPLETE;
            break;
        case ARSTREAM_READER2_CAUSE_AU_BUFFER_TOO_SMALL:
            cause = ARSTREAM_READER_CAUSE_FRAME_TOO_SMALL;
            break;
        case ARSTREAM_READER2_CAUSE_AU_COPY_COMPLETE:
            cause = ARSTREAM_READER_CAUSE_COPY_COMPLETE;
            break;
        case ARSTREAM_READER2_CAUSE_CANCEL:
            cause = ARSTREAM_READER_CAUSE_CANCEL;
            break;
    }

    /* Hack to declare an I-Frame if the AU starts with an SPS NALU */
    if (cause == ARSTREAM_READER_CAUSE_FRAME_COMPLETE)
    {
        iFrame = (*(auBuffer+4) == 0x27) ? 1 : 0;
    }

    return reader->callback (cause, auBuffer, auSize, 0, iFrame, newAuBufferSize, reader->custom);
}

void ARSTREAM_Reader_InitStreamDataBuffer (ARNETWORK_IOBufferParam_t *bufferParams, int bufferID, int maxFragmentSize, uint32_t maxNumberOfFragment)
{
    ARSTREAM_Reader2_InitStreamDataBuffer (bufferParams, bufferID, maxFragmentSize);
}

void ARSTREAM_Reader_InitStreamAckBuffer (ARNETWORK_IOBufferParam_t *bufferParams, int bufferID)
{
    ARSTREAM_Reader2_InitStreamAckBuffer (bufferParams, bufferID);
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
        retReader->manager = manager;
        retReader->dataBufferID = dataBufferID;
        retReader->ackBufferID = ackBufferID;
        retReader->maxFragmentSize = maxFragmentSize;
        retReader->maxAckInterval = maxAckInterval;
        retReader->callback = callback;
        retReader->custom = custom;
    }

    /* Setup ARStream_Reader2 */
    if (internalError == ARSTREAM_OK)
    {
        eARSTREAM_ERROR error2;
        ARSTREAM_Reader2_Config_t config;
        memset(&config, 0, sizeof(config));
        config.networkMode = ARSTREAM_READER2_NETWORK_MODE_SOCKET;
        config.manager = manager;
        config.dataBufferID = dataBufferID;
        config.ackBufferID = ackBufferID;
        config.ifaceAddr = NULL;
        config.recvAddr = "192.168.42.1";
        config.recvPort = 5004;
        config.recvTimeoutSec = 5;
        config.auCallback = ARSTREAM_Reader_Reader2AuCallback;
        config.maxPacketSize = maxFragmentSize;
        config.insertStartCodes = 1;
        config.outputIncompleteAu = 1;

        retReader->reader2 = ARSTREAM_Reader2_New (&config, frameBuffer, frameBufferSize, (void*)retReader, &error2);
        if (error2 != ARSTREAM_OK)
        {
            internalError = error2;
        }
    }

    SET_WITH_CHECK (error, internalError);
    return retReader;
}

void ARSTREAM_Reader_StopReader (ARSTREAM_Reader_t *reader)
{
    if (reader != NULL)
    {
        ARSTREAM_Reader2_StopReader (reader->reader2);
    }
}

eARSTREAM_ERROR ARSTREAM_Reader_Delete (ARSTREAM_Reader_t **reader)
{
    eARSTREAM_ERROR retVal = ARSTREAM_ERROR_BAD_PARAMETERS;
    if ((reader != NULL) &&
        (*reader != NULL))
    {
        retVal = ARSTREAM_Reader2_Delete (&((*reader)->reader2));
        if (retVal == ARSTREAM_OK)
        {
            free (*reader);
            *reader = NULL;
        }
    }
    return retVal;
}

void* ARSTREAM_Reader_RunDataThread (void *ARSTREAM_Reader_t_Param)
{
    ARSTREAM_Reader_t *reader = (ARSTREAM_Reader_t *)ARSTREAM_Reader_t_Param;

    return ARSTREAM_Reader2_RunDataThread ((void *)reader->reader2);
}

void* ARSTREAM_Reader_RunAckThread (void *ARSTREAM_Reader_t_Param)
{
    ARSTREAM_Reader_t *reader = (ARSTREAM_Reader_t *)ARSTREAM_Reader_t_Param;

    return ARSTREAM_Reader2_RunAckThread ((void *)reader->reader2);
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
    if (reader != NULL)
    {
        ret = ARSTREAM_Reader2_GetCustom (reader->reader2);
    }
    return ret;
}
