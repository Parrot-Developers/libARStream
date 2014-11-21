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
#include "../../Common/Logger/ARSTREAM_Logger.h"
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>

#define ARSTREAM_LOGGER_DEFAULT_NAME "ARStreamLog"
#define ARSTREAM_LOGGER_DATE_FORMAT  "_%04d%02d%02d_%02d%02d%02d"
#define ARSTREAM_LOGGER_STRLEN (1024)

#define ENABLE_LOGGER (1)

struct ARSTREAM_Logger {
    char filename [ARSTREAM_LOGGER_STRLEN];
    char date[ARSTREAM_LOGGER_STRLEN];
    FILE *file;
};

ARSTREAM_Logger_t* ARSTREAM_Logger_NewWithDefaultName ()
{
    return ARSTREAM_Logger_New (ARSTREAM_LOGGER_DEFAULT_NAME, 1);
}

ARSTREAM_Logger_t* ARSTREAM_Logger_New (const char *basePath, int useDate)
{
#if ENABLE_LOGGER
    ARSTREAM_Logger_t *retLogger = calloc (1, sizeof (ARSTREAM_Logger_t));
    if (retLogger == NULL)
    {
        return retLogger;
    }

    if (useDate == 1)
    {
        struct tm *nowtm;
        time_t now = time (NULL);
        nowtm = localtime (&now);
        snprintf (retLogger->date, ARSTREAM_LOGGER_STRLEN, ARSTREAM_LOGGER_DATE_FORMAT, nowtm->tm_year+1900, nowtm->tm_mon+1, nowtm->tm_mday, nowtm->tm_hour, nowtm->tm_min, nowtm->tm_sec);
    }
    snprintf (retLogger->filename, ARSTREAM_LOGGER_STRLEN, "./%s%s.log", basePath, retLogger->date);

    retLogger->file = fopen (retLogger->filename, "w+b");

    if (retLogger->file == NULL)
    {
        ARSTREAM_Logger_Delete (&retLogger);
    }

    return retLogger;
#else
    return NULL;
#endif
}

void ARSTREAM_Logger_Delete (ARSTREAM_Logger_t **logger)
{
    if (logger != NULL)
    {
        if ((*logger != NULL) &&
            ((*logger)->file != NULL))
        {
            fclose ((*logger)->file);
        }
        free (*logger);
        *logger = NULL;
    }
}

void ARSTREAM_Logger_Log (ARSTREAM_Logger_t *logger, const char *format, ...)
{
    va_list ap;

    if ((logger == NULL) ||
        (logger->file == NULL))
    {
        return;
    }

    va_start (ap, format);

    vfprintf (logger->file, format, ap);
    fprintf (logger->file, "\n");

    fflush (logger->file);

    va_end (ap);
}
