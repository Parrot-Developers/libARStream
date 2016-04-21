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
 * @brief Testbench for the ARSTREAM_Sender submodule
 * @date 03/25/2013
 * @author nicolas.brulez@parrot.com
 */

/*
 * System Headers
 */

#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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

#define BITRATE_KBPS (1)
#define FPS          (1)

#define FRAME_SIZE   (1000 * BITRATE_KBPS / FPS / 8)

#define FRAME_MIN_SIZE (FRAME_SIZE)
#define FRAME_MAX_SIZE (FRAME_SIZE)

#define I_FRAME_EVERY_N (15)
#define NB_BUFFERS (2 * I_FRAME_EVERY_N)

#define TIME_BETWEEN_FRAMES_MS (1000 / FPS)

#define __TAG__ "ARSTREAM_Sender_TB"

#define SENDER_PING_DELAY (0) // Use default value

#ifndef __IP
#define __IP "127.0.0.1"
#endif

#define NB_FRAMES_FOR_AVERAGE (15)

#define NB_FILTERS (10)

/*
 * Types
 */

typedef struct {
    int id;
} filter_ctx;

/*
 * Globals
 */

static ARSTREAM_Sender_t *g_Sender;
static ARNETWORK_Manager_t *g_Manager;

static int lastDt [NB_FRAMES_FOR_AVERAGE] = {0};
static int currentIndexInDt = 0;

static int nbSkippedSinceLast = 0;

static int stillRunning = 1;

float ARSTREAM_Sender_PercentOk = 0.0;
static int nbSent = 0;
static int nbOk = 0;

static int currentBufferIndex = 0;
static uint8_t *multiBuffer[NB_BUFFERS];
static uint32_t multiBufferSize[NB_BUFFERS];
static int multiBufferIsFree[NB_BUFFERS];

static char *appName;

static filter_ctx fctx[NB_FILTERS] = {};
static ARSTREAM_Filter_t filters[NB_FILTERS] = {};

/*
 * Internal functions declarations
 */

static uint8_t *getBuffer(void *context, int size)
{
    filter_ctx *ctx = (filter_ctx *)context;
    ARSAL_PRINT (ARSAL_PRINT_INFO, __TAG__, "getBuffer(%d) on filter %d", size, ctx->id);
    return malloc(size);
}

static int getOutputSize(void *context, int inputSize)
{
    filter_ctx *ctx = (filter_ctx *)context;
    ARSAL_PRINT (ARSAL_PRINT_INFO, __TAG__, "getOutputSize(%d) on filter %d", inputSize, ctx->id);
    return inputSize;
}

static int filterBuffer(void *context,
                        uint8_t *input, int inSize,
                        uint8_t *output, int outSize)
{
    filter_ctx *ctx = (filter_ctx *)context;
    ARSAL_PRINT (ARSAL_PRINT_INFO, __TAG__, "filterBuffer(...) on filter %d", ctx->id);
    int cpSize = inSize < outSize ? inSize : outSize;
    int i;
    memcpy(output, input, cpSize);
    output[1]++;
    return cpSize;
}

static void releaseBuffer(void *context, uint8_t *buffer)
{
    filter_ctx *ctx = (filter_ctx *)context;
    ARSAL_PRINT (ARSAL_PRINT_INFO, __TAG__, "releaseBuffer(%p) on filter %d", buffer, ctx->id);
    free(buffer);
}

static void ARSTREAM_SenderTB_AddFilters(void)
{
    int i;
    for (i = 0; i < NB_FILTERS; i++)
    {
        filters[i].getBuffer = getBuffer;
        filters[i].getOutputSize = getOutputSize;
        filters[i].filterBuffer = filterBuffer;
        filters[i].releaseBuffer = releaseBuffer;
        filters[i].context = &fctx[i];
        fctx[i].id = i;

        eARSTREAM_ERROR err = ARSTREAM_Sender_AddFilter(g_Sender, &filters[i]);
        if (err != ARSTREAM_OK)
        {
            ARSAL_PRINT(ARSAL_PRINT_ERROR, __TAG__, "Error while adding filter : %s", ARSTREAM_Error_ToString(err));
        }
    }
}


