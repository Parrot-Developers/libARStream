/**
 * @file ARSTREAMING_Sender.h
 * @brief Streaming sender over network
 * @date 03/21/2013
 * @author nicolas.brulez@parrot.com
 */

#ifndef _ARSTREAMING_SENDER_H_
#define _ARSTREAMING_SENDER_H_

/*
 * System Headers
 */
#include <inttypes.h>

/*
 * ARSDK Headers
 */
#include <libARNetwork/ARNETWORK_Manager.h>
#include <libARStreaming/ARSTREAMING_Error.h>

/*
 * Macros
 */

/*
 * Types
 */

/**
 * @brief Callback status values
 */
typedef enum {
    ARSTREAMING_SENDER_STATUS_FRAME_SENT = 0, /**< Frame was sent and acknowledged by peer */
    ARSTREAMING_SENDER_STATUS_FRAME_CANCEL, /**< Frame was not sent, and was cancelled by a new frame */
    ARSTREAMING_SENDER_STATUS_MAX,
} eARSTREAMING_SENDER_STATUS;

/**
 * @brief Callback type for sender informations
 * This callback is called when a frame pointer is no longer needed by the library.
 * This can occur when a frame is acknowledged, cancelled, or if a network error happened.
 * @param[in] status Why the call was made
 * @param[in] framePointer Pointer to the frame which was sent/cancelled
 * @param[in] frameSize Size, in bytes, of the frame
 * @param[in] custom Custom pointer passed during ARSTREAMING_Sender_New
 * @see eARSTREAMING_SENDER_STATUS
 */
typedef void (*ARSTREAMING_Sender_FrameUpdateCallback_t)(eARSTREAMING_SENDER_STATUS status, uint8_t *framePointer, uint32_t frameSize, void *custom);

/**
 * @brief An ARSTREAMING_Sender_t instance allow streaming frames over a network
 */
typedef struct ARSTREAMING_Sender_t ARSTREAMING_Sender_t;

/*
 * Functions declarations
 */

/**
 * @brief Sets an ARNETWORK_IOBufferParam_t to describe a streaming data buffer
 * @param[in] bufferParams Pointer to the ARNETWORK_IOBufferParam_t to set
 * @param[in] bufferID ID to set in the ARNETWORK_IOBufferParam_t
 */
void ARSTREAMING_Sender_InitStreamingDataBuffer (ARNETWORK_IOBufferParam_t *bufferParams, int bufferID);

/**
 * @brief Sets an ARNETWORK_IOBufferParam_t to describe a streaming ack buffer
 * @param[in] bufferParams Pointer to the ARNETWORK_IOBufferParam_t to set
 * @param[in] bufferID ID to set in the ARNETWORK_IOBufferParam_t
 */
void ARSTREAMING_Sender_InitStreamingAckBuffer (ARNETWORK_IOBufferParam_t *bufferParams, int bufferID);

/**
 * @brief Creates a new ARSTREAMING_Sender_t
 * @warning This function allocates memory. An ARSTREAMING_Sender_t muse be deleted by a call to ARSTREAMING_Sender_Delete
 *
 * @param[in] manager Pointer to a valid and connected ARNETWORK_Manager_t, which will be used to stream frames
 * @param[in] dataBufferID ID of a StreamingDataBuffer available within the manager
 * @param[in] ackBufferID ID of a StreamingAckBuffer available within the manager
 * @param[in] callback The status update callback which will be called every time the status of a send-frame is updated
 * @param[in] framesBufferSize Number of frames that the ARSTREAMING_Sender_t instance will be able to hold in queue
 * @param[in] custom Custom pointer which will be passed to callback
 * @param[out] error Optionnal pointer to an eARSTREAMING_ERROR to hold any error information
 * @return A pointer to the new ARSTREAMING_Sender_t, or NULL if an error occured
 *
 * @note framesBufferSize should be greater than the number of frames between two I-Frames
 *
 * @see ARSTREAMING_Sender_InitStreamingDataBuffer()
 * @see ARSTREAMING_Sender_InitStreamingAckBuffer()
 * @see ARSTREAMING_Sender_StopSender()
 * @see ARSTREAMING_Sender_Delete()
 */
