/**
 * @file ARVIDEO_Sender_TestBench.c
 * @brief Testbench for the ARVIDEO_Sender submodule
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
#include <libARVideo/ARVIDEO_Sender.h>

/*
 * Macros
 */

#define ACK_BUFFER_ID (13)
#define DATA_BUFFER_ID (125)

#define SENDING_PORT (54321)
#define READING_PORT (43210)

#define BITRATE_KBPS (1200)
#define FPS          (30)

#define FRAME_SIZE   (1000 * BITRATE_KBPS / FPS / 8)

#define FRAME_MIN_SIZE (FRAME_SIZE)
#define FRAME_MAX_SIZE (FRAME_SIZE)

#define I_FRAME_EVERY_N (15)
#define NB_BUFFERS (2 * I_FRAME_EVERY_N)

#define TIME_BETWEEN_FRAMES_MS (1000 / FPS)

#define __TAG__ "ARVIDEO_Sender_TB"

#define SENDER_PING_DELAY (0) // Use default value

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

static ARVIDEO_Sender_t *g_Sender;
static ARNETWORK_Manager_t *g_Manager;

static int lastDt [NB_FRAMES_FOR_AVERAGE] = {0};
static int currentIndexInDt = 0;

static int nbSkippedSinceLast = 0;

static int stillRunning = 1;

float ARVIDEO_Sender_PercentOk = 0.0;
static int nbSent = 0;
static int nbOk = 0;

static int currentBufferIndex = 0;
static uint8_t *multiBuffer[NB_BUFFERS];
static uint32_t multiBufferSize[NB_BUFFERS];
static int multiBufferIsFree[NB_BUFFERS];

static char *appName;

/*
 * Internal functions declarations
 */

/**
 * @brief Print the parameters of the application
 */
void ARVIDEO_SenderTb_printUsage ();

/**
 * @brief Initializes the multi buffers of the testbench
 */
void ARVIDEO_SenderTb_initMultiBuffers ();

/**
 * @see ARVIDEO_Sender.h
 */
void ARVIDEO_SenderTb_FrameUpdateCallback (eARVIDEO_SENDER_STATUS status, uint8_t *framePointer, uint32_t frameSize, void *custom);

/**
 * @brief Gets a free buffer pointer
 * @param[in] buffer the buffer to mark as free
 */
void ARVIDEO_SenderTb_SetBufferFree (uint8_t *buffer);

/**
 * @brief Gets a free buffer pointer
 * @param[out] retSize Pointer where to store the size of the next free buffer (or 0 if there is no free buffer)
 * @return Pointer to the next free buffer, or NULL if there is no free buffer
 */
uint8_t* ARVIDEO_SenderTb_GetNextFreeBuffer (uint32_t *retSize);

/**
 * @brief Fake encoder thread function
 * This function generates dummy frames periodically, and sends them through the ARVIDEO_Sender_t
 * @param ARVIDEO_Sender_t_Param A valid ARVIDEO_Sender_t, casted as a (void *), which will be used by the thread
 * @return No meaningful value : (void *)0
 */
void* fakeEncoderThread (void *ARVIDEO_Sender_t_Param);

/**
 * @brief Video entry point
 * @param manager An initialized network manager
 * @return "Main" return value
 */
int ARVIDEO_SenderTb_StartVideoTest (ARNETWORK_Manager_t *manager);

/*
 * Internal functions implementation
 */

void ARVIDEO_SenderTb_printUsage ()
{
    ARSAL_PRINT (ARSAL_PRINT_ERROR, __TAG__, "Usage : %s [ip]", appName);
    ARSAL_PRINT (ARSAL_PRINT_ERROR, __TAG__, "        ip -> optionnal, ip of the video reader");
}

