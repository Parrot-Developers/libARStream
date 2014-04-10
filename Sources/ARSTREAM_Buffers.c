/**
 * @file ARSTREAM_Buffers.c
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
#include "ARSTREAM_Buffers.h"

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
void ARSTREAM_Buffers_InitStreamDataBuffer (ARNETWORK_IOBufferParam_t *bufferParams, int bufferID, int maxFragmentSize, uint32_t maxFragmentPerFrame)
{
    if (bufferParams != NULL)
    {
        ARNETWORK_IOBufferParam_DefaultInit (bufferParams);
        bufferParams->ID = bufferID;
        bufferParams->dataType = ARSTREAM_BUFFERS_DATA_BUFFER_TYPE;
        bufferParams->sendingWaitTimeMs = ARSTREAM_BUFFERS_DATA_BUFFER_SEND_EVERY_MS;
        bufferParams->numberOfCell = maxFragmentPerFrame;
        bufferParams->dataCopyMaxSize = maxFragmentSize + sizeof (ARSTREAM_NetworkHeaders_DataHeader_t);
        bufferParams->isOverwriting = ARSTREAM_BUFFERS_DATA_BUFFER_OVERWRITE;
    }
}

void ARSTREAM_Buffers_InitStreamAckBuffer (ARNETWORK_IOBufferParam_t *bufferParams, int bufferID)
{
    if (bufferParams != NULL)
    {
        ARNETWORK_IOBufferParam_DefaultInit (bufferParams);
        bufferParams->ID = bufferID;
        bufferParams->dataType = ARSTREAM_BUFFERS_ACK_BUFFER_TYPE;
        bufferParams->sendingWaitTimeMs = ARSTREAM_BUFFERS_ACK_BUFFER_SEND_EVERY_MS;
        bufferParams->numberOfCell = ARSTREAM_BUFFERS_ACK_BUFFER_NUMBER_OF_CELLS;
        bufferParams->dataCopyMaxSize = ARSTREAM_BUFFERS_ACK_BUFFER_COPY_MAX_SIZE;
        bufferParams->isOverwriting = ARSTREAM_BUFFERS_ACK_BUFFER_OVERWRITE;
    }
}
