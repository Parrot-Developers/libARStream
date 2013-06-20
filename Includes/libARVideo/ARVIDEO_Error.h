/**
 * @file ARVIDEO_Error.h
 * @brief Error codes for ARVIDEO_xxx calls
 * @date 06/19/2013
 * @author nicolas.brulez@parrot.com
 */

#ifndef _ARVIDEO_ERROR_H_
#define _ARVIDEO_ERROR_H_

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
 * @brief Error codes for ARVIDEO_xxx calls
 */
typedef enum {
    ARVIDEO_ERROR_OK = 0, /**< No error */
    ARVIDEO_ERROR_BAD_PARAMETERS, /**< Bad parameters */
    ARVIDEO_ERROR_ALLOC, /**< Unable to allocate data */
    ARVIDEO_ERROR_FRAME_TOO_LARGE, /**< Bad parameter : frame too large */
    ARVIDEO_ERROR_BUSY, /**< Object is busy and can not be deleted yet */
    ARVIDEO_ERROR_QUEUE_FULL, /**< Frame queue is full */
} eARVIDEO_ERROR;

/**
 * @brief Gets the error string associated with an eARVIDEO_ERROR
 * @param error The error to describe
 * @return A static string describing the error
 *
 * @note User should NEVER try to modify a returned string
 */
char* ARVIDEO_Error_ToString (eARVIDEO_ERROR error);

#endif /* _ARVIDEO_ERROR_H_ */
