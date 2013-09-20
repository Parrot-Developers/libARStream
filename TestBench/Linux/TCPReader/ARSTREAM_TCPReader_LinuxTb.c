/**
 * @file ARSTREAM_TCPReader_LinuxTb.c
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
#include "../../Common/TCPReader/ARSTREAM_TCPReader.h"
#include "../../Common/Logger/ARSTREAM_Logger.h"

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
} ARSTREAM_TCPReader_LinuxTb_Args_t;

/*
 * Globals
 */

/*
 * Internal functions declarations
 */

/**
 * @brief Thread entry point for the testbench
 * @param params An ARSTREAM_Sender_LinuxTb_Args_t pointer, casted to void *
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
    ARSTREAM_TCPReader_LinuxTb_Args_t *args = (ARSTREAM_TCPReader_LinuxTb_Args_t *)params;

    retVal = ARSTREAM_TCPReader_Main (args->argc, args->argv);

    return (void *)retVal;
}

void* reportMain (void *params)
{
    /* Avoid unused warnings */
    params = params;

    ARSTREAM_Logger_t *logger = ARSTREAM_Logger_NewWithDefaultName ();
    ARSTREAM_Logger_Log (logger, "Mean time between frames (ms)");
    ARSAL_PRINT (ARSAL_PRINT_WARNING, __TAG__, "Mean time between frames (ms)");
    while (1)
    {
        int dt = ARSTREAM_TCPReader_GetMeanTimeBetweenFrames ();
        ARSAL_PRINT (ARSAL_PRINT_WARNING, __TAG__, "%4d", dt);
        ARSTREAM_Logger_Log (logger, "%4d", dt);
        usleep (1000 * REPORT_DELAY_MS);
    }
    ARSTREAM_Logger_Delete (&logger);

    return (void *)0;
}

/*
 * Implementation
 */

int main (int argc, char *argv[])
{
    pthread_t tbThread;
    pthread_t reportThread;

    ARSTREAM_TCPReader_LinuxTb_Args_t args = {argc, argv};
    int retVal;

    pthread_create (&tbThread, NULL, tbMain, &args);
    pthread_create (&reportThread, NULL, reportMain, NULL);

    pthread_join (reportThread, NULL);
    pthread_join (tbThread, (void **)&retVal);

    return retVal;
}
