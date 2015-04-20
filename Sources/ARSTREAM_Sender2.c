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
 * Number of previous frames to memorize
 */
#define ARSTREAM_SENDER2_PREVIOUS_FRAME_NB_SAVE (10)

/**
 * Maximum number of pre-fragments per frame
 */
#define ARSTREAM_SENDER2_MAX_PREFRAGMENT_COUNT (128)

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

typedef struct {
    uint32_t frameNumber;
    uint32_t frameSize;
    uint8_t *frameBuffer;
    int isHighPriority;
    int fragmentCount;
    uint32_t fragmentSize[ARSTREAM_SENDER2_MAX_PREFRAGMENT_COUNT];
    uint8_t *fragmentBuffer[ARSTREAM_SENDER2_MAX_PREFRAGMENT_COUNT];
} ARSTREAM_Sender2_Frame_t;

struct ARSTREAM_Sender2_t {
    /* Configuration on New */
    ARNETWORK_Manager_t *manager;
    int dataBufferID;
    int ackBufferID;
    ARSTREAM_Sender2_FrameUpdateCallback_t callback;
    uint32_t maxNumberOfNextFrames;
    uint32_t maxFragmentSize;
    uint32_t maxNumberOfFragment;
    void *custom;

    /* Current frame storage */
    ARSTREAM_Sender2_Frame_t currentFrame;
    int currentFrameNbFragments;
    int currentFrameCbWasCalled;
    ARSAL_Mutex_t packetsToSendMutex;
    ARSTREAM_NetworkHeaders_AckPacket_t packetsToSend;

    /* Acknowledge storage */
    ARSAL_Mutex_t ackMutex;
    ARSTREAM_NetworkHeaders_AckPacket_t ackPacket;

    /* Next frame storage */
    ARSAL_Mutex_t nextFrameMutex;
    ARSAL_Cond_t  nextFrameCond;
    uint32_t nextFrameNumber;
    uint32_t indexAddNextFrame;
    uint32_t indexGetNextFrame;
    uint32_t numberOfWaitingFrames;
    ARSTREAM_Sender2_Frame_t *nextFrames;
    int frameInProgress;
    uint32_t indexFrameInProgress;

    /* Thread status */
    int threadsShouldStop;
    int dataThreadStarted;
    int ackThreadStarted;
};

typedef struct {
    ARSTREAM_Sender2_t *sender;
    uint32_t frameNumber;
    int fragmentIndex;
} ARSTREAM_Sender2_NetworkCallbackParam_t;

/*
 * Internal functions declarations
 */

/**
 * @brief Flush the new frame queue
 * @param sender The sender to flush
 * @warning Must be called within a sender->nextFrameMutex lock
 */
static void ARSTREAM_Sender2_FlushQueue (ARSTREAM_Sender2_t *sender);

/**
 * @brief Add a frame to the new frame queue
 * @param sender The sender which should send the frame
 * @param size The frame size, in bytes
 * @param buffer Pointer to the buffer which contains the frame
 * @param wasFlushFrame Boolean-like (0/1) flag, active if the frame is added after a flush (high priority frame)
 * @return the number of frames previously in queue (-1 if queue is full)
 */
static int ARSTREAM_Sender2_AddToQueue (ARSTREAM_Sender2_t *sender, uint32_t size, uint8_t *buffer, int wasFlushFrame);

/**
 * @brief Add a fragment to the frame queue
 * @param sender The sender which should send the fragment
 * @param size The fragment size, in bytes
 * @param buffer Pointer to the buffer which contains the fragment
 * @param frameUserPtr optional pointer to the frame user data (must be the same for all fragments of a frame)
 * @param isLast Boolean-like (0/1) flag, active if the fragment is the last fragment of the frame
 * @param wasFlushFrame Boolean-like (0/1) flag, active if the frame is added after a flush (high priority frame)
 * @return the number of frames previously in queue (-1 if queue is full)
 */
static int ARSTREAM_Sender2_AddFragmentToQueue (ARSTREAM_Sender2_t *sender, uint32_t size, uint8_t *buffer, void *frameUserPtr, int isLast, int wasFlushFrame);

/**
 * @brief Pop a frame from the new frame queue
 * @param sender The sender
 * @param newFrame Pointer in which the function will save the new frame infos
 * @return 1 if a new frame is available
 * @return 0 if no new frame should be sent (queue is empty, or filled with low-priority frame)
 */
static int ARSTREAM_Sender2_PopFromQueue (ARSTREAM_Sender2_t *sender, ARSTREAM_Sender2_Frame_t *newFrame);

/**
 * @brief ARNETWORK_Manager_Callback_t for ARNETWORK_... calls
 * @param IoBufferId Unused as we always send on one unique buffer
 * @param dataPtr Unused as we don't need to free the memory
 * @param customData (ARSTREAM_Sender2_NetworkCallbackParam_t *) Sender + fragment index
 * @param status Network information
 * @return ARNETWORK_MANAGER_CALLBACK_RETURN_DEFAULT
 *
 * @warning customData is a malloc'd pointer, and must be freed within this callback, during last call
 */
