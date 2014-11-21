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
 * @file ARSTREAM_Reader_LinuxTestBench.c
 * @brief Testbench for the ARSTREAM_Reader submodule
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
#include "../../Common/Reader/ARSTREAM_Reader_TestBench.h"
#include "../../Common/Logger/ARSTREAM_Logger.h"

/*
 * Macros
 */

#define __TAG__ "READER_LINUX_TB"
#define REPORT_DELAY_MS (500)

/*
 * Types
 */

typedef struct {
    int argc;
    char **argv;
} ARSTREAM_Reader_LinuxTb_Args_t;

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
    ARSTREAM_Reader_LinuxTb_Args_t *args = (ARSTREAM_Reader_LinuxTb_Args_t *)params;

    retVal = ARSTREAM_Reader_TestBenchMain (args->argc, args->argv);

    return (void *)(intptr_t)retVal;
}

void* reportMain (void *params)
{
    /* Avoid unused warnings */
    params = params;

    ARSTREAM_Logger_t *logger = ARSTREAM_Logger_NewWithDefaultName ();
    ARSTREAM_Logger_Log (logger, "Latency (ms); PercentOK (%%); Missed frames; Mean time between frames (ms); Efficiency");
    ARSAL_PRINT (ARSAL_PRINT_DEBUG, __TAG__, "Latency (ms); PercentOK (%%); Missed frames; Mean time between frames (ms); Efficiency");
    while (1)
    {
        int lat = ARSTREAM_ReaderTb_GetLatency ();
        int missed = ARSTREAM_ReaderTb_GetMissedFrames ();
        int dt = ARSTREAM_ReaderTb_GetMeanTimeBetweenFrames ();
        float eff = ARSTREAM_ReaderTb_GetEfficiency ();
        ARSAL_PRINT (ARSAL_PRINT_DEBUG, __TAG__,"%4d; %5.2f; %3d; %4d; %5.3f", lat, ARSTREAM_Reader_PercentOk, missed, dt, eff);
        ARSTREAM_Logger_Log (logger, "%4d; %5.2f; %3d; %4d; %5.3f", lat, ARSTREAM_Reader_PercentOk, missed, dt, eff);
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

    ARSTREAM_Reader_LinuxTb_Args_t args = {argc, argv};
    int retVal;

    pthread_create (&tbThread, NULL, tbMain, &args);
    pthread_create (&reportThread, NULL, reportMain, NULL);

    pthread_join (reportThread, NULL);
    pthread_join (tbThread, (void **)&retVal);

    return retVal;
}
