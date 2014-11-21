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
 * @file ARSTREAM_MP4Sender_TestBench.c
 * @brief Testbench for the ARSTREAM_Sender submodule
 * This testbench streams frames from a mp4 file
 * @date 04/18/2013
 * @author nicolas.brulez@parrot.com
 */

/*
 * System Headers
 */

#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <errno.h>

/*
 * ARSDK Headers
 */

#include <libARSAL/ARSAL_Print.h>
#include <libARStream/ARSTREAM_Sender.h>

#include "../ARSTREAM_TB_Config.h"

/*
 * Macros
 */

#define ACK_BUFFER_ID (13)
#define DATA_BUFFER_ID (125)

#define SENDING_PORT (54321)
#define READING_PORT (43210)

#define NB_BUFFERS (40)
#define I_FRAME_EVERY_N (30)

#define TEST_MODE (0)

#if TEST_MODE
# define TIME_BETWEEN_FRAMES_MS (1000)
#else
# define TIME_BETWEEN_FRAMES_MS (33)
#endif

#define __TAG__ "ARSTREAM_MP4Sender_TB"

#define MP4SENDER_PING_DELAY (0) // Use default value

#ifndef __IP
#define __IP "127.0.0.1"
#endif

/*
 * Types
 */

/*
 * Globals
 */

static int stillRunning = 1;

float ARSTREAM_MP4Sender_PercentOk = 0.0;
static int nbSent = 0;
static int nbOk = 0;

static int currentBufferIndex = 0;
static uint8_t *multiBuffer[NB_BUFFERS];
static uint32_t multiBufferSize[NB_BUFFERS];
static int multiBufferIsFree[NB_BUFFERS];

static char *appName;

static FILE *mp4File;
static int mp4NbFrames;
static int mp4CurrentFrame;
static uint32_t *framesOffsetArray;
static uint32_t *framesSizeArray;

/*
 * Internal functions declarations
 */

/**
 * @brief Print the parameters of the application
 */
void ARSTREAM_MP4SenderTb_printUsage ();

/**
 * @brief Initializes the multi buffers of the testbench
 * @param maxsize Maximum size of a frame in the current mp4 file
 */
void ARSTREAM_MP4SenderTb_initMultiBuffers (int maxsize);

/**
 * @see ARSTREAM_Sender.h
 */
void ARSTREAM_MP4SenderTb_FrameUpdateCallback (eARSTREAM_SENDER_STATUS status, uint8_t *framePointer, uint32_t frameSize, void *custom);

/**
 * @brief Gets a free buffer pointer
 * @param[in] buffer the buffer to mark as free
 */
void ARSTREAM_MP4SenderTb_SetBufferFree (uint8_t *buffer);

/**
 * @brief Gets a free buffer pointer
 * @param[out] retSize Pointer where to store the size of the next free buffer (or 0 if there is no free buffer)
 * @return Pointer to the next free buffer, or NULL if there is no free buffer
 */
uint8_t* ARSTREAM_MP4SenderTb_GetNextFreeBuffer (uint32_t *retSize);

/**
 * @brief File reader thread function
 * This function reads frames periodically from the given mp4 file, and sends them through the ARSTREAM_Sender_t
 * @param ARSTREAM_Sender_t_Param A valid ARSTREAM_Sender_t, casted as a (void *), which will be used by the thread
 * @return No meaningful value : (void *)0
 *
 * @note This function loops through the mp4 file
 */
void* fileReaderThread (void *ARSTREAM_Sender_t_Param);

/**
 * @brief Stream entry point
 * @param fpath path to the mp4 file to read
 * @param manager An initialized network manager
 * @return "Main" return value
 */
int ARSTREAM_MP4SenderTb_StartStreamTest (const char *fpath, ARNETWORK_Manager_t *manager);

/**
 * @brief Opens the file for reading + get the largest frame size
 * @param path Path of the mp4 file
 * @return the size of the largest frame in the file
 */
int ARSTREAM_MP4SenderTb_OpenStreamFile (const char *path);

/**
 * @brief Get the "next" frame from file
 * @param nextFrame buffer in which to store nextFrame
 * @param nextFrameSize size of the nextFrame buffer
 * @param isIFrame pointer to an int which will hold this boolean-like flag
 * @return actual data size of the nextFrame
 *
 * @note After reading the last frame from the file, this function goes back to the beginning
 */
