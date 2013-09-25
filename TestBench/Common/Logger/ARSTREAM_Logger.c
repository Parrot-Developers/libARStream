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
