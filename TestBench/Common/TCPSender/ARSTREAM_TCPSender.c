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
 * @file ARSTREAM_Sender_TestBench.c
 * @brief Implementation file for the Drone2-like stream test
 * @date 07/10/2013
 * @author nicolas.brulez@parrot.com
 */

/*
 * System Headers
 */

#include <errno.h>
#include <stdlib.h>
#include <pthread.h>

/*
 * ARSDK Headers
 */

#include <libARSAL/ARSAL_Print.h>
#include <libARSAL/ARSAL_Socket.h>

/*
 * Macros
 */

#define DATA_PORT (32154)

#define BITRATE_KBPS (1200)
#define FPS          (30)

#define FRAME_SIZE   (1000 * BITRATE_KBPS / FPS / 8)

#define FRAME_MIN_SIZE (FRAME_SIZE)
#define FRAME_MAX_SIZE (FRAME_SIZE)

#define I_FRAME_EVERY_N (15)
#define NB_BUFFERS (2 * I_FRAME_EVERY_N)

#define TIME_BETWEEN_FRAMES_MS (1000 / FPS)

#define __TAG__ "ARSTREAM_TCPSender_TB"

#define NB_FRAMES_FOR_AVERAGE (15)

#define HEADER_MAGIC (0x37333331)

/*
 * Types
 */

typedef struct {
    int socket;
    int csocket;
} ARSTREAM_TCPSender_t;

typedef struct {
    uint32_t header;
    uint32_t size;
    uint32_t num;
} ARSTREAM_TCPSender_FrameHeader_t;

/*
 * Globals
 */

static int gConnected = 0;
static int stillRunning = 1;

/*
 * Internal function declaration
 */

/**
 * @brief Fake encoder thread function
 * This function generates dummy frames periodically, and sends them through the ARSTREAM_TCPSender
 * @param params ARSTREAM_TCPSender_t instance
 * @return No meaningful value : (void *)0
 */
void* TCP_fakeEncoderThread (void *params);

/**
 * Create and connect a new TCPSender
 */
ARSTREAM_TCPSender_t *ARSTREAM_TCPSender_Create (int port);

/**
 * Send a frame through an ARSTREAM_TCPSender_t
 */
void ARSTREAM_TCPSender_SendFrame (ARSTREAM_TCPSender_t *sender, ARSTREAM_TCPSender_FrameHeader_t *header, uint8_t *frame);

/**
 * Delete an ARSTREAM_TCPSender_t
 */
void ARSTREAM_TCPSender_Delete (ARSTREAM_TCPSender_t **sender);

/*
 * Internal functions implementation
 */

void* TCP_fakeEncoderThread (void *param)
{
    uint8_t *buffer;
    ARSTREAM_TCPSender_FrameHeader_t header = { HEADER_MAGIC, 0, 0 };
    ARSTREAM_TCPSender_t *sender = (ARSTREAM_TCPSender_t *)param;
    srand (time (NULL));
    ARSAL_PRINT (ARSAL_PRINT_WARNING, __TAG__, "Encoder thread running");
    buffer = malloc (FRAME_MAX_SIZE);
    if (buffer == NULL)
    {
        ARSAL_PRINT (ARSAL_PRINT_ERROR, __TAG__, "Alloc error !");
        stillRunning = 0;
    }
    while (stillRunning)
    {
        header.num ++;
        int frameSize = rand () % FRAME_MAX_SIZE;
        if (frameSize < FRAME_MIN_SIZE)
        {
            frameSize = FRAME_MIN_SIZE;
        }
        header.size = frameSize;
        ARSAL_PRINT (ARSAL_PRINT_WARNING, __TAG__, "Generating a frame of size %d with number %u", header.size, header.num);

        memset (buffer, header.num, header.size);
        ARSTREAM_TCPSender_SendFrame (sender, &header, buffer);
        usleep (1000 * TIME_BETWEEN_FRAMES_MS);
    }
    ARSAL_PRINT (ARSAL_PRINT_WARNING, __TAG__, "Encoder thread ended");
    return (void *)0;
}

ARSTREAM_TCPSender_t *ARSTREAM_TCPSender_Create (int port)
{
    ARSTREAM_TCPSender_t *sender = malloc (sizeof (ARSTREAM_TCPSender_t));
    if (sender == NULL)
    {
        return NULL;
    }

    sender->socket = ARSAL_Socket_Create (AF_INET, SOCK_STREAM, 0);

    if (sender->socket == -1)
    {
        ARSAL_PRINT (ARSAL_PRINT_ERROR, __TAG__, "Socket error : %s", strerror (errno));
        free (sender);
        return NULL;
    }

    struct sockaddr_in sin = {0};
    struct sockaddr_in csin = {0};
    socklen_t csinsize = sizeof (csin);
    sin.sin_addr.s_addr = htonl (INADDR_ANY);
    sin.sin_family = AF_INET;
    sin.sin_port = htons(port);

    if (ARSAL_Socket_Bind (sender->socket, (struct sockaddr *)&sin, sizeof (sin)) == -1)
    {
        ARSAL_PRINT (ARSAL_PRINT_ERROR, __TAG__, "Bind error : %s", strerror (errno));
        free (sender);
        return NULL;
    }

    if (ARSAL_Socket_Listen (sender->socket, 1) == -1)
    {
        ARSAL_PRINT (ARSAL_PRINT_ERROR, __TAG__, "Listen error : %s", strerror (errno));
        free (sender);
        return NULL;
    }

    sender->csocket = ARSAL_Socket_Accept (sender->socket, (struct sockaddr *)&csin, &csinsize);

    if (sender->csocket == -1)
    {
        ARSAL_PRINT (ARSAL_PRINT_ERROR, __TAG__, "Accept error : %s", strerror (errno));
        free (sender);
        return NULL;
    }

    ARSAL_PRINT (ARSAL_PRINT_WARNING, __TAG__, "Connected !");

    return sender;
}

void ARSTREAM_TCPSender_SendFrame (ARSTREAM_TCPSender_t *sender, ARSTREAM_TCPSender_FrameHeader_t *header, uint8_t *frame)
{
    if ((sender == NULL) ||
        (header == NULL) ||
        (frame  == NULL))
    {
        return;
    }

    ARSAL_Socket_Send (sender->csocket, header, sizeof (ARSTREAM_TCPSender_FrameHeader_t), 0);
    ARSAL_Socket_Send (sender->csocket, frame, header->size, 0);
}

void ARSTREAM_TCPSender_Delete (ARSTREAM_TCPSender_t **sender)
{
    if ((sender != NULL) &&
        (*sender != NULL))
    {
        ARSAL_Socket_Close ((*sender)->csocket);
        ARSAL_Socket_Close ((*sender)->socket);
        free (*sender);
        *sender = NULL;
    }
}

/*
 * Implementation
 */

int ARSTREAM_TCPSender_Main (int argc, char *argv[])
{
    int retVal = 0;

    ARSTREAM_TCPSender_t *sender = ARSTREAM_TCPSender_Create (DATA_PORT);

    gConnected = 1;
    stillRunning = 1;

    pthread_t enc;
    pthread_create (&enc, NULL, TCP_fakeEncoderThread, sender);
    pthread_join (enc, NULL);

    ARSTREAM_TCPSender_Delete (&sender);

    gConnected = 0;

    return retVal;
}

void ARSTREAM_TCPSender_Stop ()
{
    stillRunning = 0;
}

int ARSTREAM_TCPSender_IsConnected ()
{
    return gConnected;
}