uint32_t ARSTREAM_MP4SenderTb_GetNextFrame (uint8_t *nextFrame, int nextFrameSize, int *isIFrame);

/**
 * @brief seeks the global mp4File to the given atom
 * @param atomName 4CC of the atom to seek
 * @return size of the atom (-1 means failure)
 */
int ARSTREAM_MP4SenderTb_JumpToAtom (const char *atomName);

/*
 * Internal functions implementation
 */

void ARSTREAM_MP4SenderTb_printUsage ()
{
    ARSAL_PRINT (ARSAL_PRINT_ERROR, __TAG__, "Usage : %s file [ip]", appName);
    ARSAL_PRINT (ARSAL_PRINT_ERROR, __TAG__, "        file -> mp4 file to read from");
    ARSAL_PRINT (ARSAL_PRINT_ERROR, __TAG__, "        ip -> optionnal, ip of the stream reader");
}

void ARSTREAM_MP4SenderTb_initMultiBuffers (int maxsize)
{
    int buffIndex;
    for (buffIndex = 0; buffIndex < NB_BUFFERS; buffIndex++)
    {
        multiBuffer[buffIndex] = malloc (maxsize);
        multiBufferSize[buffIndex] = maxsize;
        multiBufferIsFree[buffIndex] = 1;
    }
}

void ARSTREAM_MP4SenderTb_FrameUpdateCallback (eARSTREAM_SENDER_STATUS status, uint8_t *framePointer, uint32_t frameSize, void *custom)
{
    custom = custom;
    switch (status)
    {
    case ARSTREAM_SENDER_STATUS_FRAME_SENT:
        ARSTREAM_MP4SenderTb_SetBufferFree (framePointer);
        ARSAL_PRINT (ARSAL_PRINT_WARNING, __TAG__, "Successfully sent a frame of size %u", frameSize);
        nbSent++;
        nbOk++;
        ARSTREAM_MP4Sender_PercentOk = (100.f * nbOk) / (1.f * nbSent);
        break;
    case ARSTREAM_SENDER_STATUS_FRAME_CANCEL:
        ARSTREAM_MP4SenderTb_SetBufferFree (framePointer);
        ARSAL_PRINT (ARSAL_PRINT_WARNING, __TAG__, "Cancelled a frame of size %u", frameSize);
        nbSent++;
        ARSTREAM_MP4Sender_PercentOk = (100.f * nbOk) / (1.f * nbSent);
        break;
    default:
        // All cases handled
        break;
    }
}

void ARSTREAM_MP4SenderTb_SetBufferFree (uint8_t *buffer)
{
    int i;
    for (i = 0; i < NB_BUFFERS; i++)
    {
        if (multiBuffer[i] == buffer)
        {
            multiBufferIsFree[i] = 1;
        }
    }
}

uint8_t* ARSTREAM_MP4SenderTb_GetNextFreeBuffer (uint32_t *retSize)
{
    uint8_t *retBuffer = NULL;
    int nbtest = 0;
    if (retSize == NULL)
    {
        return NULL;
    }
    do
    {
        if (multiBufferIsFree[currentBufferIndex] == 1)
        {
            retBuffer = multiBuffer[currentBufferIndex];
            *retSize = multiBufferSize[currentBufferIndex];
        }
        currentBufferIndex = (currentBufferIndex + 1) % NB_BUFFERS;
        nbtest++;
    } while (retBuffer == NULL && nbtest < NB_BUFFERS);
    return retBuffer;
}