eARNETWORK_MANAGER_CALLBACK_RETURN ARSTREAM_Sender2_NetworkCallback (int IoBufferId, uint8_t *dataPtr, void *customData, eARNETWORK_MANAGER_CALLBACK_STATUS status);

/**
 * @brief Signals that the current frame of the sender was acknowledged
 * @param sender The sender
 */
static void ARSTREAM_Sender2_FrameWasAck (ARSTREAM_Sender2_t *sender);

/**
 * @brief Internal wrapper around the callback calls
 * This wrapper includes checks for framePointer value, and avoids calling
 * the actual callback on invalid frames
 * @param sender The sender
 * @param status Why the call was made
 * @param framePointer Pointer to the frame which was sent/cancelled
 * @param frameSize Size, in bytes, of the frame
 */
static void ARSTREAM_Sender2_CallCallback (ARSTREAM_Sender2_t *sender, eARSTREAM_SENDER2_STATUS status, uint8_t *framePointer, uint32_t frameSize);

/*
 * Internal functions implementation
 */

static void ARSTREAM_Sender2_FlushQueue (ARSTREAM_Sender2_t *sender)
{
    while (sender->numberOfWaitingFrames > 0)
    {
        ARSTREAM_Sender2_Frame_t *nextFrame = &(sender->nextFrames [sender->indexGetNextFrame]);
        ARSTREAM_Sender2_CallCallback (sender, ARSTREAM_SENDER2_STATUS_FRAME_CANCEL, nextFrame->frameBuffer, nextFrame->frameSize);
        sender->indexGetNextFrame++;
        sender->indexGetNextFrame %= sender->maxNumberOfNextFrames;
        sender->numberOfWaitingFrames--;
    }
    sender->frameInProgress = 0;
}

static int ARSTREAM_Sender2_AddToQueue (ARSTREAM_Sender2_t *sender, uint32_t size, uint8_t *buffer, int wasFlushFrame)
{
    int retVal;
    ARSAL_Mutex_Lock (&(sender->nextFrameMutex));
    retVal = sender->numberOfWaitingFrames;
    if (sender->currentFrameCbWasCalled == 0)
    {
        retVal++;
    }
    if (wasFlushFrame == 1)
    {
        ARSTREAM_Sender2_FlushQueue (sender);
    }
    if (sender->numberOfWaitingFrames < sender->maxNumberOfNextFrames)
    {
        ARSTREAM_Sender2_Frame_t *nextFrame = &(sender->nextFrames [sender->indexAddNextFrame]);
        sender->nextFrameNumber++;
        nextFrame->frameNumber = sender->nextFrameNumber;
        nextFrame->frameBuffer = buffer;
        nextFrame->frameSize   = size;
        nextFrame->isHighPriority = wasFlushFrame;
        nextFrame->fragmentCount  = 1;
        nextFrame->fragmentBuffer[0] = buffer;
        nextFrame->fragmentSize[0] = size;

        sender->indexAddNextFrame++;
        sender->indexAddNextFrame %= sender->maxNumberOfNextFrames;

        sender->numberOfWaitingFrames++;

        ARSAL_Cond_Signal (&(sender->nextFrameCond));
    }
    else
    {
        retVal = -1;
    }
    ARSAL_Mutex_Unlock (&(sender->nextFrameMutex));
    return retVal;
}

