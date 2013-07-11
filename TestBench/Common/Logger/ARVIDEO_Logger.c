#include "../../Common/Logger/ARVIDEO_Logger.h"
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>

#define ARVIDEO_LOGGER_DEFAULT_NAME "ARVideoLog"
#define ARVIDEO_LOGGER_DATE_FORMAT  "_%04d%02d%02d_%02d%02d%02d"
#define ARVIDEO_LOGGER_STRLEN (1024)

struct ARVIDEO_Logger {
    char filename [ARVIDEO_LOGGER_STRLEN];
    char date[ARVIDEO_LOGGER_STRLEN];
    FILE *file;
};

ARVIDEO_Logger_t* ARVIDEO_Logger_NewWithDefaultName ()
{
    return ARVIDEO_Logger_New (ARVIDEO_LOGGER_DEFAULT_NAME, 1);
}

ARVIDEO_Logger_t* ARVIDEO_Logger_New (const char *basePath, int useDate)
{
    ARVIDEO_Logger_t *retLogger = calloc (1, sizeof (ARVIDEO_Logger_t));
    if (retLogger == NULL)
    {
        return retLogger;
    }

    if (useDate == 1)
    {
        struct tm *nowtm;
        time_t now = time (NULL);
        nowtm = localtime (&now);
        snprintf (retLogger->date, ARVIDEO_LOGGER_STRLEN, ARVIDEO_LOGGER_DATE_FORMAT, nowtm->tm_year+1900, nowtm->tm_mon+1, nowtm->tm_mday, nowtm->tm_hour, nowtm->tm_min, nowtm->tm_sec);
    }
    snprintf (retLogger->filename, ARVIDEO_LOGGER_STRLEN, "./%s%s.log", basePath, retLogger->date);

    retLogger->file = fopen (retLogger->filename, "w+b");

    if (retLogger->file == NULL)
    {
        ARVIDEO_Logger_Delete (&retLogger);
    }

    return retLogger;
}

void ARVIDEO_Logger_Delete (ARVIDEO_Logger_t **logger)
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

void ARVIDEO_Logger_Log (ARVIDEO_Logger_t *logger, const char *format, ...)
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
