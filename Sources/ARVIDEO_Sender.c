/**
 * @file ARVIDEO_Sender.c
 * @brief Video sender over network
 * @date 03/21/2013
 * @author nicolas.brulez@parrot.com
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

#include "ARVIDEO_Buffers.h"
#include "ARVIDEO_NetworkHeaders.h"

/*
 * ARSDK Headers
 */

#include <libARVideo/ARVIDEO_Sender.h>
#include <libARSAL/ARSAL_Mutex.h>
#include <libARSAL/ARSAL_Print.h>
#include <libARSAL/ARSAL_Endianness.h>

/*
 * Macros
 */

#define ARVIDEO_SENDER_TAG "ARVIDEO_Sender"

/*
 * Types
 */

struct ARVIDEO_Sender_t {
    /* Configuration on New */
    ARNETWORK_Manager_t *manager;
    int dataBufferID;
    int ackBufferID;
    ARVIDEO_Sender_FrameUpdateCallback_t callback;

    /* Current frame storage */
    uint32_t currentFrameNumber;
    uint8_t *currentFrameBuffer;
    uint32_t currentFrameSize;
    int currentFrameNbFragments;
    int currentFrameCbWasCalled;
    ARSAL_Mutex_t callbackMutex;
    ARSAL_Mutex_t packetsToSendMutex;
    ARVIDEO_NetworkHeaders_AckPacket_t packetsToSend;

    /* Acknowledge storage */
    ARSAL_Mutex_t ackMutex;
    ARVIDEO_NetworkHeaders_AckPacket_t ackPacket;

    /* Next frame storage */
    ARSAL_Mutex_t nextFrameMutex;
    uint32_t nextFrameNumber;
    uint8_t *nextFrameBuffer;
    uint32_t nextFrameSize;

    /* All sent / next frame condition + flags */
    ARSAL_Mutex_t hasNextMutex;
    ARSAL_Cond_t hasNextCond;
    int hasNextFrame;
    int allSent;

    /* Thread status */
    int threadsShouldStop;
    int dataThreadStarted;
    int ackThreadStarted;
};

typedef struct {
    ARVIDEO_Sender_t *sender;
    uint32_t frameNumber;
    int fragmentIndex;
} ARVIDEO_Sender_NetworkCallbackParam_t;

/*
 * Internal functions declarations
 */

/**
 * @brief Checks if the sender already has a new frame pending
 * @param sender The sender to test
 * @return 0 if no frame is pending
 * @return 1 if a new frame was already pending
 * @return -1 (and errno set) on error
 */
static int ARVIDEO_Sender_CheckForNewFrame (ARVIDEO_Sender_t *sender);

/**
 * @brief Signals the sender that a new frame is available
 * @param sender The sender that should send the new frame
 */
static void ARVIDEO_Sender_SignalNewFrameAvailable (ARVIDEO_Sender_t *sender);

/**
 * @brief Waits for either new frame or frame sending complete
 * @return 0 if we don't have a new frame
 * @return 1 if we have a new frame
 */
static int ARVIDEO_Sender_WaitForNewFrameOrSend (ARVIDEO_Sender_t *sender);

/**
 * @brief Signals that the current sends are complete
 */
static void ARVIDEO_Sender_SignalSendsComplete (ARVIDEO_Sender_t *sender);

/**
 * @brief ARNETWORK_Manager_Callback_t for ARNETWORK_... calls
 * @param IoBufferId Unused as we always send on one unique buffer
 * @param dataPtr Unused as we don't need to free the memory
 * @param customData (ARVIDEO_Sender_NetworkCallbackParam_t *) Sender + fragment index
 * @param status Network information
 * @return ARNETWORK_MANAGER_CALLBACK_RETURN_DEFAULT
 *
 * @warning customData is a malloc'd pointer, and must be freed within this callback, during last call
 */
eARNETWORK_MANAGER_CALLBACK_RETURN ARVIDEO_Sender_NetworkCallback (int IoBufferId, uint8_t *dataPtr, void *customData, eARNETWORK_MANAGER_CALLBACK_STATUS status);

/*
 * Internal functions implementation
 */

