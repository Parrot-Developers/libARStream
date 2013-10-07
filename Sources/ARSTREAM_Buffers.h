/**
 * @file ARSTREAM_Buffers.h
 * @brief Infos of libARNetwork buffers
 * @date 03/22/2013
 * @author nicolas.brulez@parrot.com
 */

#ifndef _ARSTREAM_BUFFERS_PRIVATE_H_
#define _ARSTREAM_BUFFERS_PRIVATE_H_

/*
 * System Headers
 */

/*
 * Private Headers
 */
#include "ARSTREAM_NetworkHeaders.h"

/*
 * ARSDK Headers
 */
#include <libARNetwork/ARNETWORK_IOBufferParam.h>

/*
 * Macros
 */
#define ARSTREAM_BUFFERS_DATA_BUFFER_TYPE            (ARNETWORKAL_FRAME_TYPE_DATA_LOW_LATENCY)
#define ARSTREAM_BUFFERS_DATA_BUFFER_SEND_EVERY_MS   (0) // Zero means "send every time we can"
#define ARSTREAM_BUFFERS_DATA_BUFFER_NUMBER_OF_CELLS (128)
#define ARSTREAM_BUFFERS_DATA_BUFFER_COPY_MAX_SIZE   (ARSTREAM_NETWORK_HEADERS_FRAGMENT_SIZE + sizeof (ARSTREAM_NetworkHeaders_DataHeader_t))
#define ARSTREAM_BUFFERS_DATA_BUFFER_OVERWRITE       (1)

#define ARSTREAM_BUFFERS_ACK_BUFFER_TYPE             (ARNETWORKAL_FRAME_TYPE_DATA_LOW_LATENCY)
#define ARSTREAM_BUFFERS_ACK_BUFFER_SEND_EVERY_MS    (0) // Zero means "send every time we can"
#define ARSTREAM_BUFFERS_ACK_BUFFER_NUMBER_OF_CELLS  (1000) // TODO: Change to 1 when mantis 115578 will be fixed
#define ARSTREAM_BUFFERS_ACK_BUFFER_COPY_MAX_SIZE    (sizeof (ARSTREAM_NetworkHeaders_AckPacket_t))
#define ARSTREAM_BUFFERS_ACK_BUFFER_OVERWRITE        (1)

/*
 * Types
 */

/*
 * Functions declarations
 */
void ARSTREAM_Buffers_InitStreamDataBuffer (ARNETWORK_IOBufferParam_t *bufferParams, int bufferID);

void ARSTREAM_Buffers_InitStreamAckBuffer (ARNETWORK_IOBufferParam_t *bufferParams, int bufferID);

#endif /* _ARSTREAM_BUFFERS_PRIVATE_H_ */
