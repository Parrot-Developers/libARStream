/**
 * @file ARSTREAMING_Buffers.c
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
#include "ARSTREAMING_Buffers.h"

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
void ARSTREAMING_Buffers_InitStreamingDataBuffer (ARNETWORK_IOBufferParam_t *bufferParams, int bufferID)
{
    if (bufferParams != NULL)
    {
        ARNETWORK_IOBufferParam_DefaultInit (bufferParams);
        bufferParams->ID = bufferID;
        bufferParams->dataType = ARSTREAMING_BUFFERS_DATA_BUFFER_TYPE;
        bufferParams->sendingWaitTimeMs = ARSTREAMING_BUFFERS_DATA_BUFFER_SEND_EVERY_MS;
        bufferParams->numberOfCell = ARSTREAMING_BUFFERS_DATA_BUFFER_NUMBER_OF_CELLS;
        bufferParams->dataCopyMaxSize = ARSTREAMING_BUFFERS_DATA_BUFFER_COPY_MAX_SIZE;
        bufferParams->isOverwriting = ARSTREAMING_BUFFERS_DATA_BUFFER_OVERWRITE;
    }
}

void ARSTREAMING_Buffers_InitStreamingAckBuffer (ARNETWORK_IOBufferParam_t *bufferParams, int bufferID)
{
    if (bufferParams != NULL)
    {
        ARNETWORK_IOBufferParam_DefaultInit (bufferParams);
        bufferParams->ID = bufferID;
        bufferParams->dataType = ARSTREAMING_BUFFERS_ACK_BUFFER_TYPE;
        bufferParams->sendingWaitTimeMs = ARSTREAMING_BUFFERS_ACK_BUFFER_SEND_EVERY_MS;
        bufferParams->numberOfCell = ARSTREAMING_BUFFERS_ACK_BUFFER_NUMBER_OF_CELLS;
        bufferParams->dataCopyMaxSize = ARSTREAMING_BUFFERS_ACK_BUFFER_COPY_MAX_SIZE;
        bufferParams->isOverwriting = ARSTREAMING_BUFFERS_ACK_BUFFER_OVERWRITE;
    }
}
