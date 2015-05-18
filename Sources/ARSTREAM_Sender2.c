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
 * Maximum number of NAL units in the FIFO
 */
#define ARSTREAM_SENDER2_DEFAULT_NALU_FIFO_SIZE (128 * 32)

/**
 * SSRC identifier ("ARST")
 */
#define ARSTREAM_SENDER2_SSRC 0x41525354

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
    ARNETWORK_Manager_t *manager;
    int dataBufferID;
    int ackBufferID;
    ARSTREAM_Sender2_AuCallback_t auCallback;
    int maxPacketSize;
    int naluFifoSize;
    void *custom;

    uint64_t lastAuTimestamp;
    ARSAL_Mutex_t streamMutex;

    /* Thread status */
    int threadsShouldStop;
    int dataThreadStarted;
    int ackThreadStarted;

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
    int i;
    ARSTREAM_Sender2_Nalu_t* cur = NULL;

    if (!sender)
    {
        return -1;
    }

    ARSAL_Mutex_Lock(&(sender->fifoMutex));

    if (sender->fifoHead)
    {
        for (cur = sender->fifoHead; cur; cur = cur->next)
        {
            /* cancel the Access Unit */
            ARSAL_Mutex_Lock(&(sender->streamMutex));
            if (cur->auTimestamp != sender->lastAuTimestamp)
            {
                sender->lastAuTimestamp = cur->auTimestamp;
                ARSAL_Mutex_Unlock(&(sender->streamMutex));

                /* call the auCallback */
                ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARSTREAM_SENDER2_TAG, "Time %llu: cancel frame -> auCallback", cur->auTimestamp);
                sender->auCallback(ARSTREAM_SENDER2_STATUS_AU_CANCELLED, cur->auUserPtr, sender->custom);
            }
            else
            {
                ARSAL_Mutex_Unlock(&(sender->streamMutex));
            }
        }
    }

    for (i = 0; i < sender->naluFifoSize; i++)
    {
        cur = &sender->fifoPool[i];
        cur->prev = NULL;
        cur->next = NULL;
        cur->used = 0;
    }
    sender->fifoCount = 0;
    sender->fifoHead = NULL;
    sender->fifoTail = NULL;

    ARSAL_Mutex_Unlock(&(sender->fifoMutex));

    return 0;
}


eARNETWORK_MANAGER_CALLBACK_RETURN ARSTREAM_Sender2_NetworkCallback(int IoBufferId, uint8_t *dataPtr, void *customData, eARNETWORK_MANAGER_CALLBACK_STATUS status)
{
    eARNETWORK_MANAGER_CALLBACK_RETURN retVal = ARNETWORK_MANAGER_CALLBACK_RETURN_DEFAULT;

    if (!customData)
    {
        return retVal;
    }

    /* Get params */
    ARSTREAM_Sender2_NetworkCallbackParam_t *cbParams = (ARSTREAM_Sender2_NetworkCallbackParam_t*)customData;

    /* Get Sender */
    ARSTREAM_Sender2_t *sender = cbParams->sender;

    /* Remove "unused parameter" warnings */
    (void)IoBufferId;
    (void)dataPtr;

    switch (status)
    {
    case ARNETWORK_MANAGER_CALLBACK_STATUS_SENT:
        ARSAL_Mutex_Lock(&(sender->streamMutex));
        if ((cbParams->isLastInAu) && (cbParams->auTimestamp != sender->lastAuTimestamp))
        {
            sender->lastAuTimestamp = cbParams->auTimestamp;
            ARSAL_Mutex_Unlock(&(sender->streamMutex));
            ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARSTREAM_SENDER2_TAG, "Time %llu: all packets were sent -> auCallback", cbParams->auTimestamp);
            sender->auCallback(ARSTREAM_SENDER2_STATUS_AU_SENT, cbParams->auUserPtr, sender->custom);
        }
        else
        {
            ARSAL_Mutex_Unlock(&(sender->streamMutex));
        }
        /* Free cbParams */
        free(cbParams);
        break;
    case ARNETWORK_MANAGER_CALLBACK_STATUS_CANCEL:
        /* Free cbParams */
        free(cbParams);
        break;
    default:
        break;
    }
    return retVal;
}


void ARSTREAM_Sender2_InitStreamDataBuffer(ARNETWORK_IOBufferParam_t *bufferParams, int bufferID, int maxPacketSize)
{
    ARSTREAM_Buffers_InitStreamDataBuffer(bufferParams, bufferID, maxPacketSize, ARSTREAM_BUFFERS_DATA_BUFFER_NUMBER_OF_CELLS);
}


void ARSTREAM_Sender2_InitStreamAckBuffer(ARNETWORK_IOBufferParam_t *bufferParams, int bufferID)
{
    ARSTREAM_Buffers_InitStreamAckBuffer(bufferParams, bufferID);
}