void* fileReaderThread (void *ARSTREAM_Sender_t_Param)
{

    uint8_t cnt = 0;
    uint32_t frameSize = 0;
    uint32_t frameCapacity = 0;
    uint8_t *nextFrameAddr;
    ARSTREAM_Sender_t *sender = (ARSTREAM_Sender_t *)ARSTREAM_Sender_t_Param;
    srand (time (NULL));
    ARSAL_PRINT (ARSAL_PRINT_WARNING, __TAG__, "Encoder thread running");
    while (stillRunning)
    {
        int flush;
        nextFrameAddr = ARSTREAM_MP4SenderTb_GetNextFreeBuffer (&frameCapacity);
        frameSize = ARSTREAM_MP4SenderTb_GetNextFrame (nextFrameAddr, frameCapacity, &flush);
        cnt++;

        if (frameSize != 0)
        {
            int nbPrevious = 0;
            eARSTREAM_ERROR res = ARSTREAM_Sender_SendNewFrame (sender, nextFrameAddr, frameSize, flush, &nbPrevious);
            switch (res)
            {
            case ARSTREAM_OK:
                ARSAL_PRINT (ARSAL_PRINT_WARNING, __TAG__, "Added a frame of size %u to the Sender (already %d in queue)", frameSize, nbPrevious);
                break;
            case ARSTREAM_ERROR_BAD_PARAMETERS:
            case ARSTREAM_ERROR_FRAME_TOO_LARGE:
            case ARSTREAM_ERROR_QUEUE_FULL:
                ARSAL_PRINT (ARSAL_PRINT_ERROR, __TAG__, "Unable to send the new frame : %s", ARSTREAM_Error_ToString(res));
                break;
            default:
                ARSAL_PRINT (ARSAL_PRINT_ERROR, __TAG__, "Unknown error code for SendNewFrame");
                break;
            }
        }
        else
        {
            ARSAL_PRINT (ARSAL_PRINT_ERROR, __TAG__, "Could not get a new encoded frame");
        }

        usleep (1000 * TIME_BETWEEN_FRAMES_MS);
    }
    ARSAL_PRINT (ARSAL_PRINT_WARNING, __TAG__, "Encoder thread ended");
    return (void *)0;
}

int ARSTREAM_MP4SenderTb_StartStreamTest (const char *fpath, ARNETWORK_Manager_t *manager)
{
    int retVal = 0;
    eARSTREAM_ERROR err;
    ARSTREAM_Sender_t *sender;
    ARSTREAM_MP4SenderTb_initMultiBuffers (ARSTREAM_MP4SenderTb_OpenStreamFile (fpath));
    sender = ARSTREAM_Sender_New (manager, DATA_BUFFER_ID, ACK_BUFFER_ID, ARSTREAM_MP4SenderTb_FrameUpdateCallback, NB_BUFFERS, ARSTREAM_TB_FRAG_SIZE, ARSTREAM_TB_MAX_NB_FRAG, NULL, &err);
    if (sender == NULL)
    {
        ARSAL_PRINT (ARSAL_PRINT_ERROR, __TAG__, "Error during ARSTREAM_Sender_New call : %s", ARSTREAM_Error_ToString(err));
        return 1;
    }

    pthread_t streamsend, streamread;
    pthread_create (&streamsend, NULL, ARSTREAM_Sender_RunDataThread, sender);
    pthread_create (&streamread, NULL, ARSTREAM_Sender_RunAckThread, sender);

    /* USER CODE */

    pthread_t sourceThread;
    pthread_create (&sourceThread, NULL, fileReaderThread, sender);
    pthread_join (sourceThread, NULL);

    /* END OF USER CODE */

    ARSTREAM_Sender_StopSender (sender);

    pthread_join (streamread, NULL);
    pthread_join (streamsend, NULL);

    ARSTREAM_Sender_Delete (&sender);

    return retVal;
}