/**
 * @brief Print the parameters of the application
 */
void ARSTREAM_SenderTb_printUsage ();

/**
 * @brief Initializes the multi buffers of the testbench
 */
void ARSTREAM_SenderTb_initMultiBuffers ();

/**
 * @see ARSTREAM_Sender.h
 */
void ARSTREAM_SenderTb_FrameUpdateCallback (eARSTREAM_SENDER_STATUS status, uint8_t *framePointer, uint32_t frameSize, void *custom);

/**
 * @brief Gets a free buffer pointer
 * @param[in] buffer the buffer to mark as free
 */
void ARSTREAM_SenderTb_SetBufferFree (uint8_t *buffer);

/**
 * @brief Gets a free buffer pointer
 * @param[out] retSize Pointer where to store the size of the next free buffer (or 0 if there is no free buffer)
 * @return Pointer to the next free buffer, or NULL if there is no free buffer
 */
uint8_t* ARSTREAM_SenderTb_GetNextFreeBuffer (uint32_t *retSize);

/**
 * @brief Fake encoder thread function
 * This function generates dummy frames periodically, and sends them through the ARSTREAM_Sender_t
 * @param ARSTREAM_Sender_t_Param A valid ARSTREAM_Sender_t, casted as a (void *), which will be used by the thread
 * @return No meaningful value : (void *)0
 */
void* fakeEncoderThread (void *ARSTREAM_Sender_t_Param);

/**
 * @brief Stream entry point
 * @param manager An initialized network manager
 * @return "Main" return value
 */
int ARSTREAM_SenderTb_StartStreamTest (ARNETWORK_Manager_t *manager);

/*
 * Internal functions implementation
 */

void ARSTREAM_SenderTb_printUsage ()
{
    ARSAL_PRINT (ARSAL_PRINT_ERROR, __TAG__, "Usage : %s [ip]", appName);
    ARSAL_PRINT (ARSAL_PRINT_ERROR, __TAG__, "        ip -> optionnal, ip of the stream reader");
}

void ARSTREAM_SenderTb_initMultiBuffers ()
{
    int buffIndex;
    for (buffIndex = 0; buffIndex < NB_BUFFERS; buffIndex++)
    {
        multiBuffer[buffIndex] = malloc (FRAME_MAX_SIZE);
        multiBufferSize[buffIndex] = FRAME_MAX_SIZE;
        multiBufferIsFree[buffIndex] = 1;
    }
}

void ARSTREAM_SenderTb_FrameUpdateCallback (eARSTREAM_SENDER_STATUS status, uint8_t *framePointer, uint32_t frameSize, void *custom)
{
    static struct timespec prev = {0};
    struct timespec now = {0};
    int dt;
    custom = custom;
    switch (status)
    {
    case ARSTREAM_SENDER_STATUS_FRAME_SENT:
        ARSTREAM_SenderTb_SetBufferFree (framePointer);
        ARSAL_PRINT (ARSAL_PRINT_WARNING, __TAG__, "Successfully sent a frame of size %u", frameSize);
        ARSAL_Time_GetTime(&now);
        dt = ARSAL_Time_ComputeTimespecMsTimeDiff(&prev, &now);
        prev.tv_sec = now.tv_sec;
        prev.tv_nsec = now.tv_nsec;
        lastDt[currentIndexInDt] = dt;
        currentIndexInDt ++;
        currentIndexInDt %= NB_FRAMES_FOR_AVERAGE;
        nbSent++;
        nbOk++;
        ARSTREAM_Sender_PercentOk = (100.f * nbOk) / (1.f * nbSent);
        break;
    case ARSTREAM_SENDER_STATUS_FRAME_CANCEL:
        ARSTREAM_SenderTb_SetBufferFree (framePointer);
        ARSAL_PRINT (ARSAL_PRINT_WARNING, __TAG__, "Cancelled a frame of size %u", frameSize);
        nbSent++;
        nbSkippedSinceLast++;
        ARSTREAM_Sender_PercentOk = (100.f * nbOk) / (1.f * nbSent);
        break;
    case ARSTREAM_SENDER_STATUS_FRAME_LATE_ACK:
        ARSAL_PRINT (ARSAL_PRINT_WARNING, __TAG__, "Received a late acknowledge");
        nbOk++;
        ARSTREAM_Sender_PercentOk = (100.f * nbOk) / (1.f * nbSent);
        break;
    default:
        // All cases handled
        break;
    }
}

