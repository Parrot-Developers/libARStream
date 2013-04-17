/**
 * @file ARVIDEO_Buffers.c
 * @brief Infos of libARNetwork buffers
 * @date 03/22/2013
 * @author nicolas.brulez@parrot.com
 */

#include <config.h>

/*
 * System Headers
 */
#include <stdlib.h>

/*
 * Private Headers
 */
#include "ARVIDEO_Buffers.h"

/*
 * ARSDK Headers
 */

/*
 * Macros
 */

/*
 * Types
 */

/*
 * Internal functions declarations
 */

/*
 * Internal functions implementation
 */

/*
 * Implementation
 */
void ARVIDEO_Buffers_InitVideoDataBuffer (ARNETWORK_IOBufferParam_t *bufferParams, int bufferID)
{
    if (bufferParams != NULL)
    {
        ARNETWORK_IOBufferParam_DefaultInit (bufferParams);
        bufferParams->ID = bufferID;
        bufferParams->dataType = ARVIDEO_BUFFERS_DATA_BUFFER_TYPE;
        bufferParams->sendingWaitTimeMs = ARVIDEO_BUFFERS_DATA_BUFFER_SEND_EVERY_MS;
        bufferParams->numberOfCell = ARVIDEO_BUFFERS_DATA_BUFFER_NUMBER_OF_CELLS;
        bufferParams->dataCopyMaxSize = ARVIDEO_BUFFERS_DATA_BUFFER_COPY_MAX_SIZE;
        bufferParams->isOverwriting = ARVIDEO_BUFFERS_DATA_BUFFER_OVERWRITE;
    }
}

void ARVIDEO_Buffers_InitVideoAckBuffer (ARNETWORK_IOBufferParam_t *bufferParams, int bufferID)
{
    if (bufferParams != NULL)
    {
        ARNETWORK_IOBufferParam_DefaultInit (bufferParams);
        bufferParams->ID = bufferID;
        bufferParams->dataType = ARVIDEO_BUFFERS_ACK_BUFFER_TYPE;
        bufferParams->sendingWaitTimeMs = ARVIDEO_BUFFERS_ACK_BUFFER_SEND_EVERY_MS;
        bufferParams->numberOfCell = ARVIDEO_BUFFERS_ACK_BUFFER_NUMBER_OF_CELLS;
        bufferParams->dataCopyMaxSize = ARVIDEO_BUFFERS_ACK_BUFFER_COPY_MAX_SIZE;
        bufferParams->isOverwriting = ARVIDEO_BUFFERS_ACK_BUFFER_OVERWRITE;
    }
}
