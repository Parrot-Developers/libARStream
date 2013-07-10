/**
 * @file ARVIDEO_TCPReader_LinuxTb.c
 * @brief Testbench for the Drone2-like reader submodule
 * @date 07/10/2013
 * @author nicolas.brulez@parrot.com
 */

/*
 * System Headers
 */

#include <pthread.h>

/*
 * ARSDK Headers
 */

#include <libARSAL/ARSAL_Print.h>
#include "../../Common/TCPReader/ARVIDEO_TCPReader.h"
#include "../../Common/Logger/ARVIDEO_Logger.h"

/*
 * Macros
 */

#define __TAG__ "TCPREADER_LINUX_TB"
#define REPORT_DELAY_MS (500)

/*
 * Types
 */

typedef struct {
    int argc;
    char **argv;
} ARVIDEO_TCPReader_LinuxTb_Args_t;

/*
 * Globals
 */

/*
 * Internal functions declarations
 */

/**
 * @brief Thread entry point for the testbench
 * @param params An ARVIDEO_Sender_LinuxTb_Args_t pointer, casted to void *
 * @return the main error code, casted as a void *
 */
void* tbMain (void *params);

/**
 * @brief Thread entry point for the report thread
 * @param params Unused (put NULL)
 * @return Unused ((void *)0)
 */
void *reportMain (void *params);

/*
 * Internal functions implementation
 */

void* tbMain (void *params)
{
    int retVal = 0;
    ARVIDEO_TCPReader_LinuxTb_Args_t *args = (ARVIDEO_TCPReader_LinuxTb_Args_t *)params;

    retVal = ARVIDEO_TCPReader_Main (args->argc, args->argv);

    return (void *)retVal;
}

void* reportMain (void *params)
{
    /* Avoid unused warnings */
    params = params;

    ARVIDEO_Logger_t *logger = ARVIDEO_Logger_NewWithDefaultName ();
    ARVIDEO_Logger_Log (logger, "Mean time between frames (ms)");
    ARSAL_PRINT (ARSAL_PRINT_WARNING, __TAG__, "Mean time between frames (ms)");
    while (1)
    {
        int dt = ARVIDEO_TCPReader_GetMeanTimeBetweenFrames ();
        ARSAL_PRINT (ARSAL_PRINT_WARNING, __TAG__, "%4d", dt);
        ARVIDEO_Logger_Log (logger, "%4d", dt);
        usleep (1000 * REPORT_DELAY_MS);
    }
    ARVIDEO_Logger_Delete (&logger);

    return (void *)0;
}

/*
 * Implementation
 */

int main (int argc, char *argv[])
{
    pthread_t tbThread;
    pthread_t reportThread;

    ARVIDEO_TCPReader_LinuxTb_Args_t args = {argc, argv};
    int retVal;

    pthread_create (&tbThread, NULL, tbMain, &args);
    pthread_create (&reportThread, NULL, reportMain, NULL);

    pthread_join (reportThread, NULL);
    pthread_join (tbThread, (void **)&retVal);

    return retVal;
}
