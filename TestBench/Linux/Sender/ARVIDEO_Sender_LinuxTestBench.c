/**
 * @file ARVIDEO_Sender_LinuxTestBench.c
 * @brief Testbench for the ARVIDEO_Sender submodule
 * @date 03/25/2013
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
#include "../../Common/Sender/ARVIDEO_Sender_TestBench.h"
#include "../../Common/Logger/ARVIDEO_Logger.h"

/*
 * Macros
 */

#define __TAG__ "SENDER_LINUX_TB"
#define REPORT_DELAY_MS (500)

/*
 * Types
 */

typedef struct {
    int argc;
    char **argv;
} ARVIDEO_Sender_LinuxTb_Args_t;

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
    ARVIDEO_Sender_LinuxTb_Args_t *args = (ARVIDEO_Sender_LinuxTb_Args_t *)params;

    retVal = ARVIDEO_Sender_TestBenchMain (args->argc, args->argv);

    return (void *)retVal;
}

void* reportMain (void *params)
{
    /* Avoid unused warnings */
    params = params;

    ARVIDEO_Logger_t *logger = ARVIDEO_Logger_NewWithDefaultName ();
    ARVIDEO_Logger_Log (logger, "Latency (ms); PercentOK (%%); Missed frames; Mean time between frames (ms); Efficiency");
    ARSAL_PRINT (ARSAL_PRINT_DEBUG, __TAG__, "Latency (ms); PercentOK (%%); Missed frames; Mean time between frames (ms); Efficiency");
    while (1)
    {
        int lat = ARVIDEO_SenderTb_GetLatency ();
        int missed = ARVIDEO_SenderTb_GetMissedFrames ();
        int dt = ARVIDEO_SenderTb_GetMeanTimeBetweenFrames ();
        float eff = ARVIDEO_SenderTb_GetEfficiency ();
        ARSAL_PRINT (ARSAL_PRINT_DEBUG, __TAG__,"%4d; %5.2f; %3d; %4d; %5.3f", lat, ARVIDEO_Sender_PercentOk, missed, dt, eff);
        ARVIDEO_Logger_Log (logger, "%4d; %5.2f; %3d; %4d; %5.3f", lat, ARVIDEO_Sender_PercentOk, missed, dt, eff);
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

    ARVIDEO_Sender_LinuxTb_Args_t args = {argc, argv};
    int retVal;

    pthread_create (&tbThread, NULL, tbMain, &args);
    pthread_create (&reportThread, NULL, reportMain, NULL);

    pthread_join (reportThread, NULL);
    pthread_join (tbThread, (void **)&retVal);

    return retVal;
}