static int ARVIDEO_Sender_CheckForNewFrame (ARVIDEO_Sender_t *sender)
{
    int retVal = 0;
    ARSAL_Mutex_Lock (&(sender->hasNextMutex));
    retVal = sender->hasNextFrame;
    ARSAL_Mutex_Unlock (&(sender->hasNextMutex));
    return retVal;
}

static void ARVIDEO_Sender_SignalNewFrameAvailable (ARVIDEO_Sender_t *sender)
{
    /* No arg check becase this function is always called by arg-checked functions */
    ARSAL_Mutex_Lock (&(sender->hasNextMutex));
    sender->hasNextFrame = 1;
    ARSAL_Mutex_Unlock (&(sender->hasNextMutex));
    ARSAL_Cond_Signal (&(sender->hasNextCond));
}

static int ARVIDEO_Sender_WaitForNewFrameOrSend (ARVIDEO_Sender_t *sender)
{
    int needToWait;
    int retVal;
    // Compute "needToWait" + get first retVal value
    ARSAL_Mutex_Lock (&(sender->hasNextMutex));
    if (sender->hasNextFrame == 1 ||
        sender->allSent == 1)
    {
        // We have a signal pending, dont wait
        needToWait = 0;
        retVal = sender->hasNextFrame;
        sender->hasNextFrame = 0;
        sender->allSent = 0;
    }
    else
    {
        // No signal pending, wait on condition
        needToWait = 1;
    }

    if (needToWait == 1)
    {
        // Wait then copy/reset flags
        ARSAL_Cond_Wait (&(sender->hasNextCond), &(sender->hasNextMutex));
        retVal = sender->hasNextFrame;
        sender->hasNextFrame = 0;
        sender->allSent = 0;
    }
    ARSAL_Mutex_Unlock (&(sender->hasNextMutex));
    return retVal;
}

static void ARVIDEO_Sender_SignalSendsComplete (ARVIDEO_Sender_t *sender)
{
    /* No arg check becase this function is always called by arg-checked functions */
    ARSAL_Mutex_Lock (&(sender->hasNextMutex));
    sender->allSent = 1;
    ARSAL_Mutex_Unlock (&(sender->hasNextMutex));
    ARSAL_Cond_Signal (&(sender->hasNextCond));
}