int ARSTREAM_MP4SenderTb_OpenStreamFile (const char *path)
{
    int res = 1;
    int greatest = 0;
    mp4File = fopen (path, "rb");
    if (NULL == mp4File)
    {
        perror ("Open error");
        exit (1);
    }

    /* Jump to stsz atom */
    res = ARSTREAM_MP4SenderTb_JumpToAtom ("moov");
    if (res != -1)
    {
        res = ARSTREAM_MP4SenderTb_JumpToAtom ("trak");
    }
    if (res != -1)
    {
        res = ARSTREAM_MP4SenderTb_JumpToAtom ("mdia");
    }
    if (res != -1)
    {
        res = ARSTREAM_MP4SenderTb_JumpToAtom ("minf");
    }
    if (res != -1)
    {
        res = ARSTREAM_MP4SenderTb_JumpToAtom ("stbl");
    }
    if (res != -1)
    {
        res = ARSTREAM_MP4SenderTb_JumpToAtom ("stsz");
    }

    /* Read nbFrames + all frames sizes */
    if (res != -1)
    {
        uint8_t *data = malloc (res);
        res = fread (data, 1, res, mp4File);
        uint32_t neNbFrames = *(uint32_t *)&data[8];
        mp4NbFrames = ntohl (neNbFrames);
        framesSizeArray = malloc (mp4NbFrames * sizeof (uint32_t));
        memcpy (framesSizeArray, &data[12], mp4NbFrames * sizeof (uint32_t));
        int i;
        for (i = 0; i < mp4NbFrames; i++)
        {
            framesSizeArray [i] = ntohl (framesSizeArray [i]);
            if (framesSizeArray [i] > greatest)
            {
                greatest = framesSizeArray [i];
            }
        }
        free (data);
    }

    /* Rewind file */
    rewind (mp4File);

    /* Jump to stco atom */
    if (res != -1)
    {
        res = ARSTREAM_MP4SenderTb_JumpToAtom ("moov");
    }
    if (res != -1)
    {
        res = ARSTREAM_MP4SenderTb_JumpToAtom ("trak");
    }
    if (res != -1)
    {
        res = ARSTREAM_MP4SenderTb_JumpToAtom ("mdia");
    }
    if (res != -1)
    {
        res = ARSTREAM_MP4SenderTb_JumpToAtom ("minf");
    }
    if (res != -1)
    {
        res = ARSTREAM_MP4SenderTb_JumpToAtom ("stbl");
    }
    if (res != -1)
    {
        res = ARSTREAM_MP4SenderTb_JumpToAtom ("stco");
    }

    /* Read nbFrames + all frames sizes */
    if (res != -1)
    {
        uint8_t *data = malloc (res);
        res = fread (data, 1, res, mp4File);
        framesOffsetArray = malloc (mp4NbFrames * sizeof (uint32_t));
        memcpy (framesOffsetArray, &data[8], mp4NbFrames * sizeof (uint32_t));
        int i;
        for (i = 0; i < mp4NbFrames; i++)
        {
            framesOffsetArray [i] = ntohl (framesOffsetArray [i]);
        }
        free (data);
    }

    mp4CurrentFrame = 0;
    return greatest;
}

uint32_t ARSTREAM_MP4SenderTb_GetNextFrame (uint8_t *nextFrame, int nextFrameSize, int *isIFrame)
{
    uint32_t nextSize = framesSizeArray [mp4CurrentFrame];
    uint32_t nextOffset = framesOffsetArray [mp4CurrentFrame];

    *isIFrame = 0;

    if (nextSize <= nextFrameSize)
    {
        fseek (mp4File, nextOffset, SEEK_SET);
        int readRes = fread (nextFrame, 1, nextSize, mp4File);
        if (readRes != nextSize)
        {
            ARSAL_PRINT (ARSAL_PRINT_ERROR, __TAG__, "Fread error: %s", strerror(errno));
        }
    }
    else
    {
        ARSAL_PRINT (ARSAL_PRINT_ERROR, __TAG__, "GetNextFrame error ... this is pretty bad !");
        return 0;
    }

    mp4CurrentFrame++;
    if (mp4CurrentFrame >= mp4NbFrames)
    {
        mp4CurrentFrame = 0;
    }

    *isIFrame = ((mp4CurrentFrame % I_FRAME_EVERY_N) == 1) ? 1 : 0;

    return nextSize;
}


/**
 * Read from a file functions
 * Note : fptr MUST be a valid file pointer
 */
static void read_uint32 (FILE *fptr, uint32_t *dest)
{
    uint32_t locValue = 0;
    if (1 != fread (&locValue, sizeof (locValue), 1, fptr))
    {
        fprintf (stderr, "Error reading a uint32_t\n");
    }
    *dest = ntohl (locValue);
}

static void read_uint64 (FILE *fptr, uint64_t *dest)
{
    uint64_t retValue = 0;
    uint32_t locValue = 0;
    if (1 != fread (&locValue, sizeof (locValue), 1, fptr))
    {
        fprintf (stderr, "Error reading low value of a uint64_t\n");
        return;
    }
    retValue = (uint64_t)(ntohl (locValue));
    if (1 != fread (&locValue, sizeof (locValue), 1, fptr))
    {
        fprintf (stderr, "Error reading a high value of a uint64_t\n");
    }
    retValue += ((uint64_t)(ntohl (locValue))) << 32;
    *dest = retValue;
}

