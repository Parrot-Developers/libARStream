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
#ifndef __VIDEO_APPLICATION_MAIN__
#define __VIDEO_APPLICATION_MAIN__
/*******************************************************************************
*
*						   MPEG-4 Video Decoder
*                    ITTIAM SYSTEMS PVT LTD, BANGALORE
*                            COPYRIGHT(C) 2001
*
*  This program is proprietary to Ittiam Systems Pvt. Ltd. and is protected
*  under Indian Copyright Act as an unpublished work. Its use and disclosure is
*  limited by the terms and conditions of a license agreement. It may not be
*  copied or otherwise reproduced or disclosed to persons outside the licensee's
*  organization except in accordance with the terms and conditions of such an
*  agreement. All copies and reproductions shall be the property of Ittiam
*  under the  Indian Copyright Act as an unpublished work. Its use and
*  disclosure is limited by the terms and conditions of the licensing agreement
*  between Ittiam and Sony UK dated October 30, 2001 . It may not be copied or
*  otherwise reproduced or disclosed to persons outside the licensee's
*  organization except in accordance with the terms and conditions of the said
*  agreement.
*  Systems India Pvt. Ltd. and must bear this notice in its  entirety.
*
*  File Name      : app_video_decode.h
*
*  Description    : This file contains defination of various functions used by
*		application	and context structure used by decoder
*
*  Revision History:
*
*         29 05 2007   Rajneesh   First version of the code
*
******************************************************************************/

/*****************************************************************************/
/* Constant Macros                                                           */
/*****************************************************************************/

#define ARG_DISABLED    0    /* always zero for PC/CE compilation */

#if ARG_DISABLED
#define CONTROL_FILE "D:\\work\\mpeg4_asp\\application\\test\\dec.ctl"
#endif


#ifdef RVDS
#define DISABLED_FOR_PROFILING      0
#else
#ifdef WIN32
#define DISABLED_FOR_PROFILING      0
#else
/* Making it 1, will disable file writes in the ARM worksapce */
#define DISABLED_FOR_PROFILING      0
#endif
#endif




#define H264_MAX_LEVEL_SUPPORTED 			42
#define H264_MAX_FRAME_WIDTH				1920
#define H264_MAX_FRAME_HEIGHT				1088
#define H264_MAX_REF_FRAMES					16
#define H264_MAX_REORDER_FRAMES				16

#define H264_MIN_FRAME_WIDTH				64
#define H264_MIN_FRAME_HEIGHT				64




#define MAX_BITRATE_SUPPORTED           (7*1024*1024>>1)




#define MAX_BYTES_TO_SRCH_FRM_START     100

#define MAX_BITSTREAM_BUFFER_SIZE       (MAX_BITRATE_SUPPORTED)


typedef enum {

    DECODER_ERROR       = -1,
    DECODER_SUCCESS     = 1

}DEC_CALL_STATUS_T;


typedef struct {

    it_mem_t    *ps_it_mem;
    it_file_t   *ps_ip_file;
    it_file_t   *ps_op_file;
    void        *pv_dec_handel;
    UWORD16     u2_file_save_flag;
    UWORD16     e_file_save_type;
    UWORD32     u4_max_frm_ts;
    UWORD32     u4_ip_buf_len;
    UWORD8      *pv_bs_buf;
    ivd_out_bufdesc_t s_out_buffer;
    IV_COLOR_FORMAT_T e_output_chroma_format;
	UWORD32 stride;


}vid_dec_ctx_t;
typedef struct {

    void        *pv_frm_buf[3];
    iv_mem_rec_t		*pv_mem_rec_location;
    UWORD32		u4_no_of_mem_rec;
    it_mem_t    *ps_mem;
    UWORD32     u4_frm_wd;
    UWORD32     u4_frm_ht;
    UWORD32     u4_frm_strd;
    UWORD32     u4_scratch_mem_need;
    UWORD32     u4_persistent_mem_need;
    UWORD32     u4_frm_hdr_done_flag;
    UWORD32     u4_num_frame_bufs;
	UWORD32		u4_max_ud_len;
	UWORD32     u4_frame_decoded_flag;
	UWORD32     u4_buf_id;
	iv_yuv_buf_t *ps_op_frm;
	UWORD32     flush_not_issued;
	UWORD32 u4_error_code;


}vid_dec_handel_t;


void read_cfg_file(vid_dec_ctx_t *ps_app_ctx,FILE *fp_cfg_file,
                   WORD8 *pu1_ip_file,
				   WORD8 *pu1_op_file);

void read_cfg_file_multi_inst(vid_dec_ctx_t *ps_app_ctx,FILE *fp_cfg_file,
                   WORD8 **pu1_ip_file,
				   WORD8 **pu1_op_file,
				   UWORD16 NUM_MULTI_INST);

UWORD32 fill_bs_buffer(it_file_t *ps_in_file,
                           UWORD32 u4_tot_bytes_decoded,
                           void *pv_bs_buf,UWORD32 u4_bs_buf_size);

void write_op_frm_to_file(void *pv_op_buf,
                            UWORD32 u4_op_buf_len
                            ,it_file_t *ps_op_file);


it_result_t vid_dec_create(vid_dec_ctx_t *ps_app_ctx, vid_dec_config_t *ps_config, void** ppv_handle);

it_result_t vid_dec_process(vid_dec_ctx_t  *s_app_ctx,
                            size_t  u4_inp_len,
                            UWORD32  u4_inp_timestamp,
                            UWORD32 inp_flag,
                            UWORD32 *inp_len_consumed,
                            IT_BOOL *out_present,
                            UWORD32  *out_luma_produced,
                            UWORD32  *out_timestamp,
                            UWORD32 *out_flag);

it_result_t vid_dec_destroy(void *pv_handle);


#endif

