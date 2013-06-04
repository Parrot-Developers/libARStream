/**
 * @file ARVIDEO_Sender.h
 * @brief Video sender over network
 * @date 03/21/2013
 * @author nicolas.brulez@parrot.com
 */

#ifndef _ARVIDEO_SENDER_H_
#define _ARVIDEO_SENDER_H_

/*
 * System Headers
 */
#include <inttypes.h>

/*
 * ARSDK Headers
 */
#include <libARNetwork/ARNETWORK_Manager.h>

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
    ARVIDEO_SENDER_STATUS_FRAME_SENT = 0, /**< Frame was sent and acknowledged by peer */
    ARVIDEO_SENDER_STATUS_FRAME_CANCEL, /**< Frame was not sent, and was cancelled by a new frame */
    ARVIDEO_SENDER_STATUS_MAX,
} eARVIDEO_SENDER_STATUS;

/**
 * @brief Callback type for sender informations
 * This callback is called when a frame pointer is no longer needed by the library.
 * This can occur when a frame is acknowledged, cancelled, or if a network error happened.
 * @param[in] status Why the call was made
 * @param[in] framePointer Pointer to the frame which was sent/cancelled
 * @param[in] frameSize Size, in bytes, of the frame
 * @see eARVIDEO_SENDER_STATUS
 */
typedef void (*ARVIDEO_Sender_FrameUpdateCallback_t)(eARVIDEO_SENDER_STATUS status, uint8_t *framePointer, uint32_t frameSize);

/**
 * @brief An ARVIDEO_Sender_t instance allow sending video frames over a network
 */
typedef struct ARVIDEO_Sender_t ARVIDEO_Sender_t;

/*
 * Functions declarations
 */

/**
 * @brief Sets an ARNETWORK_IOBufferParam_t to describe a video data buffer
 * @param[in] bufferParams Pointer to the ARNETWORK_IOBufferParam_t to set
 * @param[in] bufferID ID to set in the ARNETWORK_IOBufferParam_t
 */
void ARVIDEO_Sender_InitVideoDataBuffer (ARNETWORK_IOBufferParam_t *bufferParams, int bufferID);

/**
 * @brief Sets an ARNETWORK_IOBufferParam_t to describe a video ack buffer
 * @param[in] bufferParams Pointer to the ARNETWORK_IOBufferParam_t to set
 * @param[in] bufferID ID to set in the ARNETWORK_IOBufferParam_t
 */
void ARVIDEO_Sender_InitVideoAckBuffer (ARNETWORK_IOBufferParam_t *bufferParams, int bufferID);

/**
 * @brief Creates a new ARVIDEO_Sender_t
 * @Warning This function allocates memory. An ARVIDEO_Sender_t muse be deleted by a call to ARVIDEO_Sender_Delete
 *
 * @param[in] manager Pointer to a valid and connected ARNETWORK_Manager_t, which will be used to send video frames
 * @param[in] dataBufferID ID of a VideoDataBuffer available within the manager
 * @param[in] ackBufferID ID of a VideoAckBuffer available within the manager
 * @param[in] callback The status update callback which will be called every time the status of a send-frame is updated
 * @param[in] framesBufferSize Number of frames that the ARVIDEO_Sender_t instance will be able to hold in queue
 * @return A pointer to the new ARVIDEO_Sender_t, or NULL if an error occured
 *
 * @note framesBufferSize should be greater than the number of frames between two I-Frames
 *
 * @see ARVIDEO_Sender_InitVideoDataBuffer()
 * @see ARVIDEO_Sender_InitVideoAckBuffer()
 * @see ARVIDEO_Sender_StopSender()
 * @see ARVIDEO_Sender_Delete()
 */
ARVIDEO_Sender_t* ARVIDEO_Sender_New (ARNETWORK_Manager_t *manager, int dataBufferID, int ackBufferID, ARVIDEO_Sender_FrameUpdateCallback_t callback, uint32_t framesBufferSize);

/**
 * @brief Stops a running ARVIDEO_Sender_t
 * @warning Once stopped, an ARVIDEO_Sender_t can not be restarted
 *
 * @param[in] sender The ARVIDEO_Sender_t to stop
 *
 * @note Calling this function multiple times has no effect
 */
void ARVIDEO_Sender_StopSender (ARVIDEO_Sender_t *sender);

/**
 * @brief Deletes an ARVIDEO_Sender_t
 * @warning This function should NOT be called on a running ARVIDEO_Sender_t
 *
 * @param sender Pointer to the ARVIDEO_Sender_t * to delete
 *
 * @return 1 if the ARVIDEO_Sender_t was deleted
 * @return 0 if the ARVIDEO_Sender_t is still busy and can not be stopped now (probably because ARVIDEO_Sender_StopSender() was not called yet)
 * @return -1 if sender does not point to a valid ARVIDEO_Sender_t
 *
 * @note The library use a double pointer, so it can set *sender to NULL after freeing it
 */
int ARVIDEO_Sender_Delete (ARVIDEO_Sender_t **sender);

/**
 * @brief Sends a new frame
 *
 * @param[in] sender The ARVIDEO_Sender_t which will try to send the frame
 * @param[in] frameBuffer pointer to the frame in memory
 * @param[in] frameSize size of the frame in memory
 * @param[in] flushPreviousFrames Boolean-like flag (0/1). If active, tells the sender to flush the frame queue when adding this frame.
 * @return The number of frames already waiting in queue (even if flushed)
 * @return -1 on error
 */
int ARVIDEO_Sender_SendNewFrame (ARVIDEO_Sender_t *sender, uint8_t *frameBuffer, uint32_t frameSize, int flushPreviousFrames);

/**
 * @brief Runs the data loop of the ARVIDEO_Sender_t
 * @warning This function never returns until ARVIDEO_Sender_StopSender() is called. Thus, it should be called on its own thread
 * @post Stop the ARVIDEO_Sender_t by calling ARVIDEO_Sender_StopSender() before joining the thread calling this function
 * @param[in] ARVIDEO_Sender_t_Param A valid (ARVIDEO_Sender_t *) casted as a (void *)
 */
void* ARVIDEO_Sender_RunDataThread (void *ARVIDEO_Sender_t_Param);

/**
 * @brief Runs the acknowledge loop of the ARVIDEO_Sender_t
 * @warning This function never returns until ARVIDEO_Sender_StopSender() is called. Thus, it should be called on its own thread
 * @post Stop the ARVIDEO_Sender_t by calling ARVIDEO_Sender_StopSender() before joining the thread calling this function
 * @param[in] ARVIDEO_Sender_t_Param A valid (ARVIDEO_Sender_t *) casted as a (void *)
 */
void* ARVIDEO_Sender_RunAckThread (void *ARVIDEO_Sender_t_Param);

#endif /* _ARVIDEO_SENDER_H_ */
