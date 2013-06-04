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

#define ACK_BUFFER_ID (50)
#define DATA_BUFFER_ID (51)

#define SENDING_PORT (54321)
#define READING_PORT (12345)

#define FRAME_MIN_SIZE (5000)
#define FRAME_MAX_SIZE (5000)

#define NB_BUFFERS (15)
#define I_FRAME_EVERY_N (10)

#define TEST_MODE (0)

#if TEST_MODE
# define TIME_BETWEEN_FRAMES_MS (1000)
#else
# define TIME_BETWEEN_FRAMES_MS (33)
#endif

#define __TAG__ "ARVIDEO_Sender_TB"

#ifndef __IP
#define __IP "127.0.0.1"
#endif

/*
 * Types
 */

/*
 * Globals
 */

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
void printUsage ();

/**
 * @brief Initializes the multi buffers of the testbench
 */
void initMultiBuffers ();

/**
 * @see ARVIDEO_Sender.h
 */
void ARVIDEO_SenderTb_FrameUpdateCallback (eARVIDEO_SENDER_STATUS status, uint8_t *framePointer, uint32_t frameSize);

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

void printUsage ()
{
    ARSAL_PRINT (ARSAL_PRINT_ERROR, __TAG__, "Usage : %s [ip]\n", appName);
    ARSAL_PRINT (ARSAL_PRINT_ERROR, __TAG__, "        ip -> optionnal, ip of the video reader\n");
}

void initMultiBuffers ()
{
    int buffIndex;
    for (buffIndex = 0; buffIndex < NB_BUFFERS; buffIndex++)
    {
        multiBuffer[buffIndex] = malloc (FRAME_MAX_SIZE);
        multiBufferSize[buffIndex] = FRAME_MAX_SIZE;
        multiBufferIsFree[buffIndex] = 1;
    }
}

