/**
 * @file ARSTREAMING_Reader.h
 * @brief Streaming reader on network
 * @date 03/22/2013
 * @author nicolas.brulez@parrot.com
 */

#ifndef _ARSTREAMING_READER_H_
#define _ARSTREAMING_READER_H_

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
 * @brief Causes for FrameComplete callback
 */
typedef enum {
    ARSTREAMING_READER_CAUSE_FRAME_COMPLETE = 0, /**< Frame is complete (no error) */
    ARSTREAMING_READER_CAUSE_FRAME_TOO_SMALL, /**< Frame buffer is too small for the frame on the network */
    ARSTREAMING_READER_CAUSE_COPY_COMPLETE, /**< Copy of previous frame buffer is complete (called only after ARSTREAMING_READER_CAUSE_FRAME_TOO_SMALL) */
    ARSTREAMING_READER_CAUSE_CANCEL, /**< Reader is closing, so buffer is no longer used */
    ARSTREAMING_READER_CAUSE_MAX,
} eARSTREAMING_READER_CAUSE;

/**
 * @brief Callback called when a new frame is ready in a buffer
 *
 * @param[in] cause Describes why this callback was called
 * @param[in] framePointer Pointer to the frame buffer which was received
 * @param[in] frameSize Used size in framePointer buffer
 * @param[in] isFlushFrame Boolean-like (0-1) flag telling if the complete frame was a flush frame (typically an I-Frame) for the sender
 * @param[in] numberOfSkippedFrames Number of frames which were skipped between the previous call and this one. (Usually 0)
 * @param[out] newBufferCapacity Capacity of the next buffer to use
 * @param[in] custom Custom pointer passed during ARSTREAMING_Reader_New
 *
 * @return address of a new buffer which will hold the next frame
 *
 * @note If cause is ARSTREAMING_READER_CAUSE_FRAME_COMPLETE, framePointer contains a valid frame.
 * @note If cause is ARSTREAMING_READER_CAUSE_FRAME_TOO_SMALL, datas will be copied into the new frame. Old frame buffer will still be in use until the callback is called again with ARSTREAMING_READER_CAUSE_COPY_COMPLETE cause. If the new frame is still too small, the callback will be called again, until a suitable buffer is provided
 * @note If cause is ARSTREAMING_READER_CAUSE_CANCEL or ARSTREAMING_READER_CAUSE_COPY_COMPLETE, the return value and newBufferCapacity are unused
 *
 * @warning If the cause is ARSTREAMING_READER_CAUSE_FRAME_TOO_SMALL, returning a buffer shorter than 'frameSize' will cause the library to skip the current frame
 * @warning In any case, returning a NULL buffer is not supported.
 */
typedef uint8_t* (*ARSTREAMING_Reader_FrameCompleteCallback_t) (eARSTREAMING_READER_CAUSE cause, uint8_t *framePointer, uint32_t frameSize, int isFlushFrame, int numberOfSkippedFrames, uint32_t *newBufferCapacity, void *custom);

/**
 * @brief An ARSTREAMING_Reader_t instance allow reading stream frames from a network
 */
typedef struct ARSTREAMING_Reader_t ARSTREAMING_Reader_t;

/*
 * Functions declarations
 */

/**
 * @brief Sets an ARNETWORK_IOBufferParam_t to describe a streaming data buffer
 * @param[in] bufferParams Pointer to the ARNETWORK_IOBufferParam_t to set
 * @param[in] bufferID ID to set in the ARNETWORK_IOBufferParam_t
 */
void ARSTREAMING_Reader_InitStreamingDataBuffer (ARNETWORK_IOBufferParam_t *bufferParams, int bufferID);

/**
 * @brief Sets an ARNETWORK_IOBufferParam_t to describe a streaming ack buffer
 * @param[in] bufferParams Pointer to the ARNETWORK_IOBufferParam_t to set
 * @param[in] bufferID ID to set in the ARNETWORK_IOBufferParam_t
 */
void ARSTREAMING_Reader_InitStreamingAckBuffer (ARNETWORK_IOBufferParam_t *bufferParams, int bufferID);

