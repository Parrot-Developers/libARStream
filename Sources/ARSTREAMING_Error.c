/*
 * GENERATED FILE
 *  Do not modify this file, it will be erased during the next configure run
 */

/**
 * @file ARSTREAMING_Error.c
 * @brief ToString function for eARSTREAMING_ERROR enum
 */

#include <libARStreaming/ARSTREAMING_Error.h>

char* ARSTREAMING_Error_ToString (eARSTREAMING_ERROR error)
{
    switch (error)
    {
    case ARSTREAMING_OK:
        return "No error";
        break;
    case ARSTREAMING_ERROR_BAD_PARAMETERS:
        return "Bad parameters";
        break;
    case ARSTREAMING_ERROR_ALLOC:
        return "Unable to allocate data";
        break;
    case ARSTREAMING_ERROR_FRAME_TOO_LARGE:
        return "Bad parameter : frame too large";
        break;
    case ARSTREAMING_ERROR_BUSY:
        return "Object is busy and can not be deleted yet";
        break;
    case ARSTREAMING_ERROR_QUEUE_FULL:
        return "Frame queue is full";
        break;
    default:
        return "Unknown value";
        break;
    }
    return "Unknown value";
}
