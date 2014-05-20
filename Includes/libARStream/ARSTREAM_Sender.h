/**
 * @file ARSTREAM_Sender.h
 * @brief Stream sender over network
 * @date 03/21/2013
 * @author nicolas.brulez@parrot.com
 */

#ifndef _ARSTREAM_SENDER_H_
#define _ARSTREAM_SENDER_H_

/*
 * System Headers
 */
#include <inttypes.h>

/*
 * ARSDK Headers
 */
#include <libARNetwork/ARNETWORK_Manager.h>
#include <libARStream/ARSTREAM_Error.h>

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
    ARSTREAM_SENDER_STATUS_FRAME_SENT = 0, /**< Frame was sent and acknowledged by peer */
    ARSTREAM_SENDER_STATUS_FRAME_CANCEL, /**< Frame was not sent, and was cancelled by a new frame */
    ARSTREAM_SENDER_STATUS_MAX,
} eARSTREAM_SENDER_STATUS;

/**
 * @brief Callback type for sender informations
 * This callback is called when a frame pointer is no longer needed by the library.
 * This can occur when a frame is acknowledged, cancelled, or if a network error happened.
 * @param[in] status Why the call was made
 * @param[in] framePointer Pointer to the frame which was sent/cancelled
 * @param[in] frameSize Size, in bytes, of the frame
 * @param[in] custom Custom pointer passed during ARSTREAM_Sender_New
 * @see eARSTREAM_SENDER_STATUS
 */
typedef void (*ARSTREAM_Sender_FrameUpdateCallback_t)(eARSTREAM_SENDER_STATUS status, uint8_t *framePointer, uint32_t frameSize, void *custom);

/**
 * @brief An ARSTREAM_Sender_t instance allow streaming frames over a network
 */
typedef struct ARSTREAM_Sender_t ARSTREAM_Sender_t;

/*
 * Functions declarations
 */

/**
 * @brief Sets an ARNETWORK_IOBufferParam_t to describe a stream data buffer
 * @param[in] bufferParams Pointer to the ARNETWORK_IOBufferParam_t to set
 * @param[in] bufferID ID to set in the ARNETWORK_IOBufferParam_t
 * @param[in] maxFragmentSize Maximum allowed size for a video data fragment. Video frames larger that will be fragmented.
 * @param[in] maxNumberOfFragment number maximum of fragment of one frame.
 */
void ARSTREAM_Sender_InitStreamDataBuffer (ARNETWORK_IOBufferParam_t *bufferParams, int bufferID, int maxFragmentSize, uint32_t maxFragmentPerFrame);

/**
 * @brief Sets an ARNETWORK_IOBufferParam_t to describe a stream ack buffer
 * @param[in] bufferParams Pointer to the ARNETWORK_IOBufferParam_t to set
 * @param[in] bufferID ID to set in the ARNETWORK_IOBufferParam_t
 */
void ARSTREAM_Sender_InitStreamAckBuffer (ARNETWORK_IOBufferParam_t *bufferParams, int bufferID);

/**
 * @brief Creates a new ARSTREAM_Sender_t
 * @warning This function allocates memory. An ARSTREAM_Sender_t muse be deleted by a call to ARSTREAM_Sender_Delete
 *
 * @param[in] manager Pointer to a valid and connected ARNETWORK_Manager_t, which will be used to stream frames
 * @param[in] dataBufferID ID of a StreamDataBuffer available within the manager
 * @param[in] ackBufferID ID of a StreamAckBuffer available within the manager
 * @param[in] callback The status update callback which will be called every time the status of a send-frame is updated
 * @param[in] framesBufferSize Number of frames that the ARSTREAM_Sender_t instance will be able to hold in queue
 * @param[in] maxFragmentSize Maximum allowed size for a video data fragment. Video frames larger that will be fragmented.
 * @param[in] maxNumberOfFragment number maximum of fragment of one frame.
 * @param[in] custom Custom pointer which will be passed to callback
 * @param[out] error Optionnal pointer to an eARSTREAM_ERROR to hold any error information
 * @return A pointer to the new ARSTREAM_Sender_t, or NULL if an error occured
 *
 * @note framesBufferSize should be greater than the number of frames between two I-Frames
 *
 * @see ARSTREAM_Sender_InitStreamDataBuffer()
 * @see ARSTREAM_Sender_InitStreamAckBuffer()
 * @see ARSTREAM_Sender_StopSender()
 * @see ARSTREAM_Sender_Delete()
 */
ARSTREAM_Sender_t* ARSTREAM_Sender_New (ARNETWORK_Manager_t *manager, int dataBufferID, int ackBufferID, ARSTREAM_Sender_FrameUpdateCallback_t callback, uint32_t framesBufferSize, uint32_t maxFragmentSize, uint32_t maxNumberOfFragment,  void *custom, eARSTREAM_ERROR *error);

