#ifndef _ARVIDEO_LOGGER_H_
#define _ARVIDEO_LOGGER_H_

typedef struct ARVIDEO_Logger ARVIDEO_Logger_t;

ARVIDEO_Logger_t* ARVIDEO_Logger_NewWithDefaultName ();
ARVIDEO_Logger_t* ARVIDEO_Logger_New (const char *basePath, int useDate);

void ARVIDEO_Logger_Log (ARVIDEO_Logger_t *logger, const char *format, ...);

void ARVIDEO_Logger_Delete (ARVIDEO_Logger_t **logger);

#endif /* _ARVIDEO_LOGGER_H_ */
