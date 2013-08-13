/**
 * @file ARVIDEO_Reader_TestBench.c
 * @brief Testbench for the ARVIDEO_Reader submodule
 * @date 03/25/2013
 * @author nicolas.brulez@parrot.com
 */

/*
 * System Headers
 */

#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>

/*
 * ARSDK Headers
 */

#include <libARSAL/ARSAL_Print.h>
#include <libARSAL/ARSAL_Sem.h>
#include <libARVideo/ARVIDEO_Reader.h>

/*
 * Macros
 */

#define ACK_BUFFER_ID (13)
#define DATA_BUFFER_ID (125)

#define SENDING_PORT (43210)
#define READING_PORT (54321)

#define FRAME_MIN_SIZE (2000)
#define FRAME_MAX_SIZE (40000)

#define NB_BUFFERS (3)

#define __TAG__ "ARVIDEO_Reader_TB"

#define READER_PING_DELAY (0) // Use default value

#ifndef __IP
#define __IP "127.0.0.1"
#endif

#define NB_FRAMES_FOR_AVERAGE (15)

/*
 * Types
 */

/*
 * Globals
 */


static struct timeval lastRecv = {0};
static int lastDt [NB_FRAMES_FOR_AVERAGE] = {0};
static int currentIndexInDt = 0;

float ARVIDEO_Reader_PercentOk = 100.f;
static int nbRead = 0;
static int nbSkipped = 0;
static int nbSkippedSinceLast = 0;

static int running;
ARSAL_Sem_t closeSem;
static ARNETWORK_Manager_t *g_Manager = NULL;
static ARVIDEO_Reader_t *g_Reader = NULL;

static int currentBufferIndex = 0;
static uint8_t *multiBuffer[NB_BUFFERS];
static uint32_t multiBufferSize[NB_BUFFERS];
static int multiBufferIsFree[NB_BUFFERS];

static char *appName;

static FILE *outFile;

/*
 * Internal functions declarations
 */

/**
 * @brief Print the parameters of the application
 */
void ARVIDEO_ReaderTb_printUsage ();

/**
 * @brief Initializes the multi buffers of the testbench
 * @param initialSize Initial size of the buffers
 */
void ARVIDEO_ReaderTb_initMultiBuffers (int initialSize);

/**
 * @brief Realloc a buffer to a new size
 * @param id The index of the buffer to reallocate
 * @param newSize The new size of the buffer
 */
void reallocBuffer (int id, int newSize);

/**
 * @see ARVIDEO_Reader.h
 */
uint8_t* ARVIDEO_ReaderTb_FrameCompleteCallback (eARVIDEO_READER_CAUSE cause, uint8_t *framePointer, uint32_t frameSize, int numberOfSkippedFrames, int isFlushFrame, uint32_t *newBufferCapacity, void *custom);

/**
 * @brief Gets a free buffer pointer
 * @param[in] buffer the buffer to mark as free
 */
void ARVIDEO_ReaderTb_SetBufferFree (uint8_t *buffer);

/**
 * @brief Gets a free buffer pointer
 * @param[out] retSize Pointer where to store the size of the next free buffer (or 0 if there is no free buffer)
 * @param[in] reallocToDouble Set to non-zero to realloc the buffer to the double of its previous size
 * @return Pointer to the next free buffer, or NULL if there is no free buffer
 */
uint8_t* ARVIDEO_ReaderTb_GetNextFreeBuffer (uint32_t *retSize, int reallocToDouble);

/**
 * @brief Video entry point
 * @param manager An initialized network manager
 * @return "Main" return value
 */
int ARVIDEO_ReaderTb_StartVideoTest (ARNETWORK_Manager_t *manager, const char *outPath);

/*
 * Internal functions implementation
 */

void ARVIDEO_ReaderTb_printUsage ()
{
    ARSAL_PRINT (ARSAL_PRINT_ERROR, __TAG__, "Usage : %s [ip] [outFile]", appName);
    ARSAL_PRINT (ARSAL_PRINT_ERROR, __TAG__, "        ip -> optionnal, ip of the video reader");
    ARSAL_PRINT (ARSAL_PRINT_ERROR, __TAG__, "        outFile -> optionnal (ip must be provided), output file for received video stream");
}

void ARVIDEO_ReaderTb_initMultiBuffers (int initialSize)
{
    int buffIndex;
    for (buffIndex = 0; buffIndex < NB_BUFFERS; buffIndex++)
    {
        reallocBuffer (buffIndex, initialSize);
        multiBufferIsFree[buffIndex] = 1;
    }
}

void reallocBuffer (int id, int newSize)
{
    multiBuffer [id] = realloc (multiBuffer [id], newSize);
    multiBufferSize [id] = newSize;
}

