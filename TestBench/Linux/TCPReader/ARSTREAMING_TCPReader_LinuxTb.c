/**
 * @file ARSTREAMING_TCPReader_LinuxTb.c
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
#include "../../Common/TCPReader/ARSTREAMING_TCPReader.h"
#include "../../Common/Logger/ARSTREAMING_Logger.h"

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
} ARSTREAMING_TCPReader_LinuxTb_Args_t;

/*
 * Globals
 */

/*
 * Internal functions declarations
 */

/**
 * @brief Thread entry point for the testbench
 * @param params An ARSTREAMING_Sender_LinuxTb_Args_t pointer, casted to void *
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
    ARSTREAMING_TCPReader_LinuxTb_Args_t *args = (ARSTREAMING_TCPReader_LinuxTb_Args_t *)params;

    retVal = ARSTREAMING_TCPReader_Main (args->argc, args->argv);

    return (void *)retVal;
}

void* reportMain (void *params)
{
    /* Avoid unused warnings */
    params = params;

    ARSTREAMING_Logger_t *logger = ARSTREAMING_Logger_NewWithDefaultName ();
    ARSTREAMING_Logger_Log (logger, "Mean time between frames (ms)");
    ARSAL_PRINT (ARSAL_PRINT_WARNING, __TAG__, "Mean time between frames (ms)");
    while (1)
    {
        int dt = ARSTREAMING_TCPReader_GetMeanTimeBetweenFrames ();
        ARSAL_PRINT (ARSAL_PRINT_WARNING, __TAG__, "%4d", dt);
        ARSTREAMING_Logger_Log (logger, "%4d", dt);
        usleep (1000 * REPORT_DELAY_MS);
    }
    ARSTREAMING_Logger_Delete (&logger);

    return (void *)0;
}

/*
 * Implementation
 */

int main (int argc, char *argv[])
{
    pthread_t tbThread;
    pthread_t reportThread;

    ARSTREAMING_TCPReader_LinuxTb_Args_t args = {argc, argv};
    int retVal;

    pthread_create (&tbThread, NULL, tbMain, &args);
    pthread_create (&reportThread, NULL, reportMain, NULL);

    pthread_join (reportThread, NULL);
    pthread_join (tbThread, (void **)&retVal);

    return retVal;
}