ARSTREAM_Sender2_t* ARSTREAM_Sender2_New(ARNETWORK_Manager_t *manager, int dataBufferID, int ackBufferID, ARSTREAM_Sender2_AuCallback_t auCallback, int naluFifoSize, int maxPacketSize, void *custom, eARSTREAM_ERROR *error)
{
    ARSTREAM_Sender2_t *retSender = NULL;
    int streamMutexWasInit = 0;
    int fifoWasCreated = 0;
    int fifoMutexWasInit = 0;
    int fifoCondWasInit = 0;
    eARSTREAM_ERROR internalError = ARSTREAM_OK;
    /* ARGS Check */
    if ((manager == NULL) ||
        (auCallback == NULL) ||
        (maxPacketSize == 0) ||
        (naluFifoSize == 0))
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

    /* Copy parameters */
    if (internalError == ARSTREAM_OK)
    {
        retSender->manager = manager;
        retSender->dataBufferID = dataBufferID;
        retSender->ackBufferID = ackBufferID;
        retSender->auCallback = auCallback;
        retSender->naluFifoSize = naluFifoSize;
        retSender->maxPacketSize = maxPacketSize;
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
        retSender->fifoPool = malloc(naluFifoSize * sizeof(ARSTREAM_Sender2_Nalu_t));
        if (retSender->fifoPool == NULL)
        {
            internalError = ARSTREAM_ERROR_ALLOC;
        }
        else
        {
            fifoWasCreated = 1;
            memset(retSender->fifoPool, 0, naluFifoSize * sizeof(ARSTREAM_Sender2_Nalu_t));
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

    /* Setup internal variables */
    if (internalError == ARSTREAM_OK)
    {
        retSender->threadsShouldStop = 0;
        retSender->dataThreadStarted = 0;
        retSender->ackThreadStarted = 0;
        retSender->fifoCount = 0;
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
            ARSTREAM_Sender2_FlushNaluFifo(*sender);
            ARSAL_Mutex_Destroy(&((*sender)->streamMutex));
            free((*sender)->fifoPool);
            ARSAL_Mutex_Destroy(&((*sender)->fifoMutex));
            ARSAL_Cond_Destroy(&((*sender)->fifoCond));
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
    if ((retVal == ARSTREAM_OK) &&
        (naluSize + sizeof(ARSTREAM_NetworkHeaders_DataHeader2_t) > sender->maxPacketSize))
    {
        retVal = ARSTREAM_ERROR_FRAME_TOO_LARGE;
    }

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


void* ARSTREAM_Sender2_RunDataThread (void *ARSTREAM_Sender2_t_Param)
{
    /* Local declarations */
    ARSTREAM_Sender2_t *sender = (ARSTREAM_Sender2_t*)ARSTREAM_Sender2_t_Param;
    uint8_t *sendBuffer = NULL;
    uint32_t sendSize = 0;
    ARSTREAM_NetworkHeaders_DataHeader2_t *header = NULL;
    ARSTREAM_Sender2_Nalu_t nalu;
    uint64_t previousTimestamp = 0, firstTimestamp = 0;
    uint16_t seqNum = 0;
    uint16_t flags;
    void *previousAuUserPtr = NULL;
    int shouldStop;

    /* Parameters check */
    if (sender == NULL)
    {
        ARSAL_PRINT(ARSAL_PRINT_ERROR, ARSTREAM_SENDER2_TAG, "Error while starting %s, bad parameters", __FUNCTION__);
        return (void *)0;
    }

    /* Alloc and check */
    sendBuffer = malloc(sender->maxPacketSize);
    if (sendBuffer == NULL)
    {
        ARSAL_PRINT(ARSAL_PRINT_ERROR, ARSTREAM_SENDER2_TAG, "Error while starting %s, cannot allocate memory", __FUNCTION__);
        return (void*)0;
    }
    header = (ARSTREAM_NetworkHeaders_DataHeader2_t *)sendBuffer;

    ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARSTREAM_SENDER2_TAG, "Sender thread running");
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
            eARNETWORK_ERROR netError = ARNETWORK_OK;
            memcpy(sendBuffer + sizeof(ARSTREAM_NetworkHeaders_DataHeader2_t), nalu.naluBuffer, nalu.naluSize);
            sendSize = nalu.naluSize + sizeof(ARSTREAM_NetworkHeaders_DataHeader2_t);

            /* header */
            flags = 0x8060; /* with PT=96 */
            if (nalu.isLastInAu)
            {
                /* set the marker bit */
                flags |= (1 << 7);
            }
            header->flags = htons(flags);
            header->seqNum = htons(seqNum++);
            if (firstTimestamp == 0)
            {
                firstTimestamp = nalu.auTimestamp;
            }
            header->timestamp = htonl((uint32_t)(((((nalu.auTimestamp - firstTimestamp) * 90) + 500) / 1000) & 0xFFFFFFFF)); /* microseconds to 90000 Hz clock */
            header->ssrc = htonl(ARSTREAM_SENDER2_SSRC);

            /* send to the network layer */
            ARSTREAM_Sender2_NetworkCallbackParam_t *cbParams = malloc(sizeof(ARSTREAM_Sender2_NetworkCallbackParam_t));
            //TODO: avoid malloc
            cbParams->sender = sender;
            cbParams->auTimestamp = nalu.auTimestamp;
            cbParams->isLastInAu = nalu.isLastInAu;
            cbParams->auUserPtr = nalu.auUserPtr;
            netError = ARNETWORK_Manager_SendData(sender->manager, sender->dataBufferID, sendBuffer, sendSize, (void*)cbParams, ARSTREAM_Sender2_NetworkCallback, 1); //TODO: nocopy
            if (netError != ARNETWORK_OK)
            {
                ARSAL_PRINT(ARSAL_PRINT_ERROR, ARSTREAM_SENDER2_TAG, "Error occurred during sending of the fragment ; error: %d : %s", netError, ARNETWORK_Error_ToString(netError));
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

    ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARSTREAM_SENDER2_TAG, "Sender thread ended");
    ARSAL_Mutex_Lock(&(sender->streamMutex));
    sender->dataThreadStarted = 0;
    ARSAL_Mutex_Unlock(&(sender->streamMutex));

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