static int ARSTREAM_Sender2_AddFragmentToQueue (ARSTREAM_Sender2_t *sender, uint32_t size, uint8_t *buffer, void *frameUserPtr, int isLast, int wasFlushFrame)
{
    int retVal;
    ARSTREAM_Sender2_Frame_t *nextFrame = NULL;
    ARSAL_Mutex_Lock (&(sender->nextFrameMutex));
    retVal = sender->numberOfWaitingFrames;
    if (sender->currentFrameCbWasCalled == 0)
    {
        retVal++;
    }
    if (sender->frameInProgress == 0)
    {
        if (wasFlushFrame == 1)
        {
            ARSTREAM_Sender2_FlushQueue (sender);
        }
        if (sender->numberOfWaitingFrames < sender->maxNumberOfNextFrames)
        {
            sender->frameInProgress = 1;
            nextFrame = &(sender->nextFrames [sender->indexAddNextFrame]);
            sender->indexFrameInProgress = sender->indexAddNextFrame;
            sender->nextFrameNumber++;
            nextFrame->frameNumber = sender->nextFrameNumber;
            nextFrame->frameBuffer = frameUserPtr;
            nextFrame->frameSize   = 0;
            nextFrame->isHighPriority = wasFlushFrame;
            nextFrame->fragmentCount  = 0;
        }
        else
        {
            retVal = -1;
        }
    }
    else
    {
        nextFrame = &(sender->nextFrames [sender->indexFrameInProgress]);
    }
    if (retVal != -1)
    {
        if (nextFrame->fragmentCount < ARSTREAM_SENDER2_MAX_PREFRAGMENT_COUNT)
        {
            nextFrame->fragmentBuffer[nextFrame->fragmentCount] = buffer;
            nextFrame->fragmentSize[nextFrame->fragmentCount] = size;
            nextFrame->fragmentCount++;
            nextFrame->frameSize += size;
        }
        else
        {
            // return an error if the max number of fragments has been reached
            retVal = -2;
        }
        if ((isLast) || (nextFrame->fragmentCount >= ARSTREAM_SENDER2_MAX_PREFRAGMENT_COUNT))
        {
            // signal a new frame on the last fragment or if the max number of fragments has been reached
            sender->frameInProgress = 0;
            
            sender->indexAddNextFrame++;
            sender->indexAddNextFrame %= sender->maxNumberOfNextFrames;

            sender->numberOfWaitingFrames++;

            ARSAL_Cond_Signal (&(sender->nextFrameCond));
        }
    }
    ARSAL_Mutex_Unlock (&(sender->nextFrameMutex));
    return retVal;
}

static int ARSTREAM_Sender2_PopFromQueue (ARSTREAM_Sender2_t *sender, ARSTREAM_Sender2_Frame_t *newFrame)
{
    int retVal = 0;
    int hadTimeout = 0;
    ARSAL_Mutex_Lock (&(sender->nextFrameMutex));
    // Check if a frame is ready and of good priority
    if (sender->numberOfWaitingFrames > 0)
    {
        retVal = 1;
        sender->numberOfWaitingFrames--;
    }
    // If not, wait for a frame ready event
    if (retVal == 0)
    {
        int waitTime = 100; //TODO

        while ((retVal == 0) &&
               (hadTimeout == 0))
        {
            int err = ARSAL_Cond_Timedwait (&(sender->nextFrameCond), &(sender->nextFrameMutex), waitTime);
            if (err == ETIMEDOUT)
            {
                hadTimeout = 1;
            }
            if (sender->numberOfWaitingFrames > 0)
            {
                retVal = 1;
                sender->numberOfWaitingFrames--;
            }
        }
    }
    // If we got a new frame, copy it
    if (retVal == 1)
    {
        ARSTREAM_Sender2_Frame_t *frame = &(sender->nextFrames [sender->indexGetNextFrame]);
        sender->indexGetNextFrame++;
        sender->indexGetNextFrame %= sender->maxNumberOfNextFrames;

        newFrame->frameNumber = frame->frameNumber;
        newFrame->frameBuffer = frame->frameBuffer;
        newFrame->frameSize   = frame->frameSize;
        newFrame->isHighPriority = frame->isHighPriority;
        newFrame->fragmentCount = frame->fragmentCount;
        memcpy(newFrame->fragmentSize, frame->fragmentSize, frame->fragmentCount * sizeof(uint32_t));
        memcpy(newFrame->fragmentBuffer, frame->fragmentBuffer, frame->fragmentCount * sizeof(uint8_t*));
    }
    ARSAL_Mutex_Unlock (&(sender->nextFrameMutex));
    return retVal;
}

eARNETWORK_MANAGER_CALLBACK_RETURN ARSTREAM_Sender2_NetworkCallback (int IoBufferId, uint8_t *dataPtr, void *customData, eARNETWORK_MANAGER_CALLBACK_STATUS status)
{
    eARNETWORK_MANAGER_CALLBACK_RETURN retVal = ARNETWORK_MANAGER_CALLBACK_RETURN_DEFAULT;

    /* Get params */
    ARSTREAM_Sender2_NetworkCallbackParam_t *cbParams = (ARSTREAM_Sender2_NetworkCallbackParam_t *)customData;

    /* Get Sender */
    ARSTREAM_Sender2_t *sender = cbParams->sender;

    /* Get packetIndex */
    int packetIndex = cbParams->fragmentIndex;

    /* Get frameNumber */
    uint32_t frameNumber = cbParams->frameNumber;

    /* Remove "unused parameter" warnings */
    (void)IoBufferId;
    (void)dataPtr;

    switch (status)
    {
    case ARNETWORK_MANAGER_CALLBACK_STATUS_SENT:
        ARSAL_Mutex_Lock (&(sender->packetsToSendMutex));
        // Modify packetsToSend only if it refers to the frame we're sending
        if (frameNumber == sender->packetsToSend.frameNumber)
        {
            ARSAL_PRINT (ARSAL_PRINT_DEBUG, ARSTREAM_SENDER2_TAG, "Sent packet %d", packetIndex);
            if (1 == ARSTREAM_NetworkHeaders_AckPacketUnsetFlag (&(sender->packetsToSend), packetIndex))
            {
                ARSAL_PRINT (ARSAL_PRINT_DEBUG, ARSTREAM_SENDER2_TAG, "All packets were sent");
                ARSTREAM_Sender2_FrameWasAck(sender);
            }
        }
        else
        {
            ARSAL_PRINT (ARSAL_PRINT_DEBUG, ARSTREAM_SENDER2_TAG, "Sent a packet for an old frame [packet %d, current frame %d]", frameNumber,sender->packetsToSend.frameNumber);
        }
        ARSAL_Mutex_Unlock (&(sender->packetsToSendMutex));
        /* Free cbParams */
        free (cbParams);
        break;
    case ARNETWORK_MANAGER_CALLBACK_STATUS_CANCEL:
        /* Free cbParams */
        free (cbParams);
        break;
    default:
        break;
    }
    return retVal;
}


