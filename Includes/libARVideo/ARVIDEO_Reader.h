/**
 * @file ARVIDEO_Reader.h
 * @brief Video reader on network
 * @date 03/22/2013
 * @author nicolas.brulez@parrot.com
 */

#ifndef _ARVIDEO_READER_H_
#define _ARVIDEO_READER_H_

/*
 * System Headers
 */
#include <inttypes.h>

/*
 * ARSDK Headers
 */
#include <libARNetwork/ARNETWORK_Manager.h>
#include <libARVideo/ARVIDEO_Error.h>

/*
 * Macros
 */

/*
 * Types
 */

/**
 * @brief Causes for FrameComplete callback
 */
typedef enum {
    ARVIDEO_READER_CAUSE_FRAME_COMPLETE = 0, /**< Frame is complete (no error) */
    ARVIDEO_READER_CAUSE_FRAME_TOO_SMALL, /**< Frame buffer is too small for the frame on the network */
    ARVIDEO_READER_CAUSE_COPY_COMPLETE, /**< Copy of previous frame buffer is complete (called only after ARVIDEO_READER_CAUSE_FRAME_TOO_SMALL) */
    ARVIDEO_READER_CAUSE_CANCEL, /**< Reader is closing, so buffer is no longer used */
    ARVIDEO_READER_CAUSE_MAX,
} eARVIDEO_READER_CAUSE;

/**
 * @brief Callback called when a new frame is ready in a buffer
 *
 * @param[in] cause Describes why this callback was called
 * @param[in] framePointer Pointer to the frame buffer which was received
 * @param[in] frameSize Used size in framePointer buffer
 * @param[in] numberOfSkippedFrames Number of frames which were skipped between the previous call and this one. (Usually 0)
 * @param[out] newBufferCapacity Capacity of the next buffer to use
 *
 * @return address of a new buffer which will hold the next frame
 *
 * @note If cause is ARVIDEO_READER_CAUSE_FRAME_COMPLETE, framePointer contains a valid frame.
 * @note If cause is ARVIDEO_READER_CAUSE_FRAME_TOO_SMALL, datas will be copied into the new frame. Old frame buffer will still be in use until the callback is called again with ARVIDEO_READER_CAUSE_COPY_COMPLETE cause. If the new frame is still too small, the callback will be called again, until a suitable buffer is provided
 * @note If cause is ARVIDEO_READER_CAUSE_CANCEL or ARVIDEO_READER_CAUSE_COPY_COMPLETE, the return value and newBufferCapacity are unused
 *
 * @warning If the cause is ARVIDEO_READER_CAUSE_FRAME_TOO_SMALL, returning a buffer shorter than 'frameSize' will cause the library to skip the current frame
 */
typedef uint8_t* (*ARVIDEO_Reader_FrameCompleteCallback_t) (eARVIDEO_READER_CAUSE cause, uint8_t *framePointer, uint32_t frameSize, int numberOfSkippedFrames, uint32_t *newBufferCapacity);

/**
 * @brief An ARVIDEO_Reader_t instance allow reading video frames from a network
 */
typedef struct ARVIDEO_Reader_t ARVIDEO_Reader_t;

/*
 * Functions declarations
 */

/**
 * @brief Sets an ARNETWORK_IOBufferParam_t to describe a video data buffer
 * @param[in] bufferParams Pointer to the ARNETWORK_IOBufferParam_t to set
 * @param[in] bufferID ID to set in the ARNETWORK_IOBufferParam_t
 */
void ARVIDEO_Reader_InitVideoDataBuffer (ARNETWORK_IOBufferParam_t *bufferParams, int bufferID);

/**
 * @brief Sets an ARNETWORK_IOBufferParam_t to describe a video ack buffer
 * @param[in] bufferParams Pointer to the ARNETWORK_IOBufferParam_t to set
 * @param[in] bufferID ID to set in the ARNETWORK_IOBufferParam_t
 */