void ARVIDEO_SenderTb_initMultiBuffers ()
{
    int buffIndex;
    for (buffIndex = 0; buffIndex < NB_BUFFERS; buffIndex++)
    {
        multiBuffer[buffIndex] = malloc (FRAME_MAX_SIZE);
        multiBufferSize[buffIndex] = FRAME_MAX_SIZE;
        multiBufferIsFree[buffIndex] = 1;
    }
}

void ARVIDEO_SenderTb_FrameUpdateCallback (eARVIDEO_SENDER_STATUS status, uint8_t *framePointer, uint32_t frameSize, void *custom)
{
    static struct timeval prev = {0};
    struct timeval now = {0};
    int dt;
    custom = custom;
    switch (status)
    {
    case ARVIDEO_SENDER_STATUS_FRAME_SENT:
        ARVIDEO_SenderTb_SetBufferFree (framePointer);
        ARSAL_PRINT (ARSAL_PRINT_WARNING, __TAG__, "Successfully sent a frame of size %u", frameSize);
        gettimeofday (&now, NULL);
        dt = ARSAL_Time_ComputeMsTimeDiff(&prev, &now);
        prev.tv_sec = now.tv_sec;
        prev.tv_usec = now.tv_usec;
        lastDt[currentIndexInDt] = dt;
        currentIndexInDt ++;
        currentIndexInDt %= NB_FRAMES_FOR_AVERAGE;
        nbSent++;
        nbOk++;
        ARVIDEO_Sender_PercentOk = (100.f * nbOk) / (1.f * nbSent);
        break;
    case ARVIDEO_SENDER_STATUS_FRAME_CANCEL:
        ARVIDEO_SenderTb_SetBufferFree (framePointer);
        ARSAL_PRINT (ARSAL_PRINT_WARNING, __TAG__, "Cancelled a frame of size %u", frameSize);
        nbSent++;
        nbSkippedSinceLast++;
        ARVIDEO_Sender_PercentOk = (100.f * nbOk) / (1.f * nbSent);
        break;
    default:
        // All cases handled
        break;
    }
}

void ARVIDEO_SenderTb_SetBufferFree (uint8_t *buffer)
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

uint8_t* ARVIDEO_SenderTb_GetNextFreeBuffer (uint32_t *retSize)
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