uint8_t* ARVIDEO_ReaderTb_FrameCompleteCallback (eARVIDEO_READER_CAUSE cause, uint8_t *framePointer, uint32_t frameSize, int numberOfSkippedFrames, int isFlushFrame, uint32_t *newBufferCapacity, void *buffer)
{
    uint8_t *retVal = NULL;
    struct timeval now;
    int dt;
    buffer = buffer;
    switch (cause)
    {
    case ARVIDEO_READER_CAUSE_FRAME_COMPLETE:
        ARSAL_PRINT (ARSAL_PRINT_WARNING, __TAG__, "Got a complete frame of size %d, at address %p (isFlush : %d)", frameSize, framePointer, isFlushFrame);
        if (isFlushFrame != 0)
        nbRead++;
        if (numberOfSkippedFrames != 0)
        {
            ARSAL_PRINT (ARSAL_PRINT_WARNING, __TAG__, "Skipped %d frames", numberOfSkippedFrames);
            if (numberOfSkippedFrames > 0)
            {
                nbSkipped += numberOfSkippedFrames;
                nbSkippedSinceLast += numberOfSkippedFrames;
            }
        }
        ARVIDEO_Reader_PercentOk = (100.f * nbRead) / (1.f * (nbRead + nbSkipped));
        if (outFile != NULL)
        {
            fwrite (framePointer, 1, frameSize, outFile);
        }
        gettimeofday(&now, NULL);
        dt = ARSAL_Time_ComputeMsTimeDiff(&lastRecv, &now);
        lastDt [currentIndexInDt] = dt;
        currentIndexInDt ++;
        currentIndexInDt %= NB_FRAMES_FOR_AVERAGE;
        lastRecv.tv_sec = now.tv_sec;
        lastRecv.tv_usec = now.tv_usec;
        ARVIDEO_ReaderTb_SetBufferFree (framePointer);
        retVal = ARVIDEO_ReaderTb_GetNextFreeBuffer (newBufferCapacity, 0);
        break;

    case ARVIDEO_READER_CAUSE_FRAME_TOO_SMALL:
        ARSAL_PRINT (ARSAL_PRINT_WARNING, __TAG__, "Current buffer is to small for frame !");
        retVal = ARVIDEO_ReaderTb_GetNextFreeBuffer (newBufferCapacity, 1);
        break;

    case ARVIDEO_READER_CAUSE_COPY_COMPLETE:
        ARSAL_PRINT (ARSAL_PRINT_WARNING, __TAG__, "Copy complete in new buffer, freeing this one");
        ARVIDEO_ReaderTb_SetBufferFree (framePointer);
        break;

    case ARVIDEO_READER_CAUSE_CANCEL:
        ARSAL_PRINT (ARSAL_PRINT_WARNING, __TAG__, "Reader is closing");
        ARVIDEO_ReaderTb_SetBufferFree (framePointer);
        break;

    default:
        ARSAL_PRINT (ARSAL_PRINT_ERROR, __TAG__, "Unknown cause (probably a bug !)");
        break;
    }
    return retVal;
}

void ARVIDEO_ReaderTb_SetBufferFree (uint8_t *buffer)
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

uint8_t* ARVIDEO_ReaderTb_GetNextFreeBuffer (uint32_t *retSize, int reallocToDouble)
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
            if (reallocToDouble != 0)
            {
                reallocBuffer (currentBufferIndex, 2* multiBufferSize [currentBufferIndex]);
            }
            retBuffer = multiBuffer[currentBufferIndex];
            *retSize = multiBufferSize[currentBufferIndex];
        }
        currentBufferIndex = (currentBufferIndex + 1) % NB_BUFFERS;
        nbtest++;
    } while (retBuffer == NULL && nbtest < NB_BUFFERS);
    return retBuffer;
}

int ARVIDEO_ReaderTb_StartVideoTest (ARNETWORK_Manager_t *manager, const char *outPath)
{
    int retVal = 0;

    uint8_t *firstFrame;
    uint32_t firstFrameSize;
    eARVIDEO_ERROR err;
    if (NULL != outPath)
    {
        outFile = fopen (outPath, "wb");
    }
    else
    {
        outFile = NULL;
    }
    ARVIDEO_ReaderTb_initMultiBuffers (FRAME_MAX_SIZE);
    ARSAL_Sem_Init (&closeSem, 0, 0);
    firstFrame = ARVIDEO_ReaderTb_GetNextFreeBuffer (&firstFrameSize, 0);
    g_Reader = ARVIDEO_Reader_New (manager, DATA_BUFFER_ID, ACK_BUFFER_ID, ARVIDEO_ReaderTb_FrameCompleteCallback, firstFrame, firstFrameSize, NULL, &err);
    if (g_Reader == NULL)
    {
        ARSAL_PRINT (ARSAL_PRINT_ERROR, __TAG__, "Error during ARVIDEO_Reader_New call : %s", ARVIDEO_Error_ToString(err));
        return 1;
    }

    pthread_t videosend, videoread;
    pthread_create (&videosend, NULL, ARVIDEO_Reader_RunDataThread, g_Reader);
    pthread_create (&videoread, NULL, ARVIDEO_Reader_RunAckThread, g_Reader);

    /* USER CODE */

    running = 1;
    ARSAL_Sem_Wait (&closeSem);
    running = 0;

    /* END OF USER CODE */

    ARVIDEO_Reader_StopReader (g_Reader);

    pthread_join (videoread, NULL);
    pthread_join (videosend, NULL);

    ARVIDEO_Reader_Delete (&g_Reader);

    ARSAL_Sem_Destroy (&closeSem);

    return retVal;
}