static void ARSTREAM_Sender2_FrameWasAck (ARSTREAM_Sender2_t *sender)
{
    ARSTREAM_Sender2_CallCallback (sender, ARSTREAM_SENDER2_STATUS_FRAME_SENT, sender->currentFrame.frameBuffer, sender->currentFrame.frameSize);
    sender->currentFrameCbWasCalled = 1;
    ARSAL_Mutex_Lock (&(sender->nextFrameMutex));
    ARSAL_Cond_Signal (&(sender->nextFrameCond));
    ARSAL_Mutex_Unlock (&(sender->nextFrameMutex));
}

static void ARSTREAM_Sender2_CallCallback (ARSTREAM_Sender2_t *sender, eARSTREAM_SENDER2_STATUS status, uint8_t *framePointer, uint32_t frameSize)
{
    int needToCall = 1;
    // Dont call if the frame is null
    if (framePointer == NULL)
    {
        needToCall = 0;
    }

    if (needToCall == 1)
    {
        sender->callback(status, framePointer, frameSize, sender->custom);
    }
}

/*
 * Implementation
 */

void ARSTREAM_Sender2_InitStreamDataBuffer (ARNETWORK_IOBufferParam_t *bufferParams, int bufferID, int maxFragmentSize, uint32_t maxFragmentPerFrame)
{
    ARSTREAM_Buffers_InitStreamDataBuffer (bufferParams, bufferID, maxFragmentSize, maxFragmentPerFrame);
}

void ARSTREAM_Sender2_InitStreamAckBuffer (ARNETWORK_IOBufferParam_t *bufferParams, int bufferID)
{
    ARSTREAM_Buffers_InitStreamAckBuffer (bufferParams, bufferID);
}