void read_4cc (FILE *fptr, char dest[5])
{
    if (1 != fread (dest, 4, 1, fptr))
    {
        fprintf (stderr, "Error reading a 4cc\n");
    }
}

int ARSTREAM_MP4SenderTb_JumpToAtom (const char *atomName)
{
    uint32_t atomSize = 0;
    char fourCCTag [5] = {0};
    uint64_t wideAtomSize = 0;
    int found = 0;
    FILE *streamFile = mp4File;
    if (NULL == streamFile)
    {
        return 0;
    }

    read_uint32 (streamFile, &atomSize);
    read_4cc (streamFile, fourCCTag);
    if (0 == atomSize)
    {
        read_uint64 (streamFile, &wideAtomSize);
    }
    else
    {
        wideAtomSize = atomSize;
    }
    if (0 == strncmp (fourCCTag, atomName, 4))
    {
        found = 1;
    }

    while (0 == found &&
           !feof (streamFile))
    {
        fseek (streamFile, wideAtomSize - 8, SEEK_CUR);

        read_uint32 (streamFile, &atomSize);
        read_4cc (streamFile, fourCCTag);
        if (0 == atomSize)
        {
            read_uint64 (streamFile, &wideAtomSize);
        }
        else
        {
            wideAtomSize = atomSize;
        }
        if (0 == strncmp (fourCCTag, atomName, 4))
        {
            found = 1;
        }
    }
    return wideAtomSize;
}

/*
 * Implementation
 */

int ARSTREAM_MP4Sender_TestBenchMain (int argc, char *argv[])
{
    int retVal = 0;
    appName = argv[0];
    if (argc < 2)
    {
        ARSTREAM_MP4SenderTb_printUsage ();
        return 1;
    }

    char *fpath = argv[1];

    char *ip = __IP;

    if (argc >= 3)
    {
        ip = argv[2];
    }

    int nbInBuff = 1;
    ARNETWORK_IOBufferParam_t inParams;
    ARSTREAM_Sender_InitStreamDataBuffer (&inParams, DATA_BUFFER_ID, ARSTREAM_TB_FRAG_SIZE, ARSTREAM_TB_MAX_NB_FRAG);
    int nbOutBuff = 1;
    ARNETWORK_IOBufferParam_t outParams;
    ARSTREAM_Sender_InitStreamAckBuffer (&outParams, ACK_BUFFER_ID);

    eARNETWORK_ERROR error;
    eARNETWORKAL_ERROR specificError = ARNETWORKAL_OK;
    ARNETWORK_Manager_t *manager = NULL;
    ARNETWORKAL_Manager_t *osspecificManagerPtr = ARNETWORKAL_Manager_New(&specificError);

    if(specificError == ARNETWORKAL_OK)
    {
        specificError = ARNETWORKAL_Manager_InitWifiNetwork(osspecificManagerPtr, ip, SENDING_PORT, READING_PORT, 1000);
    }

    if(specificError == ARNETWORKAL_OK)
    {
        manager = ARNETWORK_Manager_New(osspecificManagerPtr, nbInBuff, &inParams, nbOutBuff, &outParams, MP4SENDER_PING_DELAY, NULL, NULL, &error);
    }
    else
    {
        error = ARNETWORK_ERROR;
    }

    if ((manager == NULL) ||
        (error != ARNETWORK_OK))
    {
        ARSAL_PRINT (ARSAL_PRINT_ERROR, __TAG__, "Error during ARNETWORK_Manager_New call : %d", error);
        return 1;
    }

    pthread_t netsend, netread;
    pthread_create (&netsend, NULL, ARNETWORK_Manager_SendingThreadRun, manager);
    pthread_create (&netread, NULL, ARNETWORK_Manager_ReceivingThreadRun, manager);

    stillRunning = 1;

    retVal = ARSTREAM_MP4SenderTb_StartStreamTest (fpath, manager);

    ARNETWORK_Manager_Stop (manager);

    pthread_join (netread, NULL);
    pthread_join (netsend, NULL);

    ARNETWORK_Manager_Delete (&manager);

    return retVal;
}

void ARSTREAM_MP4Sender_TestBenchStop ()
{
    stillRunning = 0;
}
