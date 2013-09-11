/**
 * @file ARSTREAMING_Buffers.h
 * @brief Infos of libARNetwork buffers
 * @date 03/22/2013
 * @author nicolas.brulez@parrot.com
 */

#ifndef _ARSTREAMING_BUFFERS_PRIVATE_H_
#define _ARSTREAMING_BUFFERS_PRIVATE_H_

/*
 * System Headers
 */

/*
 * Private Headers
 */
#include "ARSTREAMING_NetworkHeaders.h"

/*
 * ARSDK Headers
 */
#include <libARNetwork/ARNETWORK_IOBufferParam.h>

/*
 * Macros
 */
#define ARSTREAMING_BUFFERS_DATA_BUFFER_TYPE            (ARNETWORKAL_FRAME_TYPE_DATA_LOW_LATENCY)
#define ARSTREAMING_BUFFERS_DATA_BUFFER_SEND_EVERY_MS   (0) // Zero means "send every time we can"
#define ARSTREAMING_BUFFERS_DATA_BUFFER_NUMBER_OF_CELLS (128)
#define ARSTREAMING_BUFFERS_DATA_BUFFER_COPY_MAX_SIZE   (ARSTREAMING_NETWORK_HEADERS_FRAGMENT_SIZE + sizeof (ARSTREAMING_NetworkHeaders_DataHeader_t))
#define ARSTREAMING_BUFFERS_DATA_BUFFER_OVERWRITE       (0)

#define ARSTREAMING_BUFFERS_ACK_BUFFER_TYPE             (ARNETWORKAL_FRAME_TYPE_DATA_LOW_LATENCY)
#define ARSTREAMING_BUFFERS_ACK_BUFFER_SEND_EVERY_MS    (0) // Zero means "send every time we can"
#define ARSTREAMING_BUFFERS_ACK_BUFFER_NUMBER_OF_CELLS  (1000) // TODO: Change to 1 when mantis 115578 will be fixed
#define ARSTREAMING_BUFFERS_ACK_BUFFER_COPY_MAX_SIZE    (sizeof (ARSTREAMING_NetworkHeaders_AckPacket_t))
#define ARSTREAMING_BUFFERS_ACK_BUFFER_OVERWRITE        (1)

/*
 * Types
 */

/*
 * Functions declarations
 */
void ARSTREAMING_Buffers_InitStreamingDataBuffer (ARNETWORK_IOBufferParam_t *bufferParams, int bufferID);

void ARSTREAMING_Buffers_InitStreamingAckBuffer (ARNETWORK_IOBufferParam_t *bufferParams, int bufferID);

#endif /* _ARSTREAMING_BUFFERS_PRIVATE_H_ */