/*
 * Implementation
 */

int ARVIDEO_Reader_TestBenchMain (int argc, char *argv[])
{
    int retVal = 0;
    appName = argv[0];
    if (argc > 3)
    {
        ARVIDEO_ReaderTb_printUsage ();
        return 1;
    }

    char *ip = __IP;
    char *outPath = NULL;

    if (argc >= 2)
    {
        ip = argv[1];
    }
    if (argc >= 3)
    {
        outPath = argv[2];
    }

    ARSAL_PRINT (ARSAL_PRINT_WARNING, __TAG__, "IP = %s", ip);

    int nbInBuff = 1;
    ARNETWORK_IOBufferParam_t inParams;
    ARVIDEO_Reader_InitVideoAckBuffer (&inParams, ACK_BUFFER_ID);
    int nbOutBuff = 1;
    ARNETWORK_IOBufferParam_t outParams;
    ARVIDEO_Reader_InitVideoDataBuffer (&outParams, DATA_BUFFER_ID);

    eARNETWORK_ERROR error;
    eARNETWORKAL_ERROR specificError = ARNETWORKAL_OK;
    ARNETWORKAL_Manager_t *osspecificManagerPtr = ARNETWORKAL_Manager_New(&specificError);

    if(specificError == ARNETWORKAL_OK)
    {
        specificError = ARNETWORKAL_Manager_InitWifiNetwork(osspecificManagerPtr, ip, SENDING_PORT, READING_PORT, 1000);
    }
    else
    {
        ARSAL_PRINT (ARSAL_PRINT_ERROR, __TAG__, "Error during ARNETWORKAL_Manager_New call : %s", ARNETWORKAL_Error_ToString(specificError));
    }

    if(specificError == ARNETWORKAL_OK)
    {
        g_Manager = ARNETWORK_Manager_New(osspecificManagerPtr, nbInBuff, &inParams, nbOutBuff, &outParams, READER_PING_DELAY, &error);
    }
    else
    {
        ARSAL_PRINT (ARSAL_PRINT_ERROR, __TAG__, "Error during ARNETWORKAL_Manager_InitWifiNetwork call : %s", ARNETWORKAL_Error_ToString(specificError));
        error = ARNETWORK_ERROR;
    }

    if ((g_Manager == NULL) ||
        (error != ARNETWORK_OK))
    {
        ARSAL_PRINT (ARSAL_PRINT_ERROR, __TAG__, "Error during ARNETWORK_Manager_New call : %s", ARNETWORK_Error_ToString(error));
        return 1;
    }

    pthread_t netsend, netread;
    pthread_create (&netsend, NULL, ARNETWORK_Manager_SendingThreadRun, g_Manager);
    pthread_create (&netread, NULL, ARNETWORK_Manager_ReceivingThreadRun, g_Manager);

    retVal = ARVIDEO_ReaderTb_StartVideoTest (g_Manager, outPath);

    ARNETWORK_Manager_Stop (g_Manager);

    pthread_join (netread, NULL);
    pthread_join (netsend, NULL);

    ARNETWORK_Manager_Delete (&g_Manager);
    ARNETWORKAL_Manager_CloseWifiNetwork(osspecificManagerPtr);
    ARNETWORKAL_Manager_Delete(&osspecificManagerPtr);

    return retVal;
}

void ARVIDEO_Reader_TestBenchStop ()
{
    if (running)
    {
        ARSAL_Sem_Post (&closeSem);
    }
}

int ARVIDEO_ReaderTb_GetMeanTimeBetweenFrames ()
{
    int retVal = 0;
    int i;
    for (i = 0; i < NB_FRAMES_FOR_AVERAGE; i++)
    {
        retVal += lastDt [i];
    }
    return retVal / NB_FRAMES_FOR_AVERAGE;
}


int ARVIDEO_ReaderTb_GetLatency ()
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

int ARVIDEO_ReaderTb_GetMissedFrames ()
{
    int retval = nbSkippedSinceLast;
    nbSkippedSinceLast = 0;
    return retval;
}

float ARVIDEO_ReaderTb_GetEfficiency ()
{
    if (g_Reader == NULL)
    {
        return 0.f;
    }
    return ARVIDEO_Reader_GetEstimatedEfficiency(g_Reader);
}

int ARVIDEO_ReaderTb_GetEstimatedLoss ()
{
    if (g_Manager == NULL)
    {
        return 100;
    }
    return ARNETWORK_Manager_GetEstimatedMissPercentage(g_Manager, DATA_BUFFER_ID);
}
