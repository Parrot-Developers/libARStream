/*
    Copyright (C) 2014 Parrot SA

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in
      the documentation and/or other materials provided with the 
      distribution.
    * Neither the name of Parrot nor the names
      of its contributors may be used to endorse or promote products
      derived from this software without specific prior written
      permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
    "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
    LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
    FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
    COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
    BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
    OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED 
    AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
    OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
    OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
    SUCH DAMAGE.
*/
/*
 * GENERATED FILE
 *  Do not modify this file, it will be erased during the next configure run 
 */

/**
 * @file ARSTREAM_Error.c
 * @brief ToString function for eARSTREAM_ERROR enum
 */

#include <libARStream/ARSTREAM_Error.h>

char* ARSTREAM_Error_ToString (eARSTREAM_ERROR error)
{
    switch (error)
    {
    case ARSTREAM_OK:
        return "No error";
        break;
    case ARSTREAM_ERROR_BAD_PARAMETERS:
        return "Bad parameters";
        break;
    case ARSTREAM_ERROR_ALLOC:
        return "Unable to allocate data";
        break;
    case ARSTREAM_ERROR_FRAME_TOO_LARGE:
        return "Bad parameter : frame too large";
        break;
    case ARSTREAM_ERROR_BUSY:
        return "Object is busy and the operation can not be applied on running objects";
        break;
    case ARSTREAM_ERROR_QUEUE_FULL:
        return "Frame queue is full";
        break;
    default:
        return "Unknown value";
        break;
    }
    return "Unknown value";
}