/**
 * @brief Creates a new ARSTREAMING_Reader_t
 * @warning This function allocates memory. An ARSTREAMING_Reader_t muse be deleted by a call to ARSTREAMING_Reader_Delete
 *
 * @param[in] manager Pointer to a valid and connected ARNETWORK_Manager_t, which will be used to stream frames
 * @param[in] dataBufferID ID of a StreamingDataBuffer available within the manager
 * @param[in] ackBufferID ID of a StreamingAckBuffer available within the manager
 * @param[in] callback The callback which will be called every time a new frame is available
 * @param[in] frameBuffer The adress of the first frameBuffer to use
 * @param[in] frameBufferSize The length of the frameBuffer (to avoid overflow)
 * @param[in] custom Custom pointer which will be passed to callback
 * @param[out] error Optionnal pointer to an eARSTREAMING_ERROR to hold any error information
 * @return A pointer to the new ARSTREAMING_Reader_t, or NULL if an error occured
 * @see ARSTREAMING_Reader_InitStreamingDataBuffer()
 * @see ARSTREAMING_Reader_InitStreamingAckBuffer()
 * @see ARSTREAMING_Reader_StopReader()
 * @see ARSTREAMING_Reader_Delete()
 */
ARSTREAMING_Reader_t* ARSTREAMING_Reader_New (ARNETWORK_Manager_t *manager, int dataBufferID, int ackBufferID, ARSTREAMING_Reader_FrameCompleteCallback_t callback, uint8_t *frameBuffer, uint32_t frameBufferSize, void *custom, eARSTREAMING_ERROR *error);

/**
 * @brief Stops a running ARSTREAMING_Reader_t
 * @warning Once stopped, an ARSTREAMING_Reader_t can not be restarted
 *
 * @param[in] reader The ARSTREAMING_Reader_t to stop
 *
 * @note Calling this function multiple times has no effect
 */
void ARSTREAMING_Reader_StopReader (ARSTREAMING_Reader_t *reader);

/**
 * @brief Deletes an ARSTREAMING_Reader_t
 * @warning This function should NOT be called on a running ARSTREAMING_Reader_t
 *
 * @param reader Pointer to the ARSTREAMING_Reader_t * to delete
 *
 * @return ARSTREAMING_OK if the ARSTREAMING_Reader_t was deleted
 * @return ARSTREAMING_ERROR_BUSY if the ARSTREAMING_Reader_t is still busy and can not be stopped now (probably because ARSTREAMING_Reader_StopReader() was not called yet)
 * @return ARSTREAMING_ERROR_BAD_PARAMETERS if reader does not point to a valid ARSTREAMING_Reader_t
 *
 * @note The library use a double pointer, so it can set *reader to NULL after freeing it
 */
eARSTREAMING_ERROR ARSTREAMING_Reader_Delete (ARSTREAMING_Reader_t **reader);

/**
 * @brief Runs the data loop of the ARSTREAMING_Reader_t
 * @warning This function never returns until ARSTREAMING_Reader_StopReader() is called. Thus, it should be called on its own thread
 * @post Stop the ARSTREAMING_Reader_t by calling ARSTREAMING_Reader_StopReader() before joining the thread calling this function
 * @param[in] ARSTREAMING_Reader_t_Param A valid (ARSTREAMING_Reader_t *) casted as a (void *)
 */
void* ARSTREAMING_Reader_RunDataThread (void *ARSTREAMING_Reader_t_Param);

/**
 * @brief Runs the acknowledge loop of the ARSTREAMING_Reader_t
 * @warning This function never returns until ARSTREAMING_Reader_StopReader() is called. Thus, it should be called on its own thread
 * @post Stop the ARSTREAMING_Reader_t by calling ARSTREAMING_Reader_StopReader() before joining the thread calling this function
 * @param[in] ARSTREAMING_Reader_t_Param A valid (ARSTREAMING_Reader_t *) casted as a (void *)
 */
void* ARSTREAMING_Reader_RunAckThread (void *ARSTREAMING_Reader_t_Param);

/**
 * @brief Gets the estimated network efficiency for the ARSTREAMING link
 * An efficiency of 1.0f means that we did not receive any useless packet.
 * Efficiency is computed on all frames for which the Reader got at least a packet, even if the frame was not complete.
 * @warning This function is a debug-only function and will disappear on release builds
 * @param[in] reader The ARSTREAMING_Reader_t
 */
float ARSTREAMING_Reader_GetEstimatedEfficiency (ARSTREAMING_Reader_t *reader);

#endif /* _ARSTREAMING_READER_H_ */
