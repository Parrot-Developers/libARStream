#include "../../Common/Logger/ARSTREAMING_Logger.h"
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>

#define ARSTREAMING_LOGGER_DEFAULT_NAME "ARStreamingLog"
#define ARSTREAMING_LOGGER_DATE_FORMAT  "_%04d%02d%02d_%02d%02d%02d"
#define ARSTREAMING_LOGGER_STRLEN (1024)

struct ARSTREAMING_Logger {
    char filename [ARSTREAMING_LOGGER_STRLEN];
    char date[ARSTREAMING_LOGGER_STRLEN];
    FILE *file;
};

ARSTREAMING_Logger_t* ARSTREAMING_Logger_NewWithDefaultName ()
{
    return ARSTREAMING_Logger_New (ARSTREAMING_LOGGER_DEFAULT_NAME, 1);
}

ARSTREAMING_Logger_t* ARSTREAMING_Logger_New (const char *basePath, int useDate)
{
    ARSTREAMING_Logger_t *retLogger = calloc (1, sizeof (ARSTREAMING_Logger_t));
    if (retLogger == NULL)
    {
        return retLogger;
    }

    if (useDate == 1)
    {
        struct tm *nowtm;
        time_t now = time (NULL);
        nowtm = localtime (&now);
        snprintf (retLogger->date, ARSTREAMING_LOGGER_STRLEN, ARSTREAMING_LOGGER_DATE_FORMAT, nowtm->tm_year+1900, nowtm->tm_mon+1, nowtm->tm_mday, nowtm->tm_hour, nowtm->tm_min, nowtm->tm_sec);
    }
    snprintf (retLogger->filename, ARSTREAMING_LOGGER_STRLEN, "./%s%s.log", basePath, retLogger->date);

    retLogger->file = fopen (retLogger->filename, "w+b");

    if (retLogger->file == NULL)
    {
        ARSTREAMING_Logger_Delete (&retLogger);
    }

    return retLogger;
}

void ARSTREAMING_Logger_Delete (ARSTREAMING_Logger_t **logger)
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

void ARSTREAMING_Logger_Log (ARSTREAMING_Logger_t *logger, const char *format, ...)
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
