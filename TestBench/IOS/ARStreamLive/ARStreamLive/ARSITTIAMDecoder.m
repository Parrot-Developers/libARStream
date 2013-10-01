//
//  ARSITTIAMDecoder.m
//  ARStreamLive
//
//  Created by Nicolas BRULEZ on 11/07/13.
//  Copyright (c) 2013 parrot. All rights reserved.
//

#import "ARSITTIAMDecoder.h"


//ITTIAM Common inlcudes
#include "datatypedef.h"
#include "it_mem.h"
#include "it_memory.h"

// H264 include
#include "ih264d_cxa8.h"

#define H264_MAX_LEVEL_SUPPORTED            42
#define H264_MAX_FRAME_WIDTH                1920
#define H264_MAX_FRAME_HEIGHT               1088
#define H264_MAX_REF_FRAMES                 2
#define H264_MAX_REORDER_FRAMES                 0

#define H264_MIN_FRAME_WIDTH                64
#define H264_MIN_FRAME_HEIGHT               64

#define WAIT_FOR_I_FRAME (1)

@interface ARSITTIAMDecoder ()

@property (nonatomic) int width;
@property (nonatomic) int height;

@property (nonatomic) int nbDecoded;

@property (nonatomic, strong) ARSFrame *outputFrame;
@property (nonatomic) uint32_t num_picture_decoded;

// ITTIAM H264 Vars
@property (nonatomic) iv_obj_t * H264_DECHDL;
@property (nonatomic) it_mem_t * h264_ps_it_mem;
@property (nonatomic) iv_mem_rec_t * h264_mem_rec;
@property (nonatomic) ivd_video_decode_ip_t h264_video_decode_ip;
@property (nonatomic) ivd_video_decode_op_t h264_video_decode_op;
@property (nonatomic) ivd_get_display_frame_ip_t h264_get_display_frame_ip;
@property (nonatomic) ivd_get_display_frame_op_t h264_get_display_frame_op;
@property (nonatomic) uint32_t h264_num_mem_rec;

@property (nonatomic) BOOL initialized;

@end

@implementation ARSITTIAMDecoder

- (id)init
{
    self = [super init];
    if (self)
    {
        _outputFrame = [[ARSFrame alloc] initWithCapacity:(MAX_RES_X * MAX_RES_Y * BPP)];
        _initialized = NO;
        _width = 1280;
        _height = 720;
        _nbDecoded = 0;
    }
    return self;
}