ARSTREAM_Sender2_t* ARSTREAM_Sender2_New (ARNETWORK_Manager_t *manager, int dataBufferID, int ackBufferID, ARSTREAM_Sender2_FrameUpdateCallback_t callback, uint32_t framesBufferSize, uint32_t maxFragmentSize, uint32_t maxNumberOfFragment,  void *custom, eARSTREAM_ERROR *error)
{
    ARSTREAM_Sender2_t *retSender = NULL;
    int packetsToSendMutexWasInit = 0;
    int ackMutexWasInit = 0;
    int nextFrameMutexWasInit = 0;
    int nextFrameCondWasInit = 0;
    int nextFramesArrayWasCreated = 0;
    eARSTREAM_ERROR internalError = ARSTREAM_OK;
    /* ARGS Check */
    if ((manager == NULL) ||
        (callback == NULL) ||
        (maxFragmentSize == 0) ||
        (maxNumberOfFragment > ARSTREAM_NETWORK_HEADERS_MAX_FRAGMENTS_PER_FRAME))
    {
        SET_WITH_CHECK (error, ARSTREAM_ERROR_BAD_PARAMETERS);
        return retSender;
    }

    /* Alloc new sender */
    retSender = malloc (sizeof (ARSTREAM_Sender2_t));
    if (retSender == NULL)
    {
        internalError = ARSTREAM_ERROR_ALLOC;
    }

    /* Copy parameters */
    if (internalError == ARSTREAM_OK)
    {
        retSender->manager = manager;
        retSender->dataBufferID = dataBufferID;
        retSender->ackBufferID = ackBufferID;
        retSender->callback = callback;
        retSender->custom = custom;
        retSender->maxNumberOfFragment = maxNumberOfFragment;
        retSender->maxNumberOfNextFrames = framesBufferSize;
        retSender->maxFragmentSize = maxFragmentSize;
    }

    /* Setup internal mutexes/sems */
    if (internalError == ARSTREAM_OK)
    {
        int mutexInitRet = ARSAL_Mutex_Init (&(retSender->packetsToSendMutex));
        if (mutexInitRet != 0)
        {
            internalError = ARSTREAM_ERROR_ALLOC;
        }
        else
        {
            packetsToSendMutexWasInit = 1;
        }
    }
    if (internalError == ARSTREAM_OK)
    {
        int mutexInitRet = ARSAL_Mutex_Init (&(retSender->ackMutex));
        if (mutexInitRet != 0)
        {
            internalError = ARSTREAM_ERROR_ALLOC;
        }
        else
        {
            ackMutexWasInit = 1;
        }
    }
    if (internalError == ARSTREAM_OK)
    {
        int mutexInitRet = ARSAL_Mutex_Init (&(retSender->nextFrameMutex));
        if (mutexInitRet != 0)
        {
            internalError = ARSTREAM_ERROR_ALLOC;
        }
        else
        {
            nextFrameMutexWasInit = 1;
        }
    }
    if (internalError == ARSTREAM_OK)
    {
        int condInitRet = ARSAL_Cond_Init (&(retSender->nextFrameCond));
        if (condInitRet != 0)
        {
            internalError = ARSTREAM_ERROR_ALLOC;
        }
        else
        {
            nextFrameCondWasInit = 1;
        }
    }

    /* Allocate next frame storage */
    if (internalError == ARSTREAM_OK)
    {
        retSender->nextFrames = malloc (framesBufferSize * sizeof (ARSTREAM_Sender2_Frame_t));
        if (retSender->nextFrames == NULL)
        {
            internalError = ARSTREAM_ERROR_ALLOC;
        }
        else
        {
            nextFramesArrayWasCreated = 1;
        }
    }

    /* Setup internal variables */
    if (internalError == ARSTREAM_OK)
    {
        int i;
        retSender->currentFrame.frameNumber = 0;
        retSender->currentFrame.frameBuffer = NULL;
        retSender->currentFrame.frameSize   = 0;
        retSender->currentFrame.isHighPriority = 0;
        retSender->currentFrame.fragmentCount = 0;
        retSender->currentFrameNbFragments = 0;
        retSender->currentFrameCbWasCalled = 0;
        retSender->nextFrameNumber = 0;
        retSender->indexAddNextFrame = 0;
        retSender->indexGetNextFrame = 0;
        retSender->numberOfWaitingFrames = 0;
        retSender->frameInProgress = 0;
        retSender->indexFrameInProgress = 0;
        retSender->threadsShouldStop = 0;
        retSender->dataThreadStarted = 0;
        retSender->ackThreadStarted = 0;
    }

    if ((internalError != ARSTREAM_OK) &&
        (retSender != NULL))
    {
        if (packetsToSendMutexWasInit == 1)
        {
            ARSAL_Mutex_Destroy (&(retSender->packetsToSendMutex));
        }
        if (ackMutexWasInit == 1)
        {
            ARSAL_Mutex_Destroy (&(retSender->ackMutex));
        }
        if (nextFrameMutexWasInit == 1)
        {
            ARSAL_Mutex_Destroy (&(retSender->nextFrameMutex));
        }
        if (nextFrameCondWasInit == 1)
        {
            ARSAL_Cond_Destroy (&(retSender->nextFrameCond));
        }
        if (nextFramesArrayWasCreated == 1)
        {
            free (retSender->nextFrames);
        }
        free (retSender);
        retSender = NULL;
    }

    SET_WITH_CHECK (error, internalError);
    return retSender;
}


void ARSTREAM_Sender2_StopSender (ARSTREAM_Sender2_t *sender)
{
    if (sender != NULL)
    {
        sender->threadsShouldStop = 1;
    }
}

eARSTREAM_ERROR ARSTREAM_Sender2_Delete (ARSTREAM_Sender2_t **sender)
{
    eARSTREAM_ERROR retVal = ARSTREAM_ERROR_BAD_PARAMETERS;
    if ((sender != NULL) &&
        (*sender != NULL))
    {
        int canDelete = 0;
        if (((*sender)->dataThreadStarted == 0) &&
            ((*sender)->ackThreadStarted == 0))
        {
            canDelete = 1;
        }

        if (canDelete == 1)
        {
            ARSTREAM_Sender2_FlushQueue (*sender);
            ARSAL_Mutex_Destroy (&((*sender)->packetsToSendMutex));
            ARSAL_Mutex_Destroy (&((*sender)->ackMutex));
            ARSAL_Mutex_Destroy (&((*sender)->nextFrameMutex));
            ARSAL_Cond_Destroy (&((*sender)->nextFrameCond));
            free ((*sender)->nextFrames);
            free (*sender);
            *sender = NULL;
            retVal = ARSTREAM_OK;
        }
        else
        {
            ARSAL_PRINT (ARSAL_PRINT_ERROR, ARSTREAM_SENDER2_TAG, "Call ARSTREAM_Sender2_StopSender before calling this function");
            retVal = ARSTREAM_ERROR_BUSY;
        }
    }
    return retVal;
}

