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
/*****************************************************************************/
/*                                                                           */
/*                  ITTIAM SYSTEMS PVT LTD, BANGALORE                        */
/*                           COPYRIGHT(C) 2006                               */
/*                                                                           */
/*  This program  is  proprietary to  Ittiam  Systems  Private  Limited  and */
/*  is protected under Indian  Copyright Law as an unpublished work. Its use */
/*  and  disclosure  is  limited by  the terms  and  conditions of a license */
/*  agreement. It may not be copied or otherwise  reproduced or disclosed to */
/*  persons outside the licensee's organization except in accordance with the*/
/*  the  terms  and  conditions   of  such  an  agreement.  All  copies  and */
/*  reproductions shall be the property of Ittiam Systems Private Limited and*/
/*  must bear this notice in its entirety.                                   */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*  File Name         : it_file.h                                            */
/*                                                                           */
/*  Description       : Contains interface definition of file object. All    */
/*                      file objects have to implement this interface.       */
/*                                                                           */
/*  List of Functions : None                                                 */
/*                                                                           */
/*  Issues / Problems : None                                                 */
/*                                                                           */
/*  Revision History  :                                                      */
/*                                                                           */
/*         DD MM YYYY   Author(s)       Changes (Describe the changes made)  */
/*         24 11 2006   Ittiam          Creation                             */
/*                                                                           */
/*****************************************************************************/
#ifndef __IT_FILE_H__
#define __IT_FILE_H__

typedef struct it_file_t it_file_t;

typedef enum
{
    IT_FILE_S_OK = 0,
	/*general errors*/
    IT_FILE_E_FAILED,

    /*read errors*/
    IT_FILE_E_READ_NOT_ALLOWED,
    IT_FILE_E_DEVICE_READ_FAILED,
    IT_FILE_E_EOF_REACHED,

    /*write errors*/
    IT_FILE_E_WRITE_NOT_ALLOWED,
    
	/*seek errors*/
    IT_FILE_E_SEEK_INCAPABLE,
    IT_FILE_E_SEEK_BEFORE_START,
    IT_FILE_E_SEEK_BEYOND_END,
    
	/*tell errors*/

	/*length errors*/
    IT_FILE_E_LENGTH_UNKNOWN,

	/*eof errors*/

	/*misc errors*/
	IT_FILE_E_INVALID_ARG,
    IT_FILE_E_FEATURE_UNSUPPORTED

} it_file_result_t;

typedef enum
{
    IT_FILE_SEEK_SET,
    IT_FILE_SEEK_CUR,
    IT_FILE_SEEK_END
} it_seek_origin_t;

struct it_file_t
{
	it_file_result_t	(*read)		(IT_IN it_file_t *it_file,	
									IT_IN UWORD32 size,
									IT_IN UWORD32 count,	
									IT_IN void *buf,
									IT_OUT UWORD32 *bytes_read);
	it_file_result_t	(*write)	(IT_IN it_file_t *it_file,	
									IT_IN UWORD32 size,
									IT_IN UWORD32 count,	
									IT_IN void *buf,
									IT_OUT UWORD32 *bytes_written);

	it_file_result_t	(*seek)		(IT_IN it_file_t *it_file,
									IT_IN WORD64 offset,
									IT_IN it_seek_origin_t it_seek_origin);
	it_file_result_t	(*tell)		(IT_IN it_file_t *it_file,
									IT_OUT UWORD64 *offset);

	it_file_result_t	(*length)	(IT_IN it_file_t *it_file,
									IT_OUT UWORD64 *length);
	it_file_result_t	(*is_eof)	(IT_IN it_file_t *it_file,
									IT_OUT IT_BOOL *is_eof);
};

#endif /* __IT_FILE_H__ */

