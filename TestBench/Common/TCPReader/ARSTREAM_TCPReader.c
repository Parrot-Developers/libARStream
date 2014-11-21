/*
    Copyright (C) 2014 Parrot SA

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in
      the documentation and/or other materials provided with the 
      distribution.
    * Neither the name of Parrot nor the names
      of its contributors may be used to endorse or promote products
      derived from this software without specific prior written
      permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
    "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
    LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
    FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
    COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
    BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
    OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED 
    AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
    OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
    OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
    SUCH DAMAGE.
*/
/**
 * @file ARSTREAM_Reader_TestBench.c
 * @brief Implementation file for the Drone2-like stream test
 * @date 07/10/2013
 * @author nicolas.brulez@parrot.com
 */

/*
 * System Headers
 */

#include <errno.h>
#include <stdlib.h>

/*
 * ARSDK Headers
 */

#include <libARSAL/ARSAL_Print.h>
#include <libARSAL/ARSAL_Socket.h>

/*
 * Macros
 */

#define DATA_PORT (32154)

#define FRAME_MAX_SIZE (40000)

#define __TAG__ "ARSTREAM_TCPReader_TB"

#define NB_FRAMES_FOR_AVERAGE (15)

#define HEADER_MAGIC (0x37333331)

#define READSIZE (1000)

#define __IP "127.0.0.1"

/*
 * Types
 */

typedef struct {
    int socket;
    uint8_t *buffer;
    uint32_t index;
} ARSTREAM_TCPReader_t;

typedef struct {
    uint32_t header;
    uint32_t size;
    uint32_t num;
} ARSTREAM_TCPReader_FrameHeader_t;

/*
 * Globals
 */

static int lastDt [NB_FRAMES_FOR_AVERAGE] = {0};
static int currentIndexInDt = 0;

static int stillRunning = 1;

/*
 * Internal function declaration
 */

/**
 * Create and connect a new TCPReader
 */
ARSTREAM_TCPReader_t *ARSTREAM_TCPReader_Create (int port, const char *ip);

/**
 * Send a frame through an ARSTREAM_TCPReader_t
 */
int ARSTREAM_TCPReader_ReadFrame (ARSTREAM_TCPReader_t *reader, uint8_t *frame, uint32_t capacity);

/**
 * Delete an ARSTREAM_TCPReader_t
 */
void ARSTREAM_TCPReader_Delete (ARSTREAM_TCPReader_t **reader);

/*
 * Internal functions implementation
 */

ARSTREAM_TCPReader_t *ARSTREAM_TCPReader_Create (int port, const char *ip)
{
    ARSTREAM_TCPReader_t *reader = malloc (sizeof (ARSTREAM_TCPReader_t));
    if (reader == NULL)
    {
        return NULL;
    }

    reader->index = 0;

    reader->socket = ARSAL_Socket_Create (AF_INET, SOCK_STREAM, 0);

    if (reader->socket == -1)
    {
        ARSAL_PRINT (ARSAL_PRINT_ERROR, __TAG__, "Socket error : %s", strerror (errno));
        free (reader);
        return NULL;
    }

    struct sockaddr_in sin = {0};
    sin.sin_addr.s_addr = inet_addr (ip);
    sin.sin_port = htons (port);
    sin.sin_family = AF_INET;

    if (ARSAL_Socket_Connect (reader->socket, (struct sockaddr *)&sin, sizeof (struct sockaddr)) == -1)
    {
        ARSAL_PRINT (ARSAL_PRINT_ERROR, __TAG__, "Connect error : %s", strerror (errno));
        free (reader);
        return NULL;
    }

    reader->buffer = malloc (2*FRAME_MAX_SIZE);
    if (reader->buffer == NULL)
    {
        ARSAL_PRINT (ARSAL_PRINT_ERROR, __TAG__, "Malloc error");
        ARSAL_Socket_Close (reader->socket);
        free (reader);
        return NULL;
    }

    ARSAL_PRINT (ARSAL_PRINT_WARNING, __TAG__, "Connected !");

    return reader;
}

