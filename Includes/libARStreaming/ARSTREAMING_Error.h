/**
 * @file ARSTREAMING_Error.h
 * @brief Error codes for ARSTREAMING_xxx calls
 * @date 06/19/2013
 * @author nicolas.brulez@parrot.com
 */

#ifndef _ARSTREAMING_ERROR_H_
#define _ARSTREAMING_ERROR_H_

/*
 * System Headers
 */

/*
 * ARSDK Headers
 */

/*
 * Macros
 */

/*
 * Types
 */

/**
 * @brief Error codes for ARSTREAMING_xxx calls
 */
typedef enum {
    ARSTREAMING_OK = 0, /**< No error */
    ARSTREAMING_ERROR_BAD_PARAMETERS, /**< Bad parameters */
    ARSTREAMING_ERROR_ALLOC, /**< Unable to allocate data */
    ARSTREAMING_ERROR_FRAME_TOO_LARGE, /**< Bad parameter : frame too large */
    ARSTREAMING_ERROR_BUSY, /**< Object is busy and can not be deleted yet */
    ARSTREAMING_ERROR_QUEUE_FULL, /**< Frame queue is full */
} eARSTREAMING_ERROR;

/**
 * @brief Gets the error string associated with an eARSTREAMING_ERROR
 * @param error The error to describe
 * @return A static string describing the error
 *
 * @note User should NEVER try to modify a returned string
 */
char* ARSTREAMING_Error_ToString (eARSTREAMING_ERROR error);

#endif /* _ARSTREAMING_ERROR_H_ */