- (void)initializeWithWidth:(int)w andHeight:(int)h
{
    if (_initialized)
    {
        return;
    }
    
    _width = w;
    _height = h;
    
    /****************************************************************************/
    /* H264 ====== Initialize the memory records
     *****************************************************************************/
    NSLog (@"ITTIAM INIT (%3d x %3d)", _width, _height);
    
    iv_num_mem_rec_ip_t h264_num_mem_rec_ip;
    iv_num_mem_rec_op_t h264_num_mem_rec_op;
    
    h264_num_mem_rec_ip.e_cmd = IV_CMD_GET_NUM_MEM_REC;
    h264_num_mem_rec_ip.u4_size = sizeof (iv_num_mem_rec_ip_t);
    h264_num_mem_rec_op.u4_size = sizeof (iv_num_mem_rec_op_t);
    
    if (ih264d_cxa8_api_function(_H264_DECHDL, (void*) (&h264_num_mem_rec_ip), (void*) (&h264_num_mem_rec_op)) == IV_SUCCESS) {
        NSLog (@"IV_CMD_GET_NUM_MEM_REC    [ OK ]");
    } else {
        NSLog (@"IV_CMD_GET_NUM_MEM_REC    [ NOK ] with error %d", (UWORD32) h264_num_mem_rec_op.u4_error_code);
    }
    
    NSLog (@"Number of records : %d", h264_num_mem_rec_op.u4_num_mem_rec);
    _h264_num_mem_rec = h264_num_mem_rec_op.u4_num_mem_rec;
    
    
    /****************************************************************************/
    /* H264 ====== Allocate the pointers
     *****************************************************************************/
    _h264_ps_it_mem = (it_mem_t *) malloc(sizeof (it_mem_t));
    if (_h264_ps_it_mem == NULL) {
        NSLog (@"\nAllocation failure\n");
        return;
    }
    it_mem_init(_h264_ps_it_mem);
    
    _h264_mem_rec = _h264_ps_it_mem->alloc(_h264_ps_it_mem, (h264_num_mem_rec_op.u4_num_mem_rec) * sizeof (iv_mem_rec_t));
    if (_h264_mem_rec == NULL) {
        NSLog (@"\nAllocation failure\n");
        return;
    }
    
    /****************************************************************************/
    /* H264 ====== Fill the memory with some information
     *****************************************************************************/
    
    ih264d_cxa8_fill_mem_rec_ip_t h264_fill_mem_rec_ip;
    ih264d_cxa8_fill_mem_rec_op_t h264_fill_mem_rec_op;
    
    h264_fill_mem_rec_ip.s_ivd_fill_mem_rec_ip_t.e_cmd = IV_CMD_FILL_NUM_MEM_REC;
    h264_fill_mem_rec_ip.s_ivd_fill_mem_rec_ip_t.pv_mem_rec_location = _h264_mem_rec;
    h264_fill_mem_rec_ip.s_ivd_fill_mem_rec_ip_t.u4_max_frm_wd = _width;
    h264_fill_mem_rec_ip.s_ivd_fill_mem_rec_ip_t.u4_max_frm_ht = _height;
    h264_fill_mem_rec_ip.s_level = H264_MAX_LEVEL_SUPPORTED;
    h264_fill_mem_rec_ip.s_num_ref_frames = H264_MAX_REF_FRAMES;
    h264_fill_mem_rec_ip.s_num_reorder_frames = H264_MAX_REORDER_FRAMES;
    h264_fill_mem_rec_ip.s_ivd_fill_mem_rec_ip_t.u4_size = sizeof (ih264d_cxa8_fill_mem_rec_ip_t);
    h264_fill_mem_rec_op.s_ivd_fill_mem_rec_op_t.u4_size = sizeof (ih264d_cxa8_fill_mem_rec_op_t);
    
    int i;
    for (i = 0; i < h264_num_mem_rec_op.u4_num_mem_rec; i++)
        _h264_mem_rec[i].u4_size = sizeof (iv_mem_rec_t);
    
    if (ih264d_cxa8_api_function(_H264_DECHDL, (void*) (&h264_fill_mem_rec_ip), (void*) (&h264_fill_mem_rec_op)) == IV_SUCCESS) {
        NSLog (@"IV_CMD_FILL_NUM_MEM_REC    [ OK ]");
    } else {
        NSLog (@"IV_CMD_FILL_NUM_MEM_REC    [ NOK ]");
    }
    
    //Do some allocation on the mem_rec pointer
    iv_mem_rec_t * h264_temp_mem_rec = _h264_mem_rec;
    for (i = 0; i < h264_num_mem_rec_op.u4_num_mem_rec; i++) {
        h264_temp_mem_rec->pv_base = _h264_ps_it_mem->align_alloc(_h264_ps_it_mem, h264_temp_mem_rec->u4_mem_size, h264_temp_mem_rec->u4_mem_alignment);
        if (h264_temp_mem_rec->pv_base == NULL) {
            NSLog (@"\nAllocation failure\n");
        }
        h264_temp_mem_rec++;
    }
    
    
    /****************************************************************************/
    /* H264 ====== Init the DECHDL
     *****************************************************************************/
    ih264d_cxa8_init_ip_t h264_init_ip;
    ih264d_cxa8_init_op_t h264_init_op;
    
    void *h264_fxns = &ih264d_cxa8_api_function;
    
    iv_mem_rec_t *h264_mem_tab = (iv_mem_rec_t*) _h264_mem_rec;
    
    h264_init_ip.s_ivd_init_ip_t.e_cmd = (IVD_API_COMMAND_TYPE_T)IV_CMD_INIT;
    h264_init_ip.s_ivd_init_ip_t.pv_mem_rec_location = h264_mem_tab;
    h264_init_ip.s_ivd_init_ip_t.u4_frm_max_wd = _width;
    h264_init_ip.s_ivd_init_ip_t.u4_frm_max_ht = _height;
    h264_init_ip.s_level = H264_MAX_LEVEL_SUPPORTED;
    h264_init_ip.s_num_ref_frames = H264_MAX_REF_FRAMES;
    h264_init_ip.s_num_reorder_frames = H264_MAX_REORDER_FRAMES;
    h264_init_ip.s_ivd_init_ip_t.u4_num_mem_rec = h264_num_mem_rec_op.u4_num_mem_rec;
    h264_init_ip.s_ivd_init_ip_t.e_output_format = IV_RGB_565;
    h264_init_ip.s_ivd_init_ip_t.u4_size = sizeof (ih264d_cxa8_init_ip_t);
    h264_init_op.s_ivd_init_op_t.u4_size = sizeof (ih264d_cxa8_init_op_t);
    
    _H264_DECHDL = (iv_obj_t*) h264_mem_tab[0].pv_base;
    _H264_DECHDL->pv_fxns = h264_fxns;
    _H264_DECHDL->u4_size = sizeof (iv_obj_t);
    
    if (ih264d_cxa8_api_function(_H264_DECHDL, (void *) &h264_init_ip, (void *) &h264_init_op) == IV_SUCCESS) {
        NSLog (@"IV_CMD_INIT    [ OK ]");
    } else {
        NSLog (@"IV_CMD_INIT    [ NOK ]");
    }
    
    /****************************************************************************/
    /* H264 ====== Set decoder config
     *****************************************************************************/
    ivd_ctl_set_config_ip_t h264_ctl_set_config_ip;
    ivd_ctl_set_config_op_t h264_ctl_set_config_op;
    h264_ctl_set_config_ip.u4_disp_wd = _width;
    h264_ctl_set_config_ip.e_frm_skip_mode = IVD_NO_SKIP;
    h264_ctl_set_config_ip.e_frm_out_mode = IVD_DISPLAY_FRAME_OUT;
    h264_ctl_set_config_ip.e_vid_dec_mode = IVD_DECODE_FRAME;
    h264_ctl_set_config_ip.e_cmd = IVD_CMD_VIDEO_CTL;
    h264_ctl_set_config_ip.e_sub_cmd = IVD_CMD_CTL_SETPARAMS;
    h264_ctl_set_config_ip.u4_size = sizeof (ivd_ctl_set_config_ip_t);
    h264_ctl_set_config_op.u4_size = sizeof (ivd_ctl_set_config_op_t);
    
    if (ih264d_cxa8_api_function(_H264_DECHDL, (void *) &h264_ctl_set_config_ip, (void *) &h264_ctl_set_config_op) == IV_SUCCESS) {
        NSLog (@"IVD_CMD_VIDEO_CTL => IVD_CMD_CTL_SETPARAMS   [ OK ]");
    } else {
        NSLog (@"IVD_CMD_VIDEO_CTL => IVD_CMD_CTL_SETPARAMS   [ NOK ]");
    }
    
    /****************************************************************************/
    /* H264 ====== Decode the in buffer INIT PART
     *****************************************************************************/
    _h264_video_decode_ip.e_cmd = IVD_CMD_VIDEO_DECODE;
    _h264_video_decode_ip.u4_size = sizeof (ivd_video_decode_ip_t);
    _h264_video_decode_op.u4_size = sizeof (ivd_video_decode_op_t);
    
    /****************************************************************************/
    /* H264 ====== Display the buffer INIT PART
     *****************************************************************************/
    _h264_get_display_frame_ip.e_cmd = IVD_CMD_GET_DISPLAY_FRAME;
    _h264_get_display_frame_ip.u4_size = sizeof (ivd_get_display_frame_ip_t);
    _h264_get_display_frame_op.u4_size = sizeof (ivd_get_display_frame_op_t);
    
    /****************************************************************************/
    /* H264 ====== Get the buffers information to re-use them
     *****************************************************************************/
    ivd_ctl_getbufinfo_ip_t h264_ctl_dec_ip;
    ivd_ctl_getbufinfo_op_t h264_ctl_dec_op;
    
    h264_ctl_dec_ip.e_cmd = IVD_CMD_VIDEO_CTL;
    h264_ctl_dec_ip.e_sub_cmd = IVD_CMD_CTL_GETBUFINFO;
    h264_ctl_dec_ip.u4_size = sizeof (ivd_ctl_getbufinfo_ip_t);
    h264_ctl_dec_op.u4_size = sizeof (ivd_ctl_getbufinfo_op_t);
    
    if (ih264d_cxa8_api_function(_H264_DECHDL, (void *) &h264_ctl_dec_ip, (void *) &h264_ctl_dec_op) == IV_SUCCESS) {
        NSLog (@"IVD_CMD_VIDEO_CTL => IVD_CMD_CTL_GETBUFINFO   [ OK ]");
    } else {
        NSLog (@"IVD_CMD_VIDEO_CTL => IVD_CMD_CTL_GETBUFINFO   [ NOK ]");
    }
    
    //Allocate the output buffer used to store decoded frame (RGB 565)
    
    
    _h264_get_display_frame_ip.s_out_buffer.u4_min_out_buf_size[0] = h264_ctl_dec_op.u4_min_out_buf_size[0];
    _h264_get_display_frame_ip.s_out_buffer.pu1_bufs[0] = _h264_ps_it_mem->alloc(_h264_ps_it_mem, h264_ctl_dec_op.u4_min_out_buf_size[0]);
    _h264_get_display_frame_ip.s_out_buffer.u4_num_bufs = h264_ctl_dec_op.u4_min_num_out_bufs;
    
    NSLog (@"min buf size = %d", _h264_get_display_frame_ip.s_out_buffer.u4_min_out_buf_size[0]);
    NSLog (@"num out buf  = %d", _h264_get_display_frame_ip.s_out_buffer.u4_num_bufs);
    NSLog (@"@buffer      = %p", _h264_get_display_frame_ip.s_out_buffer.pu1_bufs[0]);
    
    puts("******************************** ITTIAM H264 decoding init *********************************");
    
    _initialized = YES;
}