int ARSTREAM_TCPReader_ReadFrame (ARSTREAM_TCPReader_t *reader, uint8_t *frame, uint32_t capacity)
{
    if ((reader == NULL) ||
        (frame  == NULL))
    {
        return -1;
    }

    /*
     * Read frame algo :
     * 1> Read data until the buffer contains the HEADER_MAGIC number
     * 2> Discard all data before the number
     * 3> Read data until the buffer contains header.size bytes
     * 4> Copy this data to frame, and return its size
     */

    int magicFound = 0;
    int magicIndex = 0;
    ARSTREAM_TCPReader_FrameHeader_t header;
    while (magicFound == 0)
    {
        if ((reader->index + READSIZE) > capacity)
        {
            reader->index = 0;
        }
        int nbRead = ARSAL_Socket_Recv (reader->socket, &(reader->buffer [reader->index]), READSIZE, 0);
        if (nbRead < 0)
        {
            ARSAL_PRINT (ARSAL_PRINT_ERROR, __TAG__, "Read error %s", strerror (errno));
            break;
        }

        reader->index += nbRead;

        int i;
        for (i = 0; i < (reader->index - sizeof header); i++)
        {
            uint32_t magic = 0;
            memcpy ((uint8_t *)&magic, &(reader->buffer[i]), sizeof magic);
            if (magic == HEADER_MAGIC)
            {
                magicFound = 1;
                magicIndex = i;
                break;
            }
        }
    }

    if (magicFound == 1)
    {
        memcpy (&header, &(reader->buffer [magicIndex]), sizeof header);
        reader->index -= (magicIndex + sizeof header);
        memmove (reader->buffer, &(reader->buffer [magicIndex + sizeof header]), reader->index);
    }
    else
    {
        return -1;
    }

    int frameComplete = 0;
    while (frameComplete == 0)
    {
        if (reader->index + READSIZE > capacity)
        {
            reader->index = 0;
        }
        int nbRead = ARSAL_Socket_Recv (reader->socket, &(reader->buffer [reader->index]), READSIZE, 0);
        if (nbRead < 0)
        {
            ARSAL_PRINT (ARSAL_PRINT_ERROR, __TAG__, "Read error %s", strerror (errno));
            break;
        }

        reader->index += nbRead;

        if (reader->index >= header.size)
        {
            frameComplete = 1;
        }
    }

    if (frameComplete == 1)
    {
        if (header.size <= capacity)
        {
            memcpy (frame, reader->buffer, header.size);
        }
        else
        {
            ARSAL_PRINT (ARSAL_PRINT_ERROR, __TAG__, "Can't copy frame -> Too big for buffer (%d / %d)", header.size, capacity);
        }
        reader->index -= header.size;
        memmove (reader->buffer, &(reader->buffer [header.size]), reader->index);
    }
    else
    {
        return -1;
    }

    return header.size;

}

void ARSTREAM_TCPReader_Delete (ARSTREAM_TCPReader_t **reader)
{
    if ((reader != NULL) &&
        (*reader != NULL))
    {
        ARSAL_Socket_Close ((*reader)->socket);
        free ((*reader)->buffer);
        free (*reader);
        *reader = NULL;
    }
}

/*
 * Implementation
 */

int ARSTREAM_TCPReader_Main (int argc, char *argv[])
{
    int retVal = 0;

    char *ip = __IP;
    struct timespec now;
    struct timespec prev;
    int dt;

    uint8_t *frameBuffer = malloc (FRAME_MAX_SIZE);
    if (frameBuffer == NULL)
    {
        ARSAL_PRINT (ARSAL_PRINT_ERROR, __TAG__, "Malloc error");
        return 1;
    }

    if (argc >= 2)
    {
        ip = argv[1];
    }

    ARSTREAM_TCPReader_t *reader = ARSTREAM_TCPReader_Create (DATA_PORT, ip);

    stillRunning = 1;

    ARSAL_Time_GetTime (&prev);

    while (stillRunning)
    {
        int size = ARSTREAM_TCPReader_ReadFrame (reader, frameBuffer, FRAME_MAX_SIZE);
        if (size > 0)
        {
            ARSAL_PRINT (ARSAL_PRINT_WARNING, __TAG__, "Got a frame of size %d", size);
            ARSAL_Time_GetTime (&now);
            dt = ARSAL_Time_ComputeTimespecMsTimeDiff (&prev, &now);
            lastDt [currentIndexInDt] = dt;
            currentIndexInDt ++;
            currentIndexInDt %= NB_FRAMES_FOR_AVERAGE;
            prev.tv_sec = now.tv_sec;
            prev.tv_nsec = now.tv_nsec;
        }
        else
        {
            ARSAL_PRINT (ARSAL_PRINT_ERROR, __TAG__, "Error while reading a frame");
            usleep (1000);
        }
    }

    ARSTREAM_TCPReader_Delete (&reader);

    return retVal;
}

void ARSTREAM_TCPReader_Stop ()
{
    stillRunning = 0;
}

int ARSTREAM_TCPReader_GetMeanTimeBetweenFrames ()
{
    int retVal = 0;
    int i;
    for (i = 0; i < NB_FRAMES_FOR_AVERAGE; i++)
    {
        retVal += lastDt [i];
    }
    return retVal / NB_FRAMES_FOR_AVERAGE;
}