void* fakeEncoderThread (void *ARVIDEO_Sender_t_Param)
{

    uint32_t cnt = 0;
    uint32_t frameSize = 0;
    uint32_t frameCapacity = 0;
    uint8_t *nextFrameAddr;
    ARVIDEO_Sender_t *sender = (ARVIDEO_Sender_t *)ARVIDEO_Sender_t_Param;
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

        nextFrameAddr = ARVIDEO_SenderTb_GetNextFreeBuffer (&frameCapacity);
        if (nextFrameAddr != NULL)
        {
            if (frameCapacity >= frameSize)
            {
                eARVIDEO_ERROR res;
                int nbPrevious;
                int flush = ((cnt % I_FRAME_EVERY_N) == 1) ? 1 : 0;
                memset (nextFrameAddr, cnt, frameSize);
                res = ARVIDEO_Sender_SendNewFrame (sender, nextFrameAddr, frameSize, flush, &nbPrevious);
                switch (res)
                {
                case ARVIDEO_OK:
                    ARSAL_PRINT (ARSAL_PRINT_WARNING, __TAG__, "Added a frame of size %u to the Sender (already %d in queue)", frameSize, nbPrevious);
                    break;
                case ARVIDEO_ERROR_BAD_PARAMETERS:
                case ARVIDEO_ERROR_FRAME_TOO_LARGE:
                case ARVIDEO_ERROR_QUEUE_FULL:
                    ARSAL_PRINT (ARSAL_PRINT_ERROR, __TAG__, "Unable to send the new frame : %s", ARVIDEO_Error_ToString(res));
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

int ARVIDEO_SenderTb_StartVideoTest (ARNETWORK_Manager_t *manager)
{
    int retVal = 0;
    eARVIDEO_ERROR err;
    ARVIDEO_SenderTb_initMultiBuffers ();
    g_Sender = ARVIDEO_Sender_New (manager, DATA_BUFFER_ID, ACK_BUFFER_ID, ARVIDEO_SenderTb_FrameUpdateCallback, NB_BUFFERS, NULL, &err);
    if (g_Sender == NULL)
    {
        ARSAL_PRINT (ARSAL_PRINT_ERROR, __TAG__, "Error during ARVIDEO_Sender_New call : %s", ARVIDEO_Error_ToString(err));
        return 1;
    }

    pthread_t videosend, videoread;
    pthread_create (&videosend, NULL, ARVIDEO_Sender_RunDataThread, g_Sender);
    pthread_create (&videoread, NULL, ARVIDEO_Sender_RunAckThread, g_Sender);

    /* USER CODE */

    pthread_t sourceThread;
    pthread_create (&sourceThread, NULL, fakeEncoderThread, g_Sender);
    pthread_join (sourceThread, NULL);

    /* END OF USER CODE */

    ARVIDEO_Sender_StopSender (g_Sender);

    pthread_join (videoread, NULL);
    pthread_join (videosend, NULL);

    ARVIDEO_Sender_Delete (&g_Sender);

    return retVal;
}

/*
 * Implementation
 */

int ARVIDEO_Sender_TestBenchMain (int argc, char *argv[])
{
    int retVal = 0;
    appName = argv[0];
    if (3 <=  argc)
    {
        ARVIDEO_SenderTb_printUsage ();
        return 1;
    }

    char *ip = __IP;

    if (2 == argc)
    {
        ip = argv[1];
    }

    int nbInBuff = 1;
    ARNETWORK_IOBufferParam_t inParams;
    ARVIDEO_Sender_InitVideoDataBuffer (&inParams, DATA_BUFFER_ID);
    int nbOutBuff = 1;
    ARNETWORK_IOBufferParam_t outParams;
    ARVIDEO_Sender_InitVideoAckBuffer (&outParams, ACK_BUFFER_ID);

    eARNETWORK_ERROR error;
    eARNETWORKAL_ERROR specificError = ARNETWORKAL_OK;
    ARNETWORKAL_Manager_t *osspecificManagerPtr = ARNETWORKAL_Manager_New(&specificError);

    if(specificError == ARNETWORKAL_OK)
    {
        specificError = ARNETWORKAL_Manager_InitWiFiNetwork(osspecificManagerPtr, ip, SENDING_PORT, READING_PORT, 1000);
    }

    if(specificError == ARNETWORKAL_OK)
    {
        g_Manager = ARNETWORK_Manager_New(osspecificManagerPtr, nbInBuff, &inParams, nbOutBuff, &outParams, SENDER_PING_DELAY, &error);
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

    retVal = ARVIDEO_SenderTb_StartVideoTest (g_Manager);

    ARNETWORK_Manager_Stop (g_Manager);

    pthread_join (netread, NULL);
    pthread_join (netsend, NULL);

    ARNETWORK_Manager_Delete (&g_Manager);

    return retVal;
}

void ARVIDEO_Sender_TestBenchStop ()
{
    stillRunning = 0;
}

int ARVIDEO_SenderTb_GetMeanTimeBetweenFrames ()
{
    int retVal = 0;
    int i;
    for (i = 0; i < NB_FRAMES_FOR_AVERAGE; i++)
    {
        retVal += lastDt [i];
    }
    return retVal / NB_FRAMES_FOR_AVERAGE;
}


int ARVIDEO_SenderTb_GetLatency ()
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

int ARVIDEO_SenderTb_GetMissedFrames ()
{
    int retval = nbSkippedSinceLast;
    nbSkippedSinceLast = 0;
    return retval;
}


float ARVIDEO_SenderTb_GetEfficiency ()
{
    if (g_Sender == NULL)
    {
        return 0.f;
    }
    return ARVIDEO_Sender_GetEstimatedEfficiency (g_Sender);
}
