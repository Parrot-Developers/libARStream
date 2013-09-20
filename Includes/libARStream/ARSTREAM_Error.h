/**
 * @file ARSTREAM_Error.h
 * @brief Error codes for ARSTREAM_xxx calls
 * @date 06/19/2013
 * @author nicolas.brulez@parrot.com
 */

#ifndef _ARSTREAM_ERROR_H_
#define _ARSTREAM_ERROR_H_

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
 * @brief Error codes for ARSTREAM_xxx calls
 */
typedef enum {
    ARSTREAM_OK = 0, /**< No error */
    ARSTREAM_ERROR_BAD_PARAMETERS, /**< Bad parameters */
    ARSTREAM_ERROR_ALLOC, /**< Unable to allocate data */
    ARSTREAM_ERROR_FRAME_TOO_LARGE, /**< Bad parameter : frame too large */
    ARSTREAM_ERROR_BUSY, /**< Object is busy and can not be deleted yet */
    ARSTREAM_ERROR_QUEUE_FULL, /**< Frame queue is full */
} eARSTREAM_ERROR;

/**
 * @brief Gets the error string associated with an eARSTREAM_ERROR
 * @param error The error to describe
 * @return A static string describing the error
 *
 * @note User should NEVER try to modify a returned string
 */
char* ARSTREAM_Error_ToString (eARSTREAM_ERROR error);

#endif /* _ARSTREAM_ERROR_H_ */