void ARVIDEO_SenderTb_FrameUpdateCallback (eARVIDEO_SENDER_STATUS status, uint8_t *framePointer, uint32_t frameSize)
{
    switch (status)
    {
    case ARVIDEO_SENDER_STATUS_FRAME_SENT:
        ARVIDEO_SenderTb_SetBufferFree (framePointer);
        ARSAL_PRINT (ARSAL_PRINT_WARNING, __TAG__, "Successfully sent a frame of size %u\n", frameSize);
        break;
    case ARVIDEO_SENDER_STATUS_FRAME_CANCEL:
        ARVIDEO_SenderTb_SetBufferFree (framePointer);
        ARSAL_PRINT (ARSAL_PRINT_WARNING, __TAG__, "Cancelled a frame of size %u\n", frameSize);
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
    ARSAL_PRINT (ARSAL_PRINT_WARNING, __TAG__, "Encoder thread running\n");
    while (1)
    {
        cnt++;
        frameSize = rand () % FRAME_MAX_SIZE;
        if (frameSize < FRAME_MIN_SIZE)
        {
            frameSize = FRAME_MIN_SIZE;
        }
        ARSAL_PRINT (ARSAL_PRINT_WARNING, __TAG__, "Generating a frame of size %d with number %u\n", frameSize, cnt);

        nextFrameAddr = ARVIDEO_SenderTb_GetNextFreeBuffer (&frameCapacity);
        if (nextFrameAddr != NULL)
        {
            if (frameCapacity >= frameSize)
            {
                int prevRes;
                int flush = ((cnt % I_FRAME_EVERY_N) == 1) ? 1 : 0;
                memset (nextFrameAddr, cnt, frameSize);
                prevRes = ARVIDEO_Sender_SendNewFrame (sender, nextFrameAddr, frameSize, flush);
                switch (prevRes)
                {
                case -1:
                    ARSAL_PRINT (ARSAL_PRINT_ERROR, __TAG__, "Unable to send the new frame\n");
                    break;
                case 0:
                    ARSAL_PRINT (ARSAL_PRINT_WARNING, __TAG__, "Try to send a frame of size %u\n", frameSize);
                    break;
                case 1:
                    ARSAL_PRINT (ARSAL_PRINT_WARNING, __TAG__, "Try to send a frame of size %u, will cancel previous frame sending\n", frameSize);
                    break;
                }
            }
            else
            {
                ARSAL_PRINT (ARSAL_PRINT_ERROR, __TAG__, "Could not encode a new frame : buffer too small !\n");
            }
        }
        else
        {
            ARSAL_PRINT (ARSAL_PRINT_ERROR, __TAG__, "Could not encode a new frame : no free buffer !\n");
        }

        usleep (1000 * TIME_BETWEEN_FRAMES_MS);
    }
    ARSAL_PRINT (ARSAL_PRINT_WARNING, __TAG__, "Encoder thread ended\n");
    return (void *)0;
}

int ARVIDEO_SenderTb_StartVideoTest (ARNETWORK_Manager_t *manager)
{
    int retVal;
    ARVIDEO_Sender_t *sender;
    initMultiBuffers ();
    sender = ARVIDEO_Sender_New (manager, DATA_BUFFER_ID, ACK_BUFFER_ID, ARVIDEO_SenderTb_FrameUpdateCallback, NB_BUFFERS);
    if (sender == NULL)
    {
        ARSAL_PRINT (ARSAL_PRINT_ERROR, __TAG__, "Error during ARVIDEO_Sender_New call\n");
        return 1;
    }

    pthread_t videosend, videoread;
    pthread_create (&videosend, NULL, ARVIDEO_Sender_RunDataThread, sender);
    pthread_create (&videoread, NULL, ARVIDEO_Sender_RunAckThread, sender);

    /* USER CODE */

    pthread_t sourceThread;
    pthread_create (&sourceThread, NULL, fakeEncoderThread, sender);
    pthread_join (sourceThread, NULL);

    /* END OF USER CODE */

    ARVIDEO_Sender_StopSender (sender);

    pthread_join (videoread, NULL);
    pthread_join (videosend, NULL);

    ARVIDEO_Sender_Delete (&sender);

    return retVal;
}

/*
 * Implementation
 */

int main (int argc, char *argv[])
{
    int retVal = 0;
    appName = argv[0];
    if (3 <=  argc)
    {
        printUsage ();
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
    ARNETWORK_Manager_t *manager = NULL;
    eARNETWORKAL_ERROR specificError = ARNETWORKAL_OK;
    ARNETWORKAL_Manager_t *osspecificManagerPtr = ARNETWORKAL_Manager_New(&specificError);

    if(specificError == ARNETWORKAL_OK)
    {
		specificError = ARNETWORKAL_Manager_InitWiFiNetwork(osspecificManagerPtr, ip, SENDING_PORT, READING_PORT, 1000);
	}

	if(specificError == ARNETWORKAL_OK)
	{
		manager = ARNETWORK_Manager_New(osspecificManagerPtr, nbInBuff, &inParams, nbOutBuff, &outParams, &error);
	}
	else
	{
		error = ARNETWORK_ERROR;
	}

    if ((manager == NULL) ||
        (error != ARNETWORK_OK))
    {
        ARSAL_PRINT (ARSAL_PRINT_ERROR, __TAG__, "Error during ARNETWORK_Manager_New call : %d\n", error);
        return 1;
    }

    pthread_t netsend, netread;
    pthread_create (&netsend, NULL, ARNETWORK_Manager_SendingThreadRun, manager);
    pthread_create (&netread, NULL, ARNETWORK_Manager_ReceivingThreadRun, manager);

    retVal = ARVIDEO_SenderTb_StartVideoTest (manager);

    ARNETWORK_Manager_Stop (manager);

    pthread_join (netread, NULL);
    pthread_join (netsend, NULL);

    ARNETWORK_Manager_Delete (&manager);

    return retVal;
}