- (void)dealloc
{
    [self close];
}

- (void)close
{
    if (!_initialized)
    {
        return;
    }
    /****************************************************************************/
    /* H264 ====== Reset the memory records
     *****************************************************************************/
    NSLog (@"ITTIAM RESET");
    
    ivd_ctl_reset_ip_t h264_ctl_reset_ip;
    ivd_ctl_reset_op_t h264_ctl_reset_op;
    
    h264_ctl_reset_ip.e_cmd = IVD_CMD_VIDEO_CTL;
    h264_ctl_reset_ip.e_sub_cmd = IVD_CMD_CTL_RESET;
    h264_ctl_reset_ip.u4_size = sizeof (ivd_ctl_reset_ip_t);
    h264_ctl_reset_op.u4_size = sizeof (ivd_ctl_reset_op_t);
    
    if (ih264d_cxa8_api_function(_H264_DECHDL, (void*) (&h264_ctl_reset_ip), (void*) (&h264_ctl_reset_op)) == IV_SUCCESS) {
        NSLog (@"IVD_CMD_CTL_RESET    [ OK ]");
    } else {
        NSLog (@"IVD_CMD_CTL_RESET    [ NOK ] with error %d", (UWORD32) h264_ctl_reset_op.u4_error_code);
        
    }
    
    iv_mem_rec_t * h264_temp_mem_rec = _h264_mem_rec;
    int i;
    for (i = 0; i < _h264_num_mem_rec; i++) {
        _h264_ps_it_mem->free(_h264_ps_it_mem, h264_temp_mem_rec->pv_base);
        h264_temp_mem_rec++;
    }
    
    _h264_ps_it_mem->free(_h264_ps_it_mem, _h264_get_display_frame_ip.s_out_buffer.pu1_bufs[0]);
    
    NSLog (@"ITTIAM CLEAN");
    
    _initialized = NO;
}

