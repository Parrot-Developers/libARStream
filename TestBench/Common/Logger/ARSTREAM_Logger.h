#ifndef _ARSTREAM_LOGGER_H_
#define _ARSTREAM_LOGGER_H_

typedef struct ARSTREAM_Logger ARSTREAM_Logger_t;

ARSTREAM_Logger_t* ARSTREAM_Logger_NewWithDefaultName ();
ARSTREAM_Logger_t* ARSTREAM_Logger_New (const char *basePath, int useDate);

void ARSTREAM_Logger_Log (ARSTREAM_Logger_t *logger, const char *format, ...);

void ARSTREAM_Logger_Delete (ARSTREAM_Logger_t **logger);

#endif /* _ARSTREAM_LOGGER_H_ */