eARSTREAM_ERROR ARSTREAM_Sender2_SendNewFrame (ARSTREAM_Sender2_t *sender, uint8_t *frameBuffer, uint32_t frameSize, int flushPreviousFrames, int *nbPreviousFrames)
{
    eARSTREAM_ERROR retVal = ARSTREAM_OK;
    // Args check
    if ((sender == NULL) ||
        (frameBuffer == NULL) ||
        (frameSize == 0) ||
        ((flushPreviousFrames != 0) &&
         (flushPreviousFrames != 1)))
    {
        retVal = ARSTREAM_ERROR_BAD_PARAMETERS;
    }
    if ((retVal == ARSTREAM_OK) &&
        (frameSize > (sender->maxFragmentSize * sender->maxNumberOfFragment)))
    {
        retVal = ARSTREAM_ERROR_FRAME_TOO_LARGE;
    }

    if (retVal == ARSTREAM_OK)
    {
        int res = ARSTREAM_Sender2_AddToQueue (sender, frameSize, frameBuffer, flushPreviousFrames);
        if (res < 0)
        {
            retVal = ARSTREAM_ERROR_QUEUE_FULL;
        }
        else if (nbPreviousFrames != NULL)
        {
            *nbPreviousFrames = res;
        }
        // No else : do nothing if the nbPreviousFrames pointer is not set
    }
    return retVal;
}

eARSTREAM_ERROR ARSTREAM_Sender2_SendNewFragment (ARSTREAM_Sender2_t *sender, uint8_t *fragmentBuffer, uint32_t fragmentSize, void *frameUserPtr, int isLast, int flushPreviousFrames, int *nbPreviousFrames)
{
    eARSTREAM_ERROR retVal = ARSTREAM_OK;
    // Args check
    if ((sender == NULL) ||
        (fragmentBuffer == NULL) ||
        (fragmentSize == 0) ||
        ((flushPreviousFrames != 0) &&
         (flushPreviousFrames != 1)))
    {
        retVal = ARSTREAM_ERROR_BAD_PARAMETERS;
    }
    if ((retVal == ARSTREAM_OK) &&
        (fragmentSize > (sender->maxFragmentSize * sender->maxNumberOfFragment)))
    {
        retVal = ARSTREAM_ERROR_FRAME_TOO_LARGE;
    }

    if (retVal == ARSTREAM_OK)
    {
        int res = ARSTREAM_Sender2_AddFragmentToQueue (sender, fragmentSize, fragmentBuffer, frameUserPtr, isLast, flushPreviousFrames);
        if (res < 0)
        {
            retVal = ARSTREAM_ERROR_QUEUE_FULL;
        }
        else if (nbPreviousFrames != NULL)
        {
            *nbPreviousFrames = res;
        }
        // No else : do nothing if the nbPreviousFrames pointer is not set
    }
    return retVal;
}

eARSTREAM_ERROR ARSTREAM_Sender2_FlushFramesQueue (ARSTREAM_Sender2_t *sender)
{
    eARSTREAM_ERROR retVal = ARSTREAM_OK;
    if (sender == NULL)
    {
        retVal = ARSTREAM_ERROR_BAD_PARAMETERS;
    }
    if (retVal == ARSTREAM_OK)
    {
        ARSAL_Mutex_Lock (&(sender->nextFrameMutex));
        ARSTREAM_Sender2_FlushQueue (sender);
        ARSAL_Mutex_Unlock (&(sender->nextFrameMutex));
    }
    return retVal;
}

