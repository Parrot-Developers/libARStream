/**
 * @file ARVIDEO_Buffers.h
 * @brief Infos of libARNetwork buffers
 * @date 03/22/2013
 * @author nicolas.brulez@parrot.com
 */

#ifndef _ARVIDEO_BUFFERS_PRIVATE_H_
#define _ARVIDEO_BUFFERS_PRIVATE_H_

/*
 * System Headers
 */

/*
 * Private Headers
 */
#include "ARVIDEO_NetworkHeaders.h"

/*
 * ARSDK Headers
 */
#include <libARNetwork/ARNETWORK_IOBufferParam.h>

/*
 * Macros
 */
#define ARVIDEO_BUFFERS_DATA_BUFFER_TYPE            (ARNETWORK_FRAME_TYPE_DATA_LOW_LATENCY)
#define ARVIDEO_BUFFERS_DATA_BUFFER_SEND_EVERY_MS   (1)
#define ARVIDEO_BUFFERS_DATA_BUFFER_NUMBER_OF_CELLS (128)
#define ARVIDEO_BUFFERS_DATA_BUFFER_COPY_MAX_SIZE   (ARVIDEO_NETWORK_HEADERS_FRAGMENT_SIZE + sizeof (ARVIDEO_NetworkHeaders_DataHeader_t))
#define ARVIDEO_BUFFERS_DATA_BUFFER_OVERWRITE       (0)

#define ARVIDEO_BUFFERS_ACK_BUFFER_TYPE             (ARNETWORK_FRAME_TYPE_DATA_LOW_LATENCY)
#define ARVIDEO_BUFFERS_ACK_BUFFER_SEND_EVERY_MS    (1)
#define ARVIDEO_BUFFERS_ACK_BUFFER_NUMBER_OF_CELLS  (20)
#define ARVIDEO_BUFFERS_ACK_BUFFER_COPY_MAX_SIZE    (sizeof (ARVIDEO_NetworkHeaders_AckPacket_t))
#define ARVIDEO_BUFFERS_ACK_BUFFER_OVERWRITE        (1)

/*
 * Types
 */

/*
 * Functions declarations
 */
void ARVIDEO_Buffers_InitVideoDataBuffer (ARNETWORK_IOBufferParam_t *bufferParams, int bufferID);

void ARVIDEO_Buffers_InitVideoAckBuffer (ARNETWORK_IOBufferParam_t *bufferParams, int bufferID);

#endif /* _ARVIDEO_BUFFERS_PRIVATE_H_ */