eARNETWORK_MANAGER_CALLBACK_RETURN ARVIDEO_Sender_NetworkCallback (int IoBufferId, uint8_t *dataPtr, void *customData, eARNETWORK_MANAGER_CALLBACK_STATUS status)
{
    eARNETWORK_MANAGER_CALLBACK_RETURN retVal = ARNETWORK_MANAGER_CALLBACK_RETURN_DEFAULT;

    /* Get params */
    ARVIDEO_Sender_NetworkCallbackParam_t *cbParams = (ARVIDEO_Sender_NetworkCallbackParam_t *)customData;

    /* Get Sender */
    ARVIDEO_Sender_t *sender = cbParams->sender;

    /* Get packetIndex */
    int packetIndex = cbParams->fragmentIndex;

    /* Get frameNumber */
    uint32_t frameNumber = cbParams->frameNumber;

    /* Remove "unused parameter" warnings */
    IoBufferId = IoBufferId;
    dataPtr = dataPtr;

    switch (status)
    {
    case ARNETWORK_MANAGER_CALLBACK_STATUS_SENT:
        ARSAL_Mutex_Lock (&(sender->packetsToSendMutex));
        // Modify packetsToSend only if it refers to the frame we're sending
        if (frameNumber == sender->packetsToSend.numFrame)
        {
            //ARSAL_PRINT (ARSAL_PRINT_DEBUG, ARVIDEO_SENDER_TAG, "Sent/free packet %d", packetIndex);
            if (1 == ARVIDEO_NetworkHeaders_AckPacketUnsetFlag (&(sender->packetsToSend), packetIndex))
            {
                //ARSAL_PRINT (ARSAL_PRINT_DEBUG, ARVIDEO_SENDER_TAG, "All packets were sent");
                ARVIDEO_Sender_SignalSendsComplete (sender);
            }
        }
        else
        {
            //ARSAL_PRINT (ARSAL_PRINT_DEBUG, ARVIDEO_SENDER_TAG, "Sent a packet for an old frame");
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

/*
 * Implementation
 */

void ARVIDEO_Sender_InitVideoDataBuffer (ARNETWORK_IOBufferParam_t *bufferParams, int bufferID)
{
    ARVIDEO_Buffers_InitVideoDataBuffer (bufferParams, bufferID);
}

void ARVIDEO_Sender_InitVideoAckBuffer (ARNETWORK_IOBufferParam_t *bufferParams, int bufferID)
{
    ARVIDEO_Buffers_InitVideoAckBuffer (bufferParams, bufferID);
}

ARVIDEO_Sender_t* ARVIDEO_Sender_New (ARNETWORK_Manager_t *manager, int dataBufferID, int ackBufferID, ARVIDEO_Sender_FrameUpdateCallback_t callback)
{
    ARVIDEO_Sender_t *retSender = NULL;
    int stillValid = 1;
    int callbackMutexWasInit = 0;
    int packetsToSendMutexWasInit = 0;
    int ackMutexWasInit = 0;
    int nextFrameMutexWasInit = 0;
    int hasNextMutexWasInit = 0;
    int hasNextCondWasInit = 0;
    /* ARGS Check */
    if ((manager == NULL) ||
        (callback == NULL))
    {
        return retSender;
    }

    /* Alloc new sender */
    retSender = malloc (sizeof (ARVIDEO_Sender_t));
    if (retSender == NULL)
    {
        stillValid = 0;
    }

    /* Copy parameters */
    if (stillValid == 1)
    {
        retSender->manager = manager;
        retSender->dataBufferID = dataBufferID;
        retSender->ackBufferID = ackBufferID;
        retSender->callback = callback;
    }

    /* Setup internal mutexes/sems */
    if (stillValid == 1)
    {
        int mutexInitRet = ARSAL_Mutex_Init (&(retSender->callbackMutex));
        if (mutexInitRet != 0)
        {
            stillValid = 0;
        }
        else
        {
            callbackMutexWasInit = 1;
        }
    }
    if (stillValid == 1)
    {
        int mutexInitRet = ARSAL_Mutex_Init (&(retSender->packetsToSendMutex));
        if (mutexInitRet != 0)
        {
            stillValid = 0;
        }
        else
        {
            packetsToSendMutexWasInit = 1;
        }
    }
    if (stillValid == 1)
    {
        int mutexInitRet = ARSAL_Mutex_Init (&(retSender->ackMutex));
        if (mutexInitRet != 0)
        {
            stillValid = 0;
        }
        else
        {
            ackMutexWasInit = 1;
        }
    }
    if (stillValid == 1)
    {
        int mutexInitRet = ARSAL_Mutex_Init (&(retSender->nextFrameMutex));
        if (mutexInitRet != 0)
        {
            stillValid = 0;
        }
        else
        {
            nextFrameMutexWasInit = 1;
        }
    }
    if (stillValid == 1)
    {
        int mutexInitRet = ARSAL_Mutex_Init (&(retSender->hasNextMutex));
        if (mutexInitRet != 0)
        {
            stillValid = 0;
        }
        else
        {
            hasNextMutexWasInit = 1;
        }
    }
    if (stillValid == 1)
    {
        int condInitRet = ARSAL_Cond_Init (&(retSender->hasNextCond));
        if (condInitRet != 0)
        {
            stillValid = 0;
        }
        else
        {
            hasNextCondWasInit = 1;
        }
    }

    /* Setup internal variables */
    if (stillValid == 1)
    {
        retSender->currentFrameNumber = 0;
        retSender->currentFrameBuffer = NULL;
        retSender->currentFrameSize = 0;
        retSender->currentFrameNbFragments = 0;
        retSender->currentFrameCbWasCalled = 0;
        retSender->nextFrameNumber = 0;
        retSender->nextFrameBuffer = NULL;
        retSender->nextFrameSize = 0;
        retSender->hasNextFrame = 0;
        retSender->allSent = 0;
        retSender->threadsShouldStop = 0;
        retSender->dataThreadStarted = 0;
        retSender->ackThreadStarted = 0;
    }

    if ((stillValid == 0) &&
        (retSender != NULL))
    {
        if (callbackMutexWasInit == 1)
        {
            ARSAL_Mutex_Destroy (&(retSender->callbackMutex));
        }
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
        if (hasNextMutexWasInit)
        {
            ARSAL_Mutex_Destroy (&(retSender->hasNextMutex));
        }
        if (hasNextCondWasInit)
        {
            ARSAL_Cond_Destroy (&(retSender->hasNextCond));
        }
        free (retSender);
        retSender = NULL;
    }

    return retSender;
}

void ARVIDEO_Sender_StopSender (ARVIDEO_Sender_t *sender)
{
    if (sender != NULL)
    {
        sender->threadsShouldStop = 1;
    }
}

int ARVIDEO_Sender_Delete (ARVIDEO_Sender_t **sender)
{
    int retVal = -1;
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
            ARSAL_Mutex_Destroy (&((*sender)->callbackMutex));
            ARSAL_Mutex_Destroy (&((*sender)->packetsToSendMutex));
            ARSAL_Mutex_Destroy (&((*sender)->ackMutex));
            ARSAL_Mutex_Destroy (&((*sender)->nextFrameMutex));
            ARSAL_Mutex_Destroy (&((*sender)->hasNextMutex));
            ARSAL_Cond_Destroy (&((*sender)->hasNextCond));
            free (*sender);
            *sender = NULL;
        }
        retVal = canDelete;
    }
    return retVal;
}

int ARVIDEO_Sender_SendNewFrame (ARVIDEO_Sender_t *sender, uint8_t *frameBuffer, uint32_t frameSize)
{
    int res = -1;
    if (sender != NULL)
    {
        ARSAL_Mutex_Lock (&(sender->nextFrameMutex));

        /* If a new frame was pending, cancel it and replace with current frame */
        res = ARVIDEO_Sender_CheckForNewFrame (sender);
        if (res == 1)
        {
            sender->callback (ARVIDEO_SENDER_STATUS_FRAME_CANCEL, sender->nextFrameBuffer, sender->nextFrameSize);
        }

        sender->nextFrameNumber++;
        sender->nextFrameBuffer = frameBuffer;
        sender->nextFrameSize = frameSize;

        /* Signal sender thread that a new frame is available */
        ARVIDEO_Sender_SignalNewFrameAvailable (sender);

        ARSAL_Mutex_Unlock (&(sender->nextFrameMutex));
    }
    return res;
}

void* ARVIDEO_Sender_RunDataThread (void *ARVIDEO_Sender_t_Param)
{
    /* Local declarations */
    ARVIDEO_Sender_t *sender = (ARVIDEO_Sender_t *)ARVIDEO_Sender_t_Param;
    uint8_t *sendFragment = NULL;
    uint32_t sendSize = 0;
    uint16_t nbPackets = 0;
    int cnt;
    int lastFragmentSize = 0;
    ARVIDEO_NetworkHeaders_DataHeader_t *header = NULL;

    /* Parameters check */
    if (sender == NULL)
    {
        ARSAL_PRINT (ARSAL_PRINT_ERROR, ARVIDEO_SENDER_TAG, "Error while starting %s, bad parameters", __FUNCTION__);
        return (void *)0;
    }

    /* Alloc and check */
    sendFragment = malloc (ARVIDEO_NETWORK_HEADERS_FRAGMENT_SIZE + sizeof (ARVIDEO_NetworkHeaders_DataHeader_t));
    if (sendFragment == NULL)
    {
        ARSAL_PRINT (ARSAL_PRINT_ERROR, ARVIDEO_SENDER_TAG, "Error while starting %s, can not alloc memory", __FUNCTION__);
        return (void *)0;
    }
    header = (ARVIDEO_NetworkHeaders_DataHeader_t *)sendFragment;

    ARSAL_PRINT (ARSAL_PRINT_DEBUG, ARVIDEO_SENDER_TAG, "Sender thread running");
    sender->dataThreadStarted = 1;

    while (sender->threadsShouldStop == 0)
    {
        int waitRes = ARVIDEO_Sender_WaitForNewFrameOrSend (sender);
        if (waitRes == 1)
        {
            /* We have a new frame to send */
            ARSAL_PRINT (ARSAL_PRINT_DEBUG, ARVIDEO_SENDER_TAG, "New frame needs to be sent");

            /* Cancel current frame if it was not already sent */
            ARSAL_Mutex_Lock (&(sender->callbackMutex));
            if (sender->currentFrameCbWasCalled == 0)
            {

                ARSAL_Mutex_Lock (&(sender->ackMutex));
                ARVIDEO_NetworkHeaders_AckPacketDump ("Cancel frame:", &(sender->ackPacket));
                ARSAL_Mutex_Unlock (&(sender->ackMutex));

                sender->callback (ARVIDEO_SENDER_STATUS_FRAME_CANCEL, sender->currentFrameBuffer, sender->currentFrameSize);
            }
            sender->currentFrameCbWasCalled = 0; // New frame
            ARSAL_Mutex_Unlock (&(sender->callbackMutex));

            /* Save next frame data into current frame data */
            ARSAL_Mutex_Lock (&(sender->nextFrameMutex));
            sender->currentFrameSize = sender->nextFrameSize;
            sender->currentFrameBuffer = sender->nextFrameBuffer;
            sender->currentFrameNumber = sender->nextFrameNumber;
            ARSAL_Mutex_Unlock (&(sender->nextFrameMutex));
            sendSize = sender->currentFrameSize;

            /* Reset ack packet - No packets are ack on the new frame */
            ARSAL_Mutex_Lock (&(sender->ackMutex));
            sender->ackPacket.numFrame = sender->currentFrameNumber;
            ARVIDEO_NetworkHeaders_AckPacketReset (&(sender->ackPacket));
            ARSAL_Mutex_Unlock (&(sender->ackMutex));

            /* Reset packetsToSend - update frame number */
            ARSAL_Mutex_Lock (&(sender->packetsToSendMutex));
            sender->packetsToSend.numFrame = sender->currentFrameNumber;
            ARVIDEO_NetworkHeaders_AckPacketReset (&(sender->packetsToSend));
            ARSAL_Mutex_Unlock (&(sender->packetsToSendMutex));

            /* Update video data header with the new frame number */
            header->frameNumber = sender->currentFrameNumber;

            /* Compute number of fragments / size of the last fragment */
            if (0 < sendSize)
            {
                lastFragmentSize = ARVIDEO_NETWORK_HEADERS_FRAGMENT_SIZE;
                nbPackets = sendSize / ARVIDEO_NETWORK_HEADERS_FRAGMENT_SIZE;
                if (sendSize % ARVIDEO_NETWORK_HEADERS_FRAGMENT_SIZE)
                {
                    nbPackets++;
                    lastFragmentSize = sendSize % ARVIDEO_NETWORK_HEADERS_FRAGMENT_SIZE;
                }
            }

            ARSAL_PRINT (ARSAL_PRINT_WARNING, ARVIDEO_SENDER_TAG, "New frame has size %d (=%d packets)", sendSize, nbPackets);
        }
        /* END OF NEW FRAME BLOCK */

        /* Flag all non-ack packets as "packet to send" */
        ARSAL_Mutex_Lock (&(sender->packetsToSendMutex));
        ARSAL_Mutex_Lock (&(sender->ackMutex));
        for (cnt = 0; cnt < nbPackets; cnt++)
        {
            if (0 == ARVIDEO_NetworkHeaders_AckPacketFlagIsSet (&(sender->ackPacket), cnt))
            {
                ARVIDEO_NetworkHeaders_AckPacketSetFlag (&(sender->packetsToSend), cnt);
            }
        }
        ARSAL_Mutex_Unlock (&(sender->ackMutex));

        /* Send all "packets to send" */
        for (cnt = 0; cnt < nbPackets; cnt++)
        {
            if (ARVIDEO_NetworkHeaders_AckPacketFlagIsSet (&(sender->packetsToSend), cnt))
            {
                int currFragmentSize = (cnt == nbPackets-1) ? lastFragmentSize : ARVIDEO_NETWORK_HEADERS_FRAGMENT_SIZE;
                //ARSAL_PRINT (ARSAL_PRINT_DEBUG, ARVIDEO_SENDER_TAG, "Will send subpacket %d with size %d", cnt, currFragmentSize);
                header->fragmentNumber = cnt;
                header->fragmentsPerFrame = nbPackets;
                memcpy (&sendFragment[sizeof (ARVIDEO_NetworkHeaders_DataHeader_t)], &(sender->currentFrameBuffer)[ARVIDEO_NETWORK_HEADERS_FRAGMENT_SIZE*cnt], currFragmentSize);
                ARVIDEO_Sender_NetworkCallbackParam_t *cbParams = malloc (sizeof (ARVIDEO_Sender_NetworkCallbackParam_t));
                cbParams->sender = sender;
                cbParams->fragmentIndex = cnt;
                ARNETWORK_Manager_SendData (sender->manager, sender->dataBufferID, sendFragment, currFragmentSize + sizeof (ARVIDEO_NetworkHeaders_DataHeader_t), (void *)cbParams, ARVIDEO_Sender_NetworkCallback, 1);
            }
        }
        ARSAL_Mutex_Unlock (&(sender->packetsToSendMutex));
    }
    /* END OF PROCESS LOOP */

    ARSAL_PRINT (ARSAL_PRINT_DEBUG, ARVIDEO_SENDER_TAG, "Sender thread ended");
    sender->dataThreadStarted = 0;

    return (void *)0;
}


void* ARVIDEO_Sender_RunAckThread (void *ARVIDEO_Sender_t_Param)
{
    ARVIDEO_NetworkHeaders_AckPacket_t recvPacket;
    int recvSize;
    ARVIDEO_Sender_t *sender = (ARVIDEO_Sender_t *)ARVIDEO_Sender_t_Param;

    ARSAL_PRINT (ARSAL_PRINT_DEBUG, ARVIDEO_SENDER_TAG, "Ack thread running");
    sender->ackThreadStarted = 1;

    ARVIDEO_NetworkHeaders_AckPacketReset (&recvPacket);

    while (sender->threadsShouldStop == 0)
    {
        eARNETWORK_ERROR err = ARNETWORK_Manager_ReadDataWithTimeout (sender->manager, sender->ackBufferID, (uint8_t *)&recvPacket, sizeof (recvPacket), &recvSize, 1000);
        if (ARNETWORK_OK != err)
        {
            ARSAL_PRINT (ARSAL_PRINT_ERROR, ARVIDEO_SENDER_TAG, "Error while reading ACK data: %s", ARNETWORK_Error_ToString (err));
        }
        else if (recvSize != sizeof (recvPacket))
        {
            ARSAL_PRINT (ARSAL_PRINT_ERROR, ARVIDEO_SENDER_TAG, "Read %d octets, expected %d", recvSize, sizeof (recvPacket));
        }
        else
        {
            /* Switch recvPacket endianness */
            recvPacket.numFrame = dtohl (recvPacket.numFrame);
            recvPacket.highPacketsAck = dtohll (recvPacket.highPacketsAck);
            recvPacket.lowPacketsAck = dtohll (recvPacket.lowPacketsAck);

            /* Apply recvPacket to sender->ackPacket if frame numbers are the same */
            ARSAL_Mutex_Lock (&(sender->ackMutex));
            if (sender->ackPacket.numFrame == recvPacket.numFrame)
            {
                sender->ackPacket.highPacketsAck |= recvPacket.highPacketsAck;
                sender->ackPacket.lowPacketsAck |= recvPacket.lowPacketsAck;
                ARSAL_Mutex_Lock (&(sender->callbackMutex));
                if ((sender->currentFrameCbWasCalled == 0) &&
                    (ARVIDEO_NetworkHeaders_AckPacketAllFlagsSet (&(sender->ackPacket), sender->currentFrameNbFragments) == 1))
                {
                    sender->callback (ARVIDEO_SENDER_STATUS_FRAME_SENT, sender->currentFrameBuffer, sender->currentFrameSize);
                    sender->currentFrameCbWasCalled = 1;
                }
                ARSAL_Mutex_Unlock (&(sender->callbackMutex));
            }

            ARSAL_Mutex_Unlock (&(sender->ackMutex));
        }
    }

    ARSAL_PRINT (ARSAL_PRINT_DEBUG, ARVIDEO_SENDER_TAG, "Ack thread ended");
    sender->ackThreadStarted = 0;
    return (void *)0;
}