void ARVIDEO_Reader_InitVideoAckBuffer (ARNETWORK_IOBufferParam_t *bufferParams, int bufferID);

/**
 * @brief Creates a new ARVIDEO_Reader_t
 * @Warning This function allocates memory. An ARVIDEO_Reader_t muse be deleted by a call to ARVIDEO_Reader_Delete
 *
 * @param[in] manager Pointer to a valid and connected ARNETWORK_Manager_t, which will be used to send video frames
 * @param[in] dataBufferID ID of a VideoDataBuffer available within the manager
 * @param[in] ackBufferID ID of a VideoAckBuffer available within the manager
 * @param[in] callback The callback which will be called every time a new frame is available
 * @param[in] frameBuffer The adress of the first frameBuffer to use
 * @param[in] frameBufferSize The length of the frameBuffer (to avoid overflow)
 * @return A pointer to the new ARVIDEO_Reader_t, or NULL if an error occured
 * @see ARVIDEO_Reader_InitVideoDataBuffer()
 * @see ARVIDEO_Reader_InitVideoAckBuffer()
 * @see ARVIDEO_Reader_StopReader()
 * @see ARVIDEO_Reader_Delete()
 */
ARVIDEO_Reader_t* ARVIDEO_Reader_New (ARNETWORK_Manager_t *manager, int dataBufferID, int ackBufferID, ARVIDEO_Reader_FrameCompleteCallback_t callback, uint8_t *frameBuffer, uint32_t frameBufferSize);

/**
 * @brief Stops a running ARVIDEO_Reader_t
 * @warning Once stopped, an ARVIDEO_Reader_t can not be restarted
 *
 * @param[in] reader The ARVIDEO_Reader_t to stop
 *
 * @note Calling this function multiple times has no effect
 */
void ARVIDEO_Reader_StopReader (ARVIDEO_Reader_t *reader);

/**
 * @brief Deletes an ARVIDEO_Reader_t
 * @warning This function should NOT be called on a running ARVIDEO_Reader_t
 *
 * @param reader Pointer to the ARVIDEO_Reader_t * to delete
 *
 * @return ARVIDEO_ERROR_OK if the ARVIDEO_Reader_t was deleted
 * @return ARVIDEO_ERROR_BUSY if the ARVIDEO_Reader_t is still busy and can not be stopped now (probably because ARVIDEO_Reader_StopReader() was not called yet)
 * @return ARVIDEO_ERROR_BAD_PARAMETERS if reader does not point to a valid ARVIDEO_Reader_t
 *
 * @note The library use a double pointer, so it can set *reader to NULL after freeing it
 */
eARVIDEO_ERROR ARVIDEO_Reader_Delete (ARVIDEO_Reader_t **reader);

/**
 * @brief Runs the data loop of the ARVIDEO_Reader_t
 * @warning This function never returns until ARVIDEO_Reader_StopReader() is called. Thus, it should be called on its own thread
 * @post Stop the ARVIDEO_Reader_t by calling ARVIDEO_Reader_StopReader() before joining the thread calling this function
 * @param[in] ARVIDEO_Reader_t_Param A valid (ARVIDEO_Reader_t *) casted as a (void *)
 */
void* ARVIDEO_Reader_RunDataThread (void *ARVIDEO_Reader_t_Param);

/**
 * @brief Runs the acknowledge loop of the ARVIDEO_Reader_t
 * @warning This function never returns until ARVIDEO_Reader_StopReader() is called. Thus, it should be called on its own thread
 * @post Stop the ARVIDEO_Reader_t by calling ARVIDEO_Reader_StopReader() before joining the thread calling this function
 * @param[in] ARVIDEO_Reader_t_Param A valid (ARVIDEO_Reader_t *) casted as a (void *)
 */
void* ARVIDEO_Reader_RunAckThread (void *ARVIDEO_Reader_t_Param);

#endif /* _ARVIDEO_READER_H_ */