void ARSTREAM_SenderTb_SetBufferFree (uint8_t *buffer)
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

uint8_t* ARSTREAM_SenderTb_GetNextFreeBuffer (uint32_t *retSize)
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

void* fakeEncoderThread (void *ARSTREAM_Sender_t_Param)
{

    uint32_t cnt = 0;
    uint32_t frameSize = 0;
    uint32_t frameCapacity = 0;
    uint8_t *nextFrameAddr;
    ARSTREAM_Sender_t *sender = (ARSTREAM_Sender_t *)ARSTREAM_Sender_t_Param;
    srand (time (NULL));
    ARSAL_PRINT (ARSAL_PRINT_WARNING, __TAG__, "Encoder thread running");
    while (stillRunning)
    {
        cnt++;
        frameSize = rand () % FRAME_MAX_SIZE;
        if (frameSize < FRAME_MIN_SIZE)
        {
            frameSize = FRAME_MIN_SIZE;
        }
        ARSAL_PRINT (ARSAL_PRINT_WARNING, __TAG__, "Generating a frame of size %d with number %u", frameSize, cnt);

        nextFrameAddr = ARSTREAM_SenderTb_GetNextFreeBuffer (&frameCapacity);
        if (nextFrameAddr != NULL)
        {
            if (frameCapacity >= frameSize)
            {
                eARSTREAM_ERROR res;
                int nbPrevious;
                int flush = ((cnt % I_FRAME_EVERY_N) == 1) ? 1 : 0;
                memset (nextFrameAddr, cnt, frameSize);
                res = ARSTREAM_Sender_SendNewFrame (sender, nextFrameAddr, frameSize, flush, &nbPrevious);
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
                ARSAL_PRINT (ARSAL_PRINT_ERROR, __TAG__, "Could not encode a new frame : buffer too small !");
            }
        }
        else
        {
            ARSAL_PRINT (ARSAL_PRINT_ERROR, __TAG__, "Could not encode a new frame : no free buffer !");
        }

        usleep (1000 * TIME_BETWEEN_FRAMES_MS);
    }
    ARSAL_PRINT (ARSAL_PRINT_WARNING, __TAG__, "Encoder thread ended");
    return (void *)0;
}

int ARSTREAM_SenderTb_StartStreamTest (ARNETWORK_Manager_t *manager)
{
    int retVal = 0;
    eARSTREAM_ERROR err;
    ARSTREAM_SenderTb_initMultiBuffers ();
    g_Sender = ARSTREAM_Sender_New (manager, DATA_BUFFER_ID, ACK_BUFFER_ID, ARSTREAM_SenderTb_FrameUpdateCallback, NB_BUFFERS, ARSTREAM_TB_FRAG_SIZE, ARSTREAM_TB_MAX_NB_FRAG, NULL, &err);
    if (g_Sender == NULL)
    {
        ARSAL_PRINT (ARSAL_PRINT_ERROR, __TAG__, "Error during ARSTREAM_Sender_New call : %s", ARSTREAM_Error_ToString(err));
        return 1;
    }

    ARSTREAM_SenderTB_AddFilters();

    pthread_t streamsend, streamread;
    pthread_create (&streamsend, NULL, ARSTREAM_Sender_RunDataThread, g_Sender);
    pthread_create (&streamread, NULL, ARSTREAM_Sender_RunAckThread, g_Sender);

    /* USER CODE */

    pthread_t sourceThread;
    pthread_create (&sourceThread, NULL, fakeEncoderThread, g_Sender);
    pthread_join (sourceThread, NULL);

    /* END OF USER CODE */

    ARSTREAM_Sender_StopSender (g_Sender);

    pthread_join (streamread, NULL);
    pthread_join (streamsend, NULL);

    ARSTREAM_Sender_Delete (&g_Sender);

    return retVal;
}