/**
 * @brief Stops a running ARSTREAM_Sender_t
 * @warning Once stopped, an ARSTREAM_Sender_t can not be restarted
 *
 * @param[in] sender The ARSTREAM_Sender_t to stop
 *
 * @note Calling this function multiple times has no effect
 */
void ARSTREAM_Sender_StopSender (ARSTREAM_Sender_t *sender);

/**
 * @brief Deletes an ARSTREAM_Sender_t
 * @warning This function should NOT be called on a running ARSTREAM_Sender_t
 *
 * @param sender Pointer to the ARSTREAM_Sender_t * to delete
 *
 * @return ARSTREAM_OK if the ARSTREAM_Sender_t was deleted
 * @return ARSTREAM_ERROR_BUSY if the ARSTREAM_Sender_t is still busy and can not be stopped now (probably because ARSTREAM_Sender_StopSender() was not called yet)
 * @return ARSTREAM_ERROR_BAD_PARAMETERS if sender does not point to a valid ARSTREAM_Sender_t
 *
 * @note The library use a double pointer, so it can set *sender to NULL after freeing it
 */
eARSTREAM_ERROR ARSTREAM_Sender_Delete (ARSTREAM_Sender_t **sender);

/**
 * @brief Sends a new frame
 *
 * @param[in] sender The ARSTREAM_Sender_t which will try to send the frame
 * @param[in] frameBuffer pointer to the frame in memory
 * @param[in] frameSize size of the frame in memory
 * @param[in] flushPreviousFrames Boolean-like flag (0/1). If active, tells the sender to flush the frame queue when adding this frame.
 * @param[out] nbPreviousFrames Optionnal int pointer which will store the number of frames previously in the buffer (even if the buffer is flushed)
 * @return ARSTREAM_OK if no error happened
 * @return ARSTREAM_ERROR_BAD_PARAMETERS if the sender or frameBuffer pointer is invalid, or if frameSize is zero
 * @return ARSTREAM_ERROR_FRAME_TOO_LARGE if the frameSize is greater that the maximum frame size of the libARStream (typically 128000 bytes)
 * @return ARSTREAM_ERROR_QUEUE_FULL if the frame can not be added to queue. This value can not happen if flushPreviousFrames is active
 */
eARSTREAM_ERROR ARSTREAM_Sender_SendNewFrame (ARSTREAM_Sender_t *sender, uint8_t *frameBuffer, uint32_t frameSize, int flushPreviousFrames, int *nbPreviousFrames);

/**
 * @brief Flushes all currently queued frames
 *
 * @param[in] sender The ARSTREAM_Sender_t to be flushed.
 * @return ARSTREAM_OK if no error occured.
 * @return ARSTREAM_ERROR_BAD_PARAMETERS if the sender is invalid.
 */
eARSTREAM_ERROR ARSTREAM_Sender_FlushFramesQueue (ARSTREAM_Sender_t *sender);

/**
 * @brief Runs the data loop of the ARSTREAM_Sender_t
 * @warning This function never returns until ARSTREAM_Sender_StopSender() is called. Thus, it should be called on its own thread
 * @post Stop the ARSTREAM_Sender_t by calling ARSTREAM_Sender_StopSender() before joining the thread calling this function
 * @param[in] ARSTREAM_Sender_t_Param A valid (ARSTREAM_Sender_t *) casted as a (void *)
 */
void* ARSTREAM_Sender_RunDataThread (void *ARSTREAM_Sender_t_Param);

/**
 * @brief Runs the acknowledge loop of the ARSTREAM_Sender_t
 * @warning This function never returns until ARSTREAM_Sender_StopSender() is called. Thus, it should be called on its own thread
 * @post Stop the ARSTREAM_Sender_t by calling ARSTREAM_Sender_StopSender() before joining the thread calling this function
 * @param[in] ARSTREAM_Sender_t_Param A valid (ARSTREAM_Sender_t *) casted as a (void *)
 */
void* ARSTREAM_Sender_RunAckThread (void *ARSTREAM_Sender_t_Param);

/**
 * @brief Gets the estimated network efficiency for the ARSTREAM link
 * An efficiency of 1.0f means that we did not do any retries
 * @warning This function is a debug-only function and will disappear on release builds
 * @param[in] sender The ARSTREAM_Sender_t
 */
float ARSTREAM_Sender_GetEstimatedEfficiency (ARSTREAM_Sender_t *sender);

/**
 * @brief Gets the custom pointer associated with the sender
 * @param[in] sender The ARSTREAM_Sender_t
 * @return The custom pointer associated with this sender, or NULL if sender does not point to a valid sender
 */
void* ARSTREAM_Sender_GetCustom (ARSTREAM_Sender_t *sender);

#endif /* _ARSTREAM_SENDER_H_ */
