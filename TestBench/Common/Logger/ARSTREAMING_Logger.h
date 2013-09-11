#ifndef _ARSTREAMING_LOGGER_H_
#define _ARSTREAMING_LOGGER_H_

typedef struct ARSTREAMING_Logger ARSTREAMING_Logger_t;

ARSTREAMING_Logger_t* ARSTREAMING_Logger_NewWithDefaultName ();
ARSTREAMING_Logger_t* ARSTREAMING_Logger_New (const char *basePath, int useDate);

void ARSTREAMING_Logger_Log (ARSTREAMING_Logger_t *logger, const char *format, ...);

void ARSTREAMING_Logger_Delete (ARSTREAMING_Logger_t **logger);

#endif /* _ARSTREAMING_LOGGER_H_ */