/*
 * Implementation
 */

int ARSTREAM_Sender_TestBenchMain (int argc, char *argv[])
{
    int retVal = 0;
    appName = argv[0];
    if (3 <=  argc)
    {
        ARSTREAM_SenderTb_printUsage ();
        return 1;
    }

    char *ip = __IP;

    if (2 == argc)
    {
        ip = argv[1];
    }

    int nbInBuff = 1;
    ARNETWORK_IOBufferParam_t inParams;
    ARSTREAM_Sender_InitStreamDataBuffer (&inParams, DATA_BUFFER_ID, ARSTREAM_TB_FRAG_SIZE, ARSTREAM_TB_MAX_NB_FRAG);
    int nbOutBuff = 1;
    ARNETWORK_IOBufferParam_t outParams;
    ARSTREAM_Sender_InitStreamAckBuffer (&outParams, ACK_BUFFER_ID);

    eARNETWORK_ERROR error;
    eARNETWORKAL_ERROR specificError = ARNETWORKAL_OK;
    ARNETWORKAL_Manager_t *osspecificManagerPtr = ARNETWORKAL_Manager_New(&specificError);

    if(specificError == ARNETWORKAL_OK)
    {
        specificError = ARNETWORKAL_Manager_InitWifiNetwork(osspecificManagerPtr, ip, SENDING_PORT, READING_PORT, 1000);
    }

    if(specificError == ARNETWORKAL_OK)
    {
        g_Manager = ARNETWORK_Manager_New(osspecificManagerPtr, nbInBuff, &inParams, nbOutBuff, &outParams, SENDER_PING_DELAY, NULL, NULL,  &error);
    }
    else
    {
        error = ARNETWORK_ERROR;
    }

    if ((g_Manager == NULL) ||
        (error != ARNETWORK_OK))
    {
        ARSAL_PRINT (ARSAL_PRINT_ERROR, __TAG__, "Error during ARNETWORK_Manager_New call : %d", error);
        return 1;
    }

    pthread_t netsend, netread;
    pthread_create (&netsend, NULL, ARNETWORK_Manager_SendingThreadRun, g_Manager);
    pthread_create (&netread, NULL, ARNETWORK_Manager_ReceivingThreadRun, g_Manager);

    stillRunning = 1;

    retVal = ARSTREAM_SenderTb_StartStreamTest (g_Manager);

    ARNETWORK_Manager_Stop (g_Manager);

    pthread_join (netread, NULL);
    pthread_join (netsend, NULL);

    ARNETWORK_Manager_Delete (&g_Manager);

    return retVal;
}

void ARSTREAM_Sender_TestBenchStop ()
{
    stillRunning = 0;
}

int ARSTREAM_SenderTb_GetMeanTimeBetweenFrames ()
{
    int retVal = 0;
    int i;
    for (i = 0; i < NB_FRAMES_FOR_AVERAGE; i++)
    {
        retVal += lastDt [i];
    }
    return retVal / NB_FRAMES_FOR_AVERAGE;
}


int ARSTREAM_SenderTb_GetLatency ()
{
    if (g_Manager == NULL)
    {
        return -1;
    }
    else
    {
        return ARNETWORK_Manager_GetEstimatedLatency(g_Manager);
    }
}

int ARSTREAM_SenderTb_GetMissedFrames ()
{
    int retval = nbSkippedSinceLast;
    nbSkippedSinceLast = 0;
    return retval;
}


float ARSTREAM_SenderTb_GetEfficiency ()
{
    if (g_Sender == NULL)
    {
        return 0.f;
    }
    return ARSTREAM_Sender_GetEstimatedEfficiency (g_Sender);
}

int ARSTREAM_SenderTb_GetEstimatedLoss ()
{
    if (g_Manager == NULL)
    {
        return 100;
    }
    return ARNETWORK_Manager_GetEstimatedMissPercentage(g_Manager, ACK_BUFFER_ID);
}
