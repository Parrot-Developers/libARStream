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

/*
 * ARSDK Headers
 */

#include <libARSAL/ARSAL_Print.h>
#include <libARSAL/ARSAL_Sem.h>
#include <libARVideo/ARVIDEO_Reader.h>

/*
 * Macros
 */

#define ACK_BUFFER_ID (50)
#define DATA_BUFFER_ID (51)

#define SENDING_PORT (12345)
#define READING_PORT (54321)

#define NET_BUFFER_SIZE (1500)

#define FRAME_MIN_SIZE (2000)
#define FRAME_MAX_SIZE (20000)

#define NB_BUFFERS (3)

#define __TAG__ "ARVIDEO_Reader_TB"

#ifndef __IP
#define __IP "127.0.0.1"
#endif

/*
 * Types
 */

/*
 * Globals
 */

ARSAL_Sem_t closeSem;

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
 * @see ARVIDEO_Reader.h
 */
uint8_t* ARVIDEO_ReaderTb_FrameCompleteCallback (eARVIDEO_READER_CAUSE cause, uint8_t *framePointer, uint32_t frameSize, int numberOfSkippedFrames, uint32_t *newBufferCapacity);

/**
 * @brief Gets a free buffer pointer
 * @param[in] buffer the buffer to mark as free
 */
void ARVIDEO_ReaderTb_SetBufferFree (uint8_t *buffer);

/**
 * @brief Gets a free buffer pointer
 * @param[out] retSize Pointer where to store the size of the next free buffer (or 0 if there is no free buffer)
 * @return Pointer to the next free buffer, or NULL if there is no free buffer
 */
uint8_t* ARVIDEO_ReaderTb_GetNextFreeBuffer (uint32_t *retSize);

/**
 * @brief Video entry point
 * @param manager An initialized network manager
 * @return "Main" return value
 */
int ARVIDEO_ReaderTb_StartVideoTest (ARNETWORK_Manager_t *manager);

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

uint8_t* ARVIDEO_ReaderTb_FrameCompleteCallback (eARVIDEO_READER_CAUSE cause, uint8_t *framePointer, uint32_t frameSize, int numberOfSkippedFrames, uint32_t *newBufferCapacity)
{
    uint8_t *retVal = NULL;
    switch (cause)
    {
    case ARVIDEO_READER_CAUSE_FRAME_COMPLETE:
        ARSAL_PRINT (ARSAL_PRINT_WARNING, __TAG__, "Got a complete frame of size %d, at address %p", frameSize, framePointer);
        if (numberOfSkippedFrames != 0)
        {
            ARSAL_PRINT (ARSAL_PRINT_WARNING, __TAG__, "Skipped %d frames", numberOfSkippedFrames);
        }
        ARVIDEO_ReaderTb_SetBufferFree (framePointer);
        retVal = ARVIDEO_ReaderTb_GetNextFreeBuffer (newBufferCapacity);
        break;

    case ARVIDEO_READER_CAUSE_FRAME_TOO_SMALL:
        ARSAL_PRINT (ARSAL_PRINT_WARNING, __TAG__, "Current buffer is to small for frame !");
        retVal = ARVIDEO_ReaderTb_GetNextFreeBuffer (newBufferCapacity);
        break;

    case ARVIDEO_READER_CAUSE_COPY_COMPLETE:
        ARSAL_PRINT (ARSAL_PRINT_WARNING, __TAG__, "Copy complete in new buffer, freeing this one");
        ARVIDEO_ReaderTb_SetBufferFree (framePointer);
        break;

    case ARVIDEO_READER_CAUSE_CANCEL:
        ARSAL_PRINT (ARSAL_PRINT_WARNING, __TAG__, "Reader is closing");
        ARVIDEO_ReaderTb_SetBufferFree (framePointer);
        ARSAL_Sem_Post (&closeSem);
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

uint8_t* ARVIDEO_ReaderTb_GetNextFreeBuffer (uint32_t *retSize)
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

int ARVIDEO_ReaderTb_StartVideoTest (ARNETWORK_Manager_t *manager)
{
    int retVal;
    ARVIDEO_Reader_t *reader;
    uint8_t *firstFrame;
    uint32_t firstFrameSize;
    initMultiBuffers ();
    ARSAL_Sem_Init (&closeSem, 0, 0);
    firstFrame = ARVIDEO_ReaderTb_GetNextFreeBuffer (&firstFrameSize);
    reader = ARVIDEO_Reader_New (manager, DATA_BUFFER_ID, ACK_BUFFER_ID, ARVIDEO_ReaderTb_FrameCompleteCallback, firstFrame, firstFrameSize);
    if (reader == NULL)
    {
        ARSAL_PRINT (ARSAL_PRINT_ERROR, __TAG__, "Error during ARVIDEO_Reader_New call\n");
        return 1;
    }

    pthread_t videosend, videoread;
    pthread_create (&videosend, NULL, ARVIDEO_Reader_RunDataThread, reader);
    pthread_create (&videoread, NULL, ARVIDEO_Reader_RunAckThread, reader);

    /* USER CODE */

    ARSAL_Sem_Wait (&closeSem);

    /* END OF USER CODE */

    ARVIDEO_Reader_StopReader (reader);

    pthread_join (videoread, NULL);
    pthread_join (videosend, NULL);

    ARVIDEO_Reader_Delete (&reader);

    ARSAL_Sem_Destroy (&closeSem);

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
    ARVIDEO_Reader_InitVideoAckBuffer (&inParams, ACK_BUFFER_ID);
    int nbOutBuff = 1;
    ARNETWORK_IOBufferParam_t outParams;
    ARVIDEO_Reader_InitVideoDataBuffer (&outParams, DATA_BUFFER_ID);

    eARNETWORK_ERROR error;
    ARNETWORK_Manager_t *manager = ARNETWORK_Manager_New (NET_BUFFER_SIZE, NET_BUFFER_SIZE, nbInBuff, &inParams, nbOutBuff, &outParams, &error);
    if ((manager == NULL) ||
        (error != ARNETWORK_OK))
    {
        ARSAL_PRINT (ARSAL_PRINT_ERROR, __TAG__, "Error during ARNETWORK_Manager_New call : %d\n", error);
        return 1;
    }

    error = ARNETWORK_Manager_SocketsInit (manager, ip, SENDING_PORT, READING_PORT, 1000);
    if (error != ARNETWORK_OK)
    {
        ARSAL_PRINT (ARSAL_PRINT_ERROR, __TAG__, "Error during ARNETWORK_Manager_SocketsInit call : %d\n", error);
        return 1;
    }

    pthread_t netsend, netread;
    pthread_create (&netsend, NULL, ARNETWORK_Manager_SendingThreadRun, manager);
    pthread_create (&netread, NULL, ARNETWORK_Manager_ReceivingThreadRun, manager);

    retVal = ARVIDEO_ReaderTb_StartVideoTest (manager);

    ARNETWORK_Manager_Stop (manager);

    pthread_join (netread, NULL);
    pthread_join (netsend, NULL);

    ARNETWORK_Manager_Delete (&manager);

    return retVal;
}
