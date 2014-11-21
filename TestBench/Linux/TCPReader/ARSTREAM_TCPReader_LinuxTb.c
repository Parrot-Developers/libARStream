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

    return (void *)(intptr_t)retVal;
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