void* ARSTREAM_Sender2_RunDataThread (void *ARSTREAM_Sender2_t_Param)
{
    /* Local declarations */
    ARSTREAM_Sender2_t *sender = (ARSTREAM_Sender2_t *)ARSTREAM_Sender2_t_Param;
    uint8_t *sendFragment = NULL;
    uint32_t sendSize = 0;
    uint16_t nbPackets = 0, nbPacketsFragment = 0;
    int cnt, cntFragment, fragmentIdx;
    int numbersOfFragmentsSentForCurrentFrame = 0;
    int lastFragmentSize = 0;
    ARSTREAM_NetworkHeaders_DataHeader_t *header = NULL;
    ARSTREAM_Sender2_Frame_t nextFrame = {0};
    int firstFrame = 1;

    /* Parameters check */
    if (sender == NULL)
    {
        ARSAL_PRINT (ARSAL_PRINT_ERROR, ARSTREAM_SENDER2_TAG, "Error while starting %s, bad parameters", __FUNCTION__);
        return (void *)0;
    }

    /* Alloc and check */
    sendFragment = malloc (sender->maxFragmentSize + sizeof (ARSTREAM_NetworkHeaders_DataHeader_t));
    if (sendFragment == NULL)
    {
        ARSAL_PRINT (ARSAL_PRINT_ERROR, ARSTREAM_SENDER2_TAG, "Error while starting %s, can not alloc memory", __FUNCTION__);
        return (void *)0;
    }
    header = (ARSTREAM_NetworkHeaders_DataHeader_t *)sendFragment;

    ARSAL_PRINT (ARSAL_PRINT_DEBUG, ARSTREAM_SENDER2_TAG, "Sender thread running");
    sender->dataThreadStarted = 1;

    while (sender->threadsShouldStop == 0)
    {
        int waitRes;
        waitRes = ARSTREAM_Sender2_PopFromQueue (sender, &nextFrame);
        // Check again if we should be stopping (after the wait)
        if (sender->threadsShouldStop != 0)
        {
            break;
        }
        ARSAL_Mutex_Lock (&(sender->ackMutex));
        if (waitRes == 1)
        {
            ARSAL_PRINT (ARSAL_PRINT_DEBUG, ARSTREAM_SENDER2_TAG, "Previous frame was sent in %d packets. Frame size was %d packets", numbersOfFragmentsSentForCurrentFrame, nbPackets);
            numbersOfFragmentsSentForCurrentFrame = 0;
            /* We have a new frame to send */
            ARSAL_PRINT (ARSAL_PRINT_DEBUG, ARSTREAM_SENDER2_TAG, "New frame needs to be sent");

            /* Cancel current frame if it was not already sent */
            /* Do not do it for the first "NULL" frame that is in the
             * ARStream Sender before any call to SendNewFrame */
            if (sender->currentFrameCbWasCalled == 0 && firstFrame == 0)
            {
                ARSAL_PRINT (ARSAL_PRINT_DEBUG, ARSTREAM_SENDER2_TAG, "Canceled frame #%d", sender->currentFrame.frameNumber);

                ARNETWORK_Manager_FlushInputBuffer (sender->manager, sender->dataBufferID);

                ARSTREAM_Sender2_CallCallback(sender, ARSTREAM_SENDER2_STATUS_FRAME_CANCEL, sender->currentFrame.frameBuffer, sender->currentFrame.frameSize);
            }
            sender->currentFrameCbWasCalled = 0; // New frame
            firstFrame = 0;

            /* Save next frame data into current frame data */
            sender->currentFrame.frameNumber = nextFrame.frameNumber;
            sender->currentFrame.frameBuffer = nextFrame.frameBuffer;
            sender->currentFrame.frameSize   = nextFrame.frameSize;
            sender->currentFrame.isHighPriority = nextFrame.isHighPriority;
            sender->currentFrame.fragmentCount = nextFrame.fragmentCount;
            if (sender->currentFrame.fragmentCount > 0)
            {
                memcpy(sender->currentFrame.fragmentSize, nextFrame.fragmentSize, sender->currentFrame.fragmentCount * sizeof(uint32_t));
                memcpy(sender->currentFrame.fragmentBuffer, nextFrame.fragmentBuffer, sender->currentFrame.fragmentCount * sizeof(uint8_t*));
            }
            for (fragmentIdx = 0, sendSize = 0; fragmentIdx < sender->currentFrame.fragmentCount; fragmentIdx++)
            {
                sendSize += sender->currentFrame.fragmentSize[fragmentIdx];
            }

            /* Reset packetsToSend - update frame number */
            ARSAL_Mutex_Lock (&(sender->packetsToSendMutex));
            sender->packetsToSend.frameNumber = sender->currentFrame.frameNumber;
            ARSTREAM_NetworkHeaders_AckPacketReset (&(sender->packetsToSend));
            ARSAL_Mutex_Unlock (&(sender->packetsToSendMutex));

            /* Update stream data header with the new frame number */
            header->frameNumber = sender->currentFrame.frameNumber;
            header->frameFlags = 0;
            header->frameFlags |= (sender->currentFrame.isHighPriority != 0) ? ARSTREAM_NETWORK_HEADERS_FLAG_FLUSH_FRAME : 0;

            /* Compute total number of fragments (pre-fragments and post-fragments) */
            for (fragmentIdx = 0, nbPackets = 0; fragmentIdx < sender->currentFrame.fragmentCount; fragmentIdx++)
            {
                nbPackets += sender->currentFrame.fragmentSize[fragmentIdx] / sender->maxFragmentSize;
                if (sender->currentFrame.fragmentSize[fragmentIdx] % sender->maxFragmentSize)
                {
                    nbPackets++;
                }
            }
            sender->currentFrameNbFragments = nbPackets;

            ARSAL_PRINT (ARSAL_PRINT_DEBUG, ARSTREAM_SENDER2_TAG, "New frame has size %d (=%d packets)", sendSize, nbPackets);
        }
        ARSAL_Mutex_Unlock (&(sender->ackMutex));
        /* END OF NEW FRAME BLOCK */

        /* Flag all packets as "packet to send" */
        ARSAL_Mutex_Lock (&(sender->packetsToSendMutex));
        ARSAL_Mutex_Lock (&(sender->ackMutex));
        ARSTREAM_NetworkHeaders_AckPacketReset (&(sender->packetsToSend));
        for (cnt = 0; cnt < nbPackets; cnt++)
        {
            ARSTREAM_NetworkHeaders_AckPacketSetFlag (&(sender->packetsToSend), cnt);
        }

        /* Send all "packets to send" */
        for (fragmentIdx = 0, cnt = 0; fragmentIdx < sender->currentFrame.fragmentCount; fragmentIdx++)
        {
            uint32_t maxFragSize = sender->maxFragmentSize;
            nbPacketsFragment = sender->currentFrame.fragmentSize[fragmentIdx] / maxFragSize;
            lastFragmentSize = maxFragSize;
            if (sender->currentFrame.fragmentSize[fragmentIdx] % maxFragSize)
            {
                nbPacketsFragment++;
                lastFragmentSize = sender->currentFrame.fragmentSize[fragmentIdx] % sender->maxFragmentSize;
            }
            for (cntFragment = 0; cntFragment < nbPacketsFragment; cntFragment++, cnt++)
            {
                if (ARSTREAM_NetworkHeaders_AckPacketFlagIsSet (&(sender->packetsToSend), cnt))
                {
                    eARNETWORK_ERROR netError = ARNETWORK_OK;
                    numbersOfFragmentsSentForCurrentFrame ++;
                    int currFragmentSize = (cntFragment == nbPacketsFragment-1) ? lastFragmentSize : maxFragSize;
                    header->fragmentNumber = cnt;
                    header->fragmentsPerFrame = nbPackets;
                    memcpy (&sendFragment[sizeof (ARSTREAM_NetworkHeaders_DataHeader_t)], &(sender->currentFrame.fragmentBuffer[fragmentIdx])[maxFragSize*cntFragment], currFragmentSize);
                    ARSTREAM_Sender2_NetworkCallbackParam_t *cbParams = malloc (sizeof (ARSTREAM_Sender2_NetworkCallbackParam_t));
                    cbParams->sender = sender;
                    cbParams->fragmentIndex = cnt;
                    cbParams->frameNumber = sender->packetsToSend.frameNumber;
                    ARSAL_Mutex_Unlock (&(sender->packetsToSendMutex));
                    netError = ARNETWORK_Manager_SendData (sender->manager, sender->dataBufferID, sendFragment, currFragmentSize + sizeof (ARSTREAM_NetworkHeaders_DataHeader_t), (void *)cbParams, ARSTREAM_Sender2_NetworkCallback, 1);
                    if (netError != ARNETWORK_OK)
                    {
                        ARSAL_PRINT (ARSAL_PRINT_ERROR, ARSTREAM_SENDER2_TAG, "Error occurred during sending of the fragment ; error: %d : %s", netError, ARNETWORK_Error_ToString(netError));
                    }
                    ARSAL_Mutex_Lock (&(sender->packetsToSendMutex));
                }
            }
        }
        ARSAL_Mutex_Unlock (&(sender->ackMutex));
        ARSAL_Mutex_Unlock (&(sender->packetsToSendMutex));
    }
    /* END OF PROCESS LOOP */

    if (sender->currentFrameCbWasCalled == 0 && firstFrame == 0)
    {
        ARSAL_PRINT (ARSAL_PRINT_DEBUG, ARSTREAM_SENDER2_TAG, "Canceled frame #%d", sender->currentFrame.frameNumber);

        ARSTREAM_Sender2_CallCallback (sender, ARSTREAM_SENDER2_STATUS_FRAME_CANCEL, sender->currentFrame.frameBuffer, sender->currentFrame.frameSize);
    }

    ARSAL_PRINT (ARSAL_PRINT_DEBUG, ARSTREAM_SENDER2_TAG, "Sender thread ended");
    sender->dataThreadStarted = 0;

    if(sendFragment)
    {
        free(sendFragment);
        sendFragment = NULL;
    }

    return (void *)0;
}


void* ARSTREAM_Sender2_RunAckThread (void *ARSTREAM_Sender2_t_Param)
{
    ARSTREAM_Sender2_t *sender = (ARSTREAM_Sender2_t *)ARSTREAM_Sender2_t_Param;

    ARSAL_PRINT (ARSAL_PRINT_DEBUG, ARSTREAM_SENDER2_TAG, "Ack thread running");
    sender->ackThreadStarted = 1;

    ARSAL_PRINT (ARSAL_PRINT_DEBUG, ARSTREAM_SENDER2_TAG, "Ack thread ended");
    sender->ackThreadStarted = 0;
    return (void *)0;
}

void* ARSTREAM_Sender2_GetCustom (ARSTREAM_Sender2_t *sender)
{
    void *ret = NULL;
    if (sender != NULL)
    {
        ret = sender->custom;
    }
    return ret;
}
