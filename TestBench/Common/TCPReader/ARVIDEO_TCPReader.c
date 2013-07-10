/**
 * @file ARVIDEO_Reader_TestBench.c
 * @brief Implementation file for the Drone2-like video test
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

#define __TAG__ "ARVIDEO_TCPReader_TB"

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
} ARVIDEO_TCPReader_t;

typedef struct {
    uint32_t header;
    uint32_t size;
    uint32_t num;
} ARVIDEO_TCPReader_FrameHeader_t;

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
ARVIDEO_TCPReader_t *ARVIDEO_TCPReader_Create (int port, const char *ip);

/**
 * Send a frame through an ARVIDEO_TCPReader_t
 */
int ARVIDEO_TCPReader_ReadFrame (ARVIDEO_TCPReader_t *reader, uint8_t *frame, uint32_t capacity);

/**
 * Delete an ARVIDEO_TCPReader_t
 */
void ARVIDEO_TCPReader_Delete (ARVIDEO_TCPReader_t **reader);

/*
 * Internal functions implementation
 */

ARVIDEO_TCPReader_t *ARVIDEO_TCPReader_Create (int port, const char *ip)
{
    ARVIDEO_TCPReader_t *reader = malloc (sizeof (ARVIDEO_TCPReader_t));
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

int ARVIDEO_TCPReader_ReadFrame (ARVIDEO_TCPReader_t *reader, uint8_t *frame, uint32_t capacity)
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
    ARVIDEO_TCPReader_FrameHeader_t header;
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

void ARVIDEO_TCPReader_Delete (ARVIDEO_TCPReader_t **reader)
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

int ARVIDEO_TCPReader_Main (int argc, char *argv[])
{
    int retVal = 0;

    char *ip = __IP;
    struct timeval now;
    struct timeval prev;
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

    ARVIDEO_TCPReader_t *reader = ARVIDEO_TCPReader_Create (DATA_PORT, ip);

    stillRunning = 1;

    gettimeofday (&prev, NULL);

    while (stillRunning)
    {
        int size = ARVIDEO_TCPReader_ReadFrame (reader, frameBuffer, FRAME_MAX_SIZE);
        if (size > 0)
        {
            ARSAL_PRINT (ARSAL_PRINT_WARNING, __TAG__, "Got a frame of size %d", size);
            gettimeofday (&now, NULL);
            dt = ARSAL_Time_ComputeMsTimeDiff (&prev, &now);
            lastDt [currentIndexInDt] = dt;
            currentIndexInDt ++;
            currentIndexInDt %= NB_FRAMES_FOR_AVERAGE;
            prev.tv_sec = now.tv_sec;
            prev.tv_usec = now.tv_usec;
        }
        else
        {
            ARSAL_PRINT (ARSAL_PRINT_ERROR, __TAG__, "Error while reading a frame");
            usleep (1000);
        }
    }

    ARVIDEO_TCPReader_Delete (&reader);

    return retVal;
}

void ARVIDEO_TCPReader_Stop ()
{
    stillRunning = 0;
}

int ARVIDEO_TCPReader_GetMeanTimeBetweenFrames ()
{
    int retVal = 0;
    int i;
    for (i = 0; i < NB_FRAMES_FOR_AVERAGE; i++)
    {
        retVal += lastDt [i];
    }
    return retVal / NB_FRAMES_FOR_AVERAGE;
}