- (ARSFrame *)decodeFrame:(ARSFrame *)frame
{
    ARSFrame *retFrame = nil;
    BOOL waitForIFrame = NO;
    if (frame.missed != 0) {
        waitForIFrame = YES;
    }

    if (!waitForIFrame || frame.isIFrame) {
        /****************************************************************************/
        /* Decode the in buffer EXEC PART
         *****************************************************************************/
        
        _h264_video_decode_ip.u4_ts = _num_picture_decoded;
        _h264_video_decode_ip.pv_stream_buffer = ((unsigned char*) frame.data);
        _h264_video_decode_ip.u4_num_Bytes = frame.used;
        
        if (ih264d_cxa8_api_function(_H264_DECHDL, (void *) &_h264_video_decode_ip, (void *) &_h264_video_decode_op) == IV_SUCCESS) {
            //NSLog (@"IVD_CMD_VIDEO_DECODE   [ OK ]");
        } else {
            NSLog (@"IVD_CMD_VIDEO_DECODE   [ NOK ]");
        }
        
        /****************************************************************************/
        /* Display the buffer EXEC PART
         *****************************************************************************/
        
        if (ih264d_cxa8_api_function(_H264_DECHDL, (void *) &_h264_get_display_frame_ip, (void *) &_h264_get_display_frame_op) == IV_SUCCESS) {
            //NSLog (@"IVD_CMD_GET_DISPLAY_FRAME   [ OK ]");
            
            if (_h264_video_decode_op.u4_frame_decoded_flag == 1) {
                
                _num_picture_decoded++;
                [self incrementNbDecoded];
                
                //Adress of output ittiam pointer
                _outputFrame.data = (uint8_t*) _h264_get_display_frame_ip.s_out_buffer.pu1_bufs[0];
                _outputFrame.used = _h264_get_display_frame_ip.s_out_buffer.u4_min_out_buf_size[0];
                _outputFrame.missed = frame.missed;
                _outputFrame.isIFrame = frame.isIFrame;
                retFrame = _outputFrame;
            }
        } else {
            NSLog (@"IVD_CMD_GET_DISPLAY_FRAME   [ NOK ]");
        }
    }
    return retFrame;
}

- (void)incrementNbDecoded
{
    @synchronized (self)
    {
        _nbDecoded++;
    }
}

- (int)getAndResetNbDecoded
{
    int ret = 0;
    @synchronized (self)
    {
        ret = _nbDecoded;
        _nbDecoded = 0;
    }
    return ret;
}

@end