ARSTREAMING_Sender_t* ARSTREAMING_Sender_New (ARNETWORK_Manager_t *manager, int dataBufferID, int ackBufferID, ARSTREAMING_Sender_FrameUpdateCallback_t callback, uint32_t framesBufferSize, void *custom, eARSTREAMING_ERROR *error);

/**
 * @brief Stops a running ARSTREAMING_Sender_t
 * @warning Once stopped, an ARSTREAMING_Sender_t can not be restarted
 *
 * @param[in] sender The ARSTREAMING_Sender_t to stop
 *
 * @note Calling this function multiple times has no effect
 */
void ARSTREAMING_Sender_StopSender (ARSTREAMING_Sender_t *sender);

/**
 * @brief Deletes an ARSTREAMING_Sender_t
 * @warning This function should NOT be called on a running ARSTREAMING_Sender_t
 *
 * @param sender Pointer to the ARSTREAMING_Sender_t * to delete
 *
 * @return ARSTREAMING_OK if the ARSTREAMING_Sender_t was deleted
 * @return ARSTREAMING_ERROR_BUSY if the ARSTREAMING_Sender_t is still busy and can not be stopped now (probably because ARSTREAMING_Sender_StopSender() was not called yet)
 * @return ARSTREAMING_ERROR_BAD_PARAMETERS if sender does not point to a valid ARSTREAMING_Sender_t
 *
 * @note The library use a double pointer, so it can set *sender to NULL after freeing it
 */
eARSTREAMING_ERROR ARSTREAMING_Sender_Delete (ARSTREAMING_Sender_t **sender);

/**
 * @brief Sends a new frame
 *
 * @param[in] sender The ARSTREAMING_Sender_t which will try to send the frame
 * @param[in] frameBuffer pointer to the frame in memory
 * @param[in] frameSize size of the frame in memory
 * @param[in] flushPreviousFrames Boolean-like flag (0/1). If active, tells the sender to flush the frame queue when adding this frame.
 * @param[out] nbPreviousFrames Optionnal int pointer which will store the number of frames previously in the buffer (even if the buffer is flushed)
 * @return ARSTREAMING_OK if no error happened
 * @return ARSTREAMING_ERROR_BAD_PARAMETERS if the sender or frameBuffer pointer is invalid, or if frameSize is zero
 * @return ARSTREAMING_ERROR_FRAME_TOO_LARGE if the frameSize is greater that the maximum frame size of the libARStreaming (typically 128000 bytes)
 * @return ARSTREAMING_ERROR_QUEUE_FULL if the frame can not be added to queue. This value can not happen if flushPreviousFrames is active
 */
eARSTREAMING_ERROR ARSTREAMING_Sender_SendNewFrame (ARSTREAMING_Sender_t *sender, uint8_t *frameBuffer, uint32_t frameSize, int flushPreviousFrames, int *nbPreviousFrames);

/**
 * @brief Runs the data loop of the ARSTREAMING_Sender_t
 * @warning This function never returns until ARSTREAMING_Sender_StopSender() is called. Thus, it should be called on its own thread
 * @post Stop the ARSTREAMING_Sender_t by calling ARSTREAMING_Sender_StopSender() before joining the thread calling this function
 * @param[in] ARSTREAMING_Sender_t_Param A valid (ARSTREAMING_Sender_t *) casted as a (void *)
 */
void* ARSTREAMING_Sender_RunDataThread (void *ARSTREAMING_Sender_t_Param);

/**
 * @brief Runs the acknowledge loop of the ARSTREAMING_Sender_t
 * @warning This function never returns until ARSTREAMING_Sender_StopSender() is called. Thus, it should be called on its own thread
 * @post Stop the ARSTREAMING_Sender_t by calling ARSTREAMING_Sender_StopSender() before joining the thread calling this function
 * @param[in] ARSTREAMING_Sender_t_Param A valid (ARSTREAMING_Sender_t *) casted as a (void *)
 */
void* ARSTREAMING_Sender_RunAckThread (void *ARSTREAMING_Sender_t_Param);

/**
 * @brief Gets the estimated network efficiency for the ARSTREAMING link
 * An efficiency of 1.0f means that we did not do any retries
 * @warning This function is a debug-only function and will disappear on release builds
 * @param[in] sender The ARSTREAMING_Sender_t
 */
float ARSTREAMING_Sender_GetEstimatedEfficiency (ARSTREAMING_Sender_t *sender);

#endif /* _ARSTREAMING_SENDER_H_ */
