#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <arpa/inet.h>

#include "h264p.h"


#define printf(...)
#define fprintf(...)
#define log2(x) (log(x) / log(2))


#define BYTE_STREAM_NALU_START_CODE 0x00000001

#define SEI_PAYLOAD_TYPE_BUFFERING_PERIOD 0
#define SEI_PAYLOAD_TYPE_PIC_TIMING 1
#define SEI_PAYLOAD_TYPE_USER_DATA_UNREGISTERED 5

#define SLICE_TYPE_P 0
#define SLICE_TYPE_B 1
#define SLICE_TYPE_I 2
#define SLICE_TYPE_SP 3
#define SLICE_TYPE_SI 4
#define SLICE_TYPE_P_ALL 5
#define SLICE_TYPE_B_ALL 6
#define SLICE_TYPE_I_ALL 7
#define SLICE_TYPE_SP_ALL 8
#define SLICE_TYPE_SI_ALL 9


typedef struct parser_s {
    H264P_Config_t config;
    
    // NALU buffer
    uint8_t* pNaluBuf;
    uint8_t* pNaluBufCur;
    int naluBufSize;
    int naluBufManaged;
    unsigned int naluSize;      // in bytes
    unsigned int remNaluSize;   // in bytes
    
    // Bitstream cache
    uint32_t cache;
    int cacheLength;   // in bits
    int oldZeroCount;
    
    //TODO: value per PPS ID or SPS ID
    int pic_width_in_mbs_minus1;
    int pic_height_in_map_units_minus1;
    int slice_group_change_rate_minus1;
    int entropy_coding_mode_flag;
    int separate_colour_plane_flag;
    int frame_mbs_only_flag;
    int log2_max_frame_num_minus4;
    int pic_order_cnt_type;
    int log2_max_pic_order_cnt_lsb_minus4;
    int bottom_field_pic_order_in_frame_present_flag;
    int delta_pic_order_always_zero_flag;
    int initial_cpb_removal_delay_length_minus1;
    int cpb_removal_delay_length_minus1;
    int dpb_output_delay_length_minus1;
    int nal_hrd_parameters_present_flag;
    int vcl_hrd_parameters_present_flag;
    int cpb_cnt_minus1;
    int time_offset_length;
    int pic_struct_present_flag;
    int chroma_format_idc;
    int redundant_pic_cnt_present_flag;
    int weighted_pred_flag;
    int weighted_bipred_idc;
    int deblocking_filter_control_present_flag;
    int num_slice_groups_minus1;
    int slice_group_map_type;

    int idrPicFlag;
    int nal_ref_idc;
    int nal_unit_type;

    H264P_SliceInfo_t sliceInfo;

    // User data SEI
    uint8_t* pUserDataBuf;
    int userDataBufSize;
    int userDataSize;
} parser_t;


typedef int (*parseNaluType_func)(parser_t* parser);

static int parseSps(parser_t* parser);
static int parsePps(parser_t* parser);
static int parseSei(parser_t* parser);
static int parseAud(parser_t* parser);
static int parseFillerData(parser_t* parser);
static int parseSlice(parser_t* parser);


static parseNaluType_func parseNaluType[] = 
{
    NULL,
    parseSlice,
    NULL,
    NULL,
    NULL,
    parseSlice,
    parseSei,
    parseSps,
    parsePps,
    parseAud,
    NULL,
    NULL,
    parseFillerData,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};


static char *naluTypeStr[] =
{
    "undefined",
    "coded slice",
    "coded slice data partition A",
    "coded slice data partition B",
    "coded slice data partition C",
    "coded slice IDR",
    "SEI",
    "SPS",
    "PPS",
    "access unit delimiter",
    "end of sequence",
    "end of stream",
    "filler data",
    "SPS extension",
    "prefix NALU",
    "subset SPS",

    "reserved",
    "reserved",
    "reserved",

    "coded slice auxiliary coded picture",
    "coded slice extension",
    "coded slice extension - depth view",
    
    "reserved",
    "reserved",

    "unspecified",
    "unspecified",
    "unspecified",
    "unspecified",
    "unspecified",
    "unspecified",
    "unspecified",
    "unspecified"
};


static char *sliceTypeStr[] =
{
    "P",
    "B",
    "I",
    "SP",
    "SI",
    "P (all)",
    "B (all)",
    "I (all)",
    "SP (all)",
    "SI (all)"
};


static int picStructToNumClockTS[16] =
{ 1, 1, 1, 2, 2, 3, 3, 2, 3, 0, 0, 0, 0, 0, 0, 0};


static inline int bitstreamByteAlign(parser_t* _parser)
{
    int _align = 0;

    if (_parser->cacheLength & 7)
    {
        _align = _parser->cacheLength & 7;
        _parser->cache <<= _align;
        _parser->cacheLength -= _align;
    }

    return _align;
}


static inline int readBits(parser_t* _parser, unsigned int _numBits, uint32_t *_value, int _emulationPrevention)
{
    unsigned int _count, _remBits = _numBits;
    uint32_t _val = 0, _read32, _bitMask;
    uint8_t _read8;

    if (_parser->cacheLength < _numBits)
    {
        // Not enough bits in the cache

        // Get the cache remaining bits
        _remBits -= _parser->cacheLength;
        _val = (_parser->cache >> (32 - _parser->cacheLength)) << _remBits;

        // Read at most 4 bytes in the buffer
        _parser->cache = 0;
        _parser->cacheLength = 0;
        _count = 0;
        while ((_parser->remNaluSize) && (_count < 4))
        {
            _read8 = *(_parser->pNaluBufCur++);
            _parser->remNaluSize--;
            _parser->cache |= (_read8 << (24 - _parser->cacheLength));
            _parser->cacheLength += 8;
            _count++;
        }

        // Emulation prevention
        if (_emulationPrevention)
        {
            int _zeroCount = _parser->oldZeroCount;
            int _bitPos = 24; //32 - _parser->cacheLength;
            uint8_t _byteVal;
            uint32_t _cacheLeft, _cacheRight;

            while (_bitPos >= 32 - _parser->cacheLength)
            {
                _byteVal = ((_parser->cache >> _bitPos) & 0xFF);
                if ((_zeroCount == 2) && (_byteVal == 0x03))
                {
                    // Remove the 0x03 byte
                    _cacheLeft = (_bitPos < 24) ? (_parser->cache >> (_bitPos + 8)) << (_bitPos + 8) : 0;
                    _cacheRight = (_bitPos > 0) ? (_parser->cache << (32 - _bitPos)) >> (32 - _bitPos - 8) : 0;
                    _parser->cache = _cacheLeft | _cacheRight;
                    _parser->cacheLength -= 8;
                    _zeroCount = 0;
                }
                else if (_byteVal == 0x00)
                {
                    _zeroCount++;
                    _bitPos -= 8;
                }
                else
                {
                    _zeroCount = 0;
                    _bitPos -= 8;
                }
            }
            _parser->oldZeroCount = _zeroCount;
        }
    }

    // Get the bits from the cache and shift
    _val |= _parser->cache >> (32 - _remBits);
    _parser->cache <<= _remBits;
    _parser->cacheLength -= _remBits;

    _bitMask = (uint32_t)-1 >> (32 - _numBits);
    if (_value) *_value = _val & _bitMask;
    return _numBits;
}


static inline int readBits_expGolomb_ue(parser_t* _parser, uint32_t *_value, int _emulationPrevention)
{
    int _ret, _leadingZeroBits = -1;
    uint32_t _b, _val;

    for (_b = 0; !_b; _leadingZeroBits++)
    {
        _ret = readBits(_parser, 1, &_b, _emulationPrevention);
        if (_ret != 1) return -1;
    }

    _b = 0;
    if (_leadingZeroBits)
    {
        _ret = readBits(_parser, _leadingZeroBits, &_b, _emulationPrevention);
        if (_ret < 0) return -1;
    }

    _val = (1 << _leadingZeroBits) - 1 + _b;
    *_value = _val;
    return _leadingZeroBits * 2 + 1;
}


static inline int readBits_expGolomb_se(parser_t* _parser, int32_t *_value, int _emulationPrevention)
{
    int _ret, _leadingZeroBits = -1;
    uint32_t _b, _val;

    for (_b = 0; !_b; _leadingZeroBits++)
    {
        _ret = readBits(_parser, 1, &_b, _emulationPrevention);
        if (_ret != 1) return -1;
    }

    _b = 0;
    if (_leadingZeroBits)
    {
        _ret = readBits(_parser, _leadingZeroBits, &_b, _emulationPrevention);
        if (_ret < 0) return -1;
    }

    _val = (1 << _leadingZeroBits) - 1 + _b;
    *_value = (_val & 1) ? (((int32_t)_val + 1) / 2) : (-((int32_t)_val + 1) / 2);
    return _leadingZeroBits * 2 + 1;
}


static inline int seekToByte(parser_t* _parser, int _start, int _whence)
{
    int _ret, _pos;

    if (_whence == SEEK_CUR)
    {
        _pos = _parser->naluSize - _parser->remNaluSize - ((_parser->cacheLength + 7) / 8) + _start;
    }
    else if (_whence == SEEK_END)
    {
        _pos = _parser->naluSize - _start;
    }
    else //(_whence == SEEK_SET)
    {
        _pos = _start;
    }

    _parser->pNaluBufCur = _parser->pNaluBuf + _pos;
    _parser->remNaluSize = _parser->naluSize - _pos;

    // Reset the cache
    _parser->cache = 0;
    _parser->cacheLength = 0;
    _parser->oldZeroCount = 0; // NB: this value is wrong when emulation prevention is in use (inside NAL Units)

    return _pos;
}


static inline int skipBytes(parser_t* _parser, int _byteCount)
{
    int _ret, _i, _readBits = 0;
    uint32_t _val;

    for (_i = 0; _i < _byteCount; _i++)
    {
        _ret = readBits(_parser, 8, &_val, 1);
        if (_ret < 0)
        {
            fprintf(stderr, "error: failed to read from file\n");
            return _ret;
        }
        _readBits += _ret;
    }

    return _readBits / 8;
}


static inline int moreRbspData(parser_t* _parser)
{
    int _ret, _retval = 0;
    uint32_t _val;

    if (_parser->remNaluSize > 1)
    {
        // More than 1 byte remaining
        _retval = 1;
    }
    else if (_parser->remNaluSize == 1)
    {
        // Exactly 1 byte remaining
        _ret = readBits(_parser, 8, &_val, 1);
        if (_ret < 0)
        {
            fprintf(stderr, "error: failed to read from file\n");
            return 0;
        }
        if (_val != 0x80)
        {
            // Not only the RBSP trailing bits remain
            _retval = 1;
        }

        _ret = seekToByte(_parser, -1, SEEK_CUR);
        if (_ret < 0)
        {
            fprintf(stderr, "Error: seekToByte() failed (%d)\n", _ret);
            return 0;
        }
    }
    
    return _retval;
}


static int parseScalingList(parser_t* parser, int sizeOfScalingList)
{
    int ret, j;
    int _readBits = 0;
    int32_t val_se = 0;
    int lastScale = 8;
    int nextScale = 8;
    
    // scaling_list
    if (parser->config.printLogs) printf("---- scaling_list()\n");
    
    for (j = 0; j < sizeOfScalingList; j++)
    {
        if (nextScale != 0)
        {
            // delta_scale
            ret = readBits_expGolomb_se(parser, &val_se, 1);
            if (ret < 0)
            {
                fprintf(stderr, "error: failed to read from file\n");
                return ret;
            }
            _readBits += ret;
            if (parser->config.printLogs) printf("------ delta_scale = %d\n", val_se);
            nextScale = (lastScale + val_se + 256 ) & 0xFF;
        }
        lastScale = (nextScale == 0) ? lastScale : nextScale;
    }

    return _readBits;
}


static int parseHrdParams(parser_t* parser)
{
    int ret = 0;
    uint32_t val = 0;
    int _readBits = 0;
    int i;

    // hrd_parameters
    if (parser->config.printLogs) printf("------ hrd_parameters()\n");

    // cpb_cnt_minus1
    ret = readBits_expGolomb_ue(parser, &val, 1);
    if (ret < 0)
    {
        fprintf(stderr, "error: failed to read from file\n");
        return ret;
    }
    _readBits += ret;
    parser->cpb_cnt_minus1 = val;
    if (parser->config.printLogs) printf("-------- cpb_cnt_minus1 = %d\n", val);

    // bit_rate_scale
    ret = readBits(parser, 4, &val, 1);
    if (ret < 0)
    {
        fprintf(stderr, "error: failed to read from file\n");
        return ret;
    }
    _readBits += ret;
    if (parser->config.printLogs) printf("-------- bit_rate_scale = %d\n", val);

    // cpb_size_scale
    ret = readBits(parser, 4, &val, 1);
    if (ret < 0)
    {
        fprintf(stderr, "error: failed to read from file\n");
        return ret;
    }
    _readBits += ret;
    if (parser->config.printLogs) printf("-------- cpb_size_scale = %d\n", val);

    for (i = 0; i <= parser->cpb_cnt_minus1; i++)
    {
        // bit_rate_value_minus1
        ret = readBits_expGolomb_ue(parser, &val, 1);
        if (ret < 0)
        {
            fprintf(stderr, "error: failed to read from file\n");
            return ret;
        }
        _readBits += ret;
        if (parser->config.printLogs) printf("-------- bit_rate_value_minus1[%d] = %d\n", i, val);

        // cpb_size_value_minus1
        ret = readBits_expGolomb_ue(parser, &val, 1);
        if (ret < 0)
        {
            fprintf(stderr, "error: failed to read from file\n");
            return ret;
        }
        _readBits += ret;
        if (parser->config.printLogs) printf("-------- cpb_size_value_minus1[%d] = %d\n", i, val);

        // cbr_flag
        ret = readBits(parser, 1, &val, 1);
        if (ret < 0)
        {
            fprintf(stderr, "error: failed to read from file\n");
            return ret;
        }
        _readBits += ret;
        if (parser->config.printLogs) printf("-------- cbr_flag[%d] = %d\n", i, val);
    }

    // initial_cpb_removal_delay_length_minus1
    ret = readBits(parser, 5, &val, 1);
    if (ret < 0)
    {
        fprintf(stderr, "error: failed to read from file\n");
        return ret;
    }
    _readBits += ret;
    parser->initial_cpb_removal_delay_length_minus1 = val;
    if (parser->config.printLogs) printf("-------- initial_cpb_removal_delay_length_minus1 = %d\n", val);

    // cpb_removal_delay_length_minus1
    ret = readBits(parser, 5, &val, 1);
    if (ret < 0)
    {
        fprintf(stderr, "error: failed to read from file\n");
        return ret;
    }
    _readBits += ret;
    parser->cpb_removal_delay_length_minus1 = val;
    if (parser->config.printLogs) printf("-------- cpb_removal_delay_length_minus1 = %d\n", val);

    // dpb_output_delay_length_minus1
    ret = readBits(parser, 5, &val, 1);
    if (ret < 0)
    {
        fprintf(stderr, "error: failed to read from file\n");
        return ret;
    }
    _readBits += ret;
    parser->dpb_output_delay_length_minus1 = val;
    if (parser->config.printLogs) printf("-------- dpb_output_delay_length_minus1 = %d\n", val);

    // time_offset_length
    ret = readBits(parser, 5, &val, 1);
    if (ret < 0)
    {
        fprintf(stderr, "error: failed to read from file\n");
        return ret;
    }
    _readBits += ret;
    parser->time_offset_length = val;
    if (parser->config.printLogs) printf("-------- time_offset_length = %d\n", val);

    return _readBits;
}


static int parseVui(parser_t* parser)
{
    int ret = 0;
    uint32_t val = 0;
    int _readBits = 0;

    // vui_parameters
    if (parser->config.printLogs) printf("---- vui_parameters()\n");

    // aspect_ratio_info_present_flag
    ret = readBits(parser, 1, &val, 1);
    if (ret < 0)
    {
        fprintf(stderr, "error: failed to read from file\n");
        return ret;
    }
    _readBits += ret;
    if (parser->config.printLogs) printf("------ aspect_ratio_info_present_flag = %d\n", val);

    if (val)
    {
        // aspect_ratio_idc
        ret = readBits(parser, 8, &val, 1);
        if (ret < 0)
        {
            fprintf(stderr, "error: failed to read from file\n");
            return ret;
        }
        _readBits += ret;
        if (parser->config.printLogs) printf("------ aspect_ratio_idc = %d\n", val);

        if (val == 255)
        {
            // sar_width
            ret = readBits(parser, 16, &val, 1);
            if (ret < 0)
            {
                fprintf(stderr, "error: failed to read from file\n");
                return ret;
            }
            _readBits += ret;
            if (parser->config.printLogs) printf("------ sar_width = %d\n", val);

            // sar_height
            ret = readBits(parser, 16, &val, 1);
            if (ret < 0)
            {
                fprintf(stderr, "error: failed to read from file\n");
                return ret;
            }
            _readBits += ret;
            if (parser->config.printLogs) printf("------ sar_height = %d\n", val);
        }
    }

    // overscan_info_present_flag
    ret = readBits(parser, 1, &val, 1);
    if (ret < 0)
    {
        fprintf(stderr, "error: failed to read from file\n");
        return ret;
    }
    _readBits += ret;
    if (parser->config.printLogs) printf("------ overscan_info_present_flag = %d\n", val);

    if (val)
    {
        // overscan_appropriate_flag
        ret = readBits(parser, 1, &val, 1);
        if (ret < 0)
        {
            fprintf(stderr, "error: failed to read from file\n");
            return ret;
        }
        _readBits += ret;
        if (parser->config.printLogs) printf("------ overscan_appropriate_flag = %d\n", val);
    }

    // video_signal_type_present_flag
    ret = readBits(parser, 1, &val, 1);
    if (ret < 0)
    {
        fprintf(stderr, "error: failed to read from file\n");
        return ret;
    }
    _readBits += ret;
    if (parser->config.printLogs) printf("------ video_signal_type_present_flag = %d\n", val);

    if (val)
    {
        // video_format
        ret = readBits(parser, 3, &val, 1);
        if (ret < 0)
        {
            fprintf(stderr, "error: failed to read from file\n");
            return ret;
        }
        _readBits += ret;
        if (parser->config.printLogs) printf("------ video_format = %d\n", val);

        // video_full_range_flag
        ret = readBits(parser, 1, &val, 1);
        if (ret < 0)
        {
            fprintf(stderr, "error: failed to read from file\n");
            return ret;
        }
        _readBits += ret;
        if (parser->config.printLogs) printf("------ video_full_range_flag = %d\n", val);

        // colour_description_present_flag
        ret = readBits(parser, 1, &val, 1);
        if (ret < 0)
        {
            fprintf(stderr, "error: failed to read from file\n");
            return ret;
        }
        _readBits += ret;
        if (parser->config.printLogs) printf("------ colour_description_present_flag = %d\n", val);
        
        if (val)
        {
            // colour_primaries
            ret = readBits(parser, 8, &val, 1);
            if (ret < 0)
            {
                fprintf(stderr, "error: failed to read from file\n");
                return ret;
            }
            _readBits += ret;
            if (parser->config.printLogs) printf("------ colour_primaries = %d\n", val);

            // transfer_characteristics
            ret = readBits(parser, 8, &val, 1);
            if (ret < 0)
            {
                fprintf(stderr, "error: failed to read from file\n");
                return ret;
            }
            _readBits += ret;
            if (parser->config.printLogs) printf("------ transfer_characteristics = %d\n", val);

            // matrix_coefficients
            ret = readBits(parser, 8, &val, 1);
            if (ret < 0)
            {
                fprintf(stderr, "error: failed to read from file\n");
                return ret;
            }
            _readBits += ret;
            if (parser->config.printLogs) printf("------ matrix_coefficients = %d\n", val);

        }
    }

    // chroma_loc_info_present_flag
    ret = readBits(parser, 1, &val, 1);
    if (ret < 0)
    {
        fprintf(stderr, "error: failed to read from file\n");
        return ret;
    }
    _readBits += ret;
    if (parser->config.printLogs) printf("------ chroma_loc_info_present_flag = %d\n", val);
    
    if (val)
    {
        // chroma_sample_loc_type_top_field
        ret = readBits_expGolomb_ue(parser, &val, 1);
        if (ret < 0)
        {
            fprintf(stderr, "error: failed to read from file\n");
            return ret;
        }
        _readBits += ret;
        if (parser->config.printLogs) printf("------ chroma_sample_loc_type_top_field = %d\n", val);

        // chroma_sample_loc_type_bottom_field
        ret = readBits_expGolomb_ue(parser, &val, 1);
        if (ret < 0)
        {
            fprintf(stderr, "error: failed to read from file\n");
            return ret;
        }
        _readBits += ret;
        if (parser->config.printLogs) printf("------ chroma_sample_loc_type_bottom_field = %d\n", val);
    }

    // timing_info_present_flag
    ret = readBits(parser, 1, &val, 1);
    if (ret < 0)
    {
        fprintf(stderr, "error: failed to read from file\n");
        return ret;
    }
    _readBits += ret;
    if (parser->config.printLogs) printf("------ timing_info_present_flag = %d\n", val);
    
    if (val)
    {
        // num_units_in_tick
        ret = readBits(parser, 32, &val, 1);
        if (ret < 0)
        {
            fprintf(stderr, "error: failed to read from file\n");
            return ret;
        }
        _readBits += ret;
        if (parser->config.printLogs) printf("------ num_units_in_tick = %d\n", val);
        
        // time_scale
        ret = readBits(parser, 32, &val, 1);
        if (ret < 0)
        {
            fprintf(stderr, "error: failed to read from file\n");
            return ret;
        }
        _readBits += ret;
        if (parser->config.printLogs) printf("------ time_scale = %d\n", val);
        
        // fixed_frame_rate_flag
        ret = readBits(parser, 1, &val, 1);
        if (ret < 0)
        {
            fprintf(stderr, "error: failed to read from file\n");
            return ret;
        }
        _readBits += ret;
        if (parser->config.printLogs) printf("------ fixed_frame_rate_flag = %d\n", val);
    }

    // nal_hrd_parameters_present_flag
    ret = readBits(parser, 1, &val, 1);
    if (ret < 0)
    {
        fprintf(stderr, "error: failed to read from file\n");
        return ret;
    }
    _readBits += ret;
    parser->nal_hrd_parameters_present_flag = val;
    if (parser->config.printLogs) printf("------ nal_hrd_parameters_present_flag = %d\n", val);
    
    if (parser->nal_hrd_parameters_present_flag)
    {
        // hrd_parameters
        ret = parseHrdParams(parser);
        if (ret < 0)
        {
            fprintf(stderr, "error: parseHrdParams() failed (%d)\n", ret);
            return ret;
        }
        _readBits += ret;
    }

    // vcl_hrd_parameters_present_flag
    ret = readBits(parser, 1, &val, 1);
    if (ret < 0)
    {
        fprintf(stderr, "error: failed to read from file\n");
        return ret;
    }
    _readBits += ret;
    parser->vcl_hrd_parameters_present_flag = val;
    if (parser->config.printLogs) printf("------ vcl_hrd_parameters_present_flag = %d\n", val);
    
    if (parser->vcl_hrd_parameters_present_flag)
    {
        // hrd_parameters
        ret = parseHrdParams(parser);
        if (ret < 0)
        {
            fprintf(stderr, "error: parseHrdParams() failed (%d)\n", ret);
            return ret;
        }
        _readBits += ret;
    }

    if (parser->nal_hrd_parameters_present_flag || parser->vcl_hrd_parameters_present_flag)
    {
        // low_delay_hrd_flag
        ret = readBits(parser, 1, &val, 1);
        if (ret < 0)
        {
            fprintf(stderr, "error: failed to read from file\n");
            return ret;
        }
        _readBits += ret;
        if (parser->config.printLogs) printf("------ low_delay_hrd_flag = %d\n", val);
    }

    // pic_struct_present_flag
    ret = readBits(parser, 1, &val, 1);
    if (ret < 0)
    {
        fprintf(stderr, "error: failed to read from file\n");
        return ret;
    }
    _readBits += ret;
    parser->pic_struct_present_flag = val;
    if (parser->config.printLogs) printf("------ pic_struct_present_flag = %d\n", val);
    
    // bitstream_restriction_flag
    ret = readBits(parser, 1, &val, 1);
    if (ret < 0)
    {
        fprintf(stderr, "error: failed to read from file\n");
        return ret;
    }
    _readBits += ret;
    if (parser->config.printLogs) printf("------ bitstream_restriction_flag = %d\n", val);
    
    if (val)
    {
        // motion_vectors_over_pic_boundaries_flag
        ret = readBits(parser, 1, &val, 1);
        if (ret < 0)
        {
            fprintf(stderr, "error: failed to read from file\n");
            return ret;
        }
        _readBits += ret;
        if (parser->config.printLogs) printf("------ motion_vectors_over_pic_boundaries_flag = %d\n", val);

        // max_bytes_per_pic_denom
        ret = readBits_expGolomb_ue(parser, &val, 1);
        if (ret < 0)
        {
            fprintf(stderr, "error: failed to read from file\n");
            return ret;
        }
        _readBits += ret;
        if (parser->config.printLogs) printf("------ max_bytes_per_pic_denom = %d\n", val);

        // max_bits_per_mb_denom
        ret = readBits_expGolomb_ue(parser, &val, 1);
        if (ret < 0)
        {
            fprintf(stderr, "error: failed to read from file\n");
            return ret;
        }
        _readBits += ret;
        if (parser->config.printLogs) printf("------ max_bits_per_mb_denom = %d\n", val);

        // log2_max_mv_length_horizontal
        ret = readBits_expGolomb_ue(parser, &val, 1);
        if (ret < 0)
        {
            fprintf(stderr, "error: failed to read from file\n");
            return ret;
        }
        _readBits += ret;
        if (parser->config.printLogs) printf("------ log2_max_mv_length_horizontal = %d\n", val);

        // log2_max_mv_length_vertical
        ret = readBits_expGolomb_ue(parser, &val, 1);
        if (ret < 0)
        {
            fprintf(stderr, "error: failed to read from file\n");
            return ret;
        }
        _readBits += ret;
        if (parser->config.printLogs) printf("------ log2_max_mv_length_vertical = %d\n", val);

        // max_num_reorder_frames
        ret = readBits_expGolomb_ue(parser, &val, 1);
        if (ret < 0)
        {
            fprintf(stderr, "error: failed to read from file\n");
            return ret;
        }
        _readBits += ret;
        if (parser->config.printLogs) printf("------ max_num_reorder_frames = %d\n", val);

        // max_dec_frame_buffering
        ret = readBits_expGolomb_ue(parser, &val, 1);
        if (ret < 0)
        {
            fprintf(stderr, "error: failed to read from file\n");
            return ret;
        }
        _readBits += ret;
        if (parser->config.printLogs) printf("------ max_dec_frame_buffering = %d\n", val);
    }

    return _readBits;
}


static int parseSps(parser_t* parser)
{
    int ret = 0;
    uint32_t val = 0;
    int32_t val_se = 0;
    int readBytes = 0, _readBits = 0;
    int i, profile_idc, num_ref_frames_in_pic_order_cnt_cycle, width, height;

    // seq_parameter_set_rbsp
    if (parser->config.printLogs) printf("-- seq_parameter_set_rbsp()\n");
    
    // seq_parameter_set_data

    ret = readBits(parser, 24, &val, 1);
    if (ret < 0)
    {
        fprintf(stderr, "error: failed to read from file\n");
        return ret;
    }
    readBytes += 3;
    profile_idc = val >> 16;
    
    // profile_idc
    if (parser->config.printLogs) printf("---- profile_idc = %d\n", profile_idc);
    
    // constraint_set0_flag
    if (parser->config.printLogs) printf("---- constraint_set0_flag = %d\n", (val >> 15) & 1);
    
    // constraint_set1_flag
    if (parser->config.printLogs) printf("---- constraint_set1_flag = %d\n", (val >> 14) & 1);
    
    // constraint_set2_flag
    if (parser->config.printLogs) printf("---- constraint_set2_flag = %d\n", (val >> 13) & 1);
    
    // constraint_set3_flag
    if (parser->config.printLogs) printf("---- constraint_set3_flag = %d\n", (val >> 12) & 1);
    
    // constraint_set4_flag
    if (parser->config.printLogs) printf("---- constraint_set4_flag = %d\n", (val >> 11) & 1);
    
    // constraint_set5_flag
    if (parser->config.printLogs) printf("---- constraint_set5_flag = %d\n", (val >> 10) & 1);
    
    // reserved_zero_2bits
    if (parser->config.printLogs) printf("---- reserved_zero_2bits = %d%d\n", (val >> 9) & 1, (val >> 8) & 1);
    
    // level_idc
    if (parser->config.printLogs) printf("---- level_idc = %d\n", val & 0xFF);

    // seq_parameter_set_id
    ret = readBits_expGolomb_ue(parser, &val, 1);
    if (ret < 0)
    {
        fprintf(stderr, "error: failed to read from file\n");
        return ret;
    }
    _readBits += ret;
    if (parser->config.printLogs) printf("---- seq_parameter_set_id = %d\n", val);

    if (profile_idc == 100 || profile_idc == 110 || profile_idc == 122 || profile_idc == 244 
            || profile_idc == 44 || profile_idc == 83 || profile_idc == 86 
            || profile_idc == 118 || profile_idc == 128 || profile_idc == 138)
    {
        // chroma_format_idc
        ret = readBits_expGolomb_ue(parser, &val, 1);
        if (ret < 0)
        {
            fprintf(stderr, "error: failed to read from file\n");
            return ret;
        }
        _readBits += ret;
        parser->chroma_format_idc = val;
        if (parser->config.printLogs) printf("---- chroma_format_idc = %d\n", val);

        if (val == 3)
        {
            // separate_colour_plane_flag
            ret = readBits(parser, 1, &val, 1);
            if (ret < 0)
            {
                fprintf(stderr, "error: failed to read from file\n");
                return ret;
            }
            _readBits += ret;
            parser->separate_colour_plane_flag = val;
            if (parser->config.printLogs) printf("---- separate_colour_plane_flag = %d\n", val);
        }

        // bit_depth_luma_minus8
        ret = readBits_expGolomb_ue(parser, &val, 1);
        if (ret < 0)
        {
            fprintf(stderr, "error: failed to read from file\n");
            return ret;
        }
        _readBits += ret;
        if (parser->config.printLogs) printf("---- bit_depth_luma_minus8 = %d\n", val);

        // bit_depth_chroma_minus8
        ret = readBits_expGolomb_ue(parser, &val, 1);
        if (ret < 0)
        {
            fprintf(stderr, "error: failed to read from file\n");
            return ret;
        }
        _readBits += ret;
        if (parser->config.printLogs) printf("---- bit_depth_chroma_minus8 = %d\n", val);

        // qpprime_y_zero_transform_bypass_flag
        ret = readBits(parser, 1, &val, 1);
        if (ret < 0)
        {
            fprintf(stderr, "error: failed to read from file\n");
            return ret;
        }
        _readBits += ret;
        if (parser->config.printLogs) printf("---- qpprime_y_zero_transform_bypass_flag = %d\n", val);

        // seq_scaling_matrix_present_flag
        ret = readBits(parser, 1, &val, 1);
        if (ret < 0)
        {
            fprintf(stderr, "error: failed to read from file\n");
            return ret;
        }
        _readBits += ret;
        if (parser->config.printLogs) printf("---- seq_scaling_matrix_present_flag = %d\n", val);

        if (val)
        {
            for (i = 0; i < ((parser->chroma_format_idc != 3) ? 8 : 12); i++)
            {
                // seq_scaling_list_present_flag
                ret = readBits(parser, 1, &val, 1);
                if (ret < 0)
                {
                    fprintf(stderr, "error: failed to read from file\n");
                    return ret;
                }
                _readBits += ret;
                if (parser->config.printLogs) printf("---- seq_scaling_list_present_flag[%d] = %d\n", i, val);

                if (val)
                {
                    ret = parseScalingList(parser, (i < 6) ? 16 : 64);
                    if (ret < 0)
                    {
                        fprintf(stderr, "error: parseScalingList() failed (%d)\n", ret);
                        return ret;
                    }
                    _readBits += ret;
                }
            }
        }
    }

    // log2_max_frame_num_minus4
    ret = readBits_expGolomb_ue(parser, &val, 1);
    if (ret < 0)
    {
        fprintf(stderr, "error: failed to read from file\n");
        return ret;
    }
    _readBits += ret;
    parser->log2_max_frame_num_minus4 = val;
    if (parser->config.printLogs) printf("---- log2_max_frame_num_minus4 = %d\n", val);

    // pic_order_cnt_type
    ret = readBits_expGolomb_ue(parser, &val, 1);
    if (ret < 0)
    {
        fprintf(stderr, "error: failed to read from file\n");
        return ret;
    }
    _readBits += ret;
    parser->pic_order_cnt_type = val;
    if (parser->config.printLogs) printf("---- pic_order_cnt_type = %d\n", val);

    if (parser->pic_order_cnt_type == 0)
    {
        // log2_max_pic_order_cnt_lsb_minus4
        ret = readBits_expGolomb_ue(parser, &val, 1);
        if (ret < 0)
        {
            fprintf(stderr, "error: failed to read from file\n");
            return ret;
        }
        _readBits += ret;
        parser->log2_max_pic_order_cnt_lsb_minus4 = val;
        if (parser->config.printLogs) printf("---- log2_max_pic_order_cnt_lsb_minus4 = %d\n", val);
    }
    else if (parser->pic_order_cnt_type == 1)
    {
        // delta_pic_order_always_zero_flag
        ret = readBits(parser, 1, &val, 1);
        if (ret < 0)
        {
            fprintf(stderr, "error: failed to read from file\n");
            return ret;
        }
        _readBits += ret;
        parser->delta_pic_order_always_zero_flag = val;
        if (parser->config.printLogs) printf("---- delta_pic_order_always_zero_flag = %d\n", val);

        // offset_for_non_ref_pic
        ret = readBits_expGolomb_se(parser, &val_se, 1);
        if (ret < 0)
        {
            fprintf(stderr, "error: failed to read from file\n");
            return ret;
        }
        _readBits += ret;
        if (parser->config.printLogs) printf("---- offset_for_non_ref_pic = %d\n", val_se);

        // offset_for_top_to_bottom_field
        ret = readBits_expGolomb_se(parser, &val_se, 1);
        if (ret < 0)
        {
            fprintf(stderr, "error: failed to read from file\n");
            return ret;
        }
        _readBits += ret;
        if (parser->config.printLogs) printf("---- offset_for_top_to_bottom_field = %d\n", val_se);

        // num_ref_frames_in_pic_order_cnt_cycle
        ret = readBits_expGolomb_ue(parser, &val, 1);
        if (ret < 0)
        {
            fprintf(stderr, "error: failed to read from file\n");
            return ret;
        }
        _readBits += ret;
        num_ref_frames_in_pic_order_cnt_cycle = val;
        if (parser->config.printLogs) printf("---- num_ref_frames_in_pic_order_cnt_cycle = %d\n", num_ref_frames_in_pic_order_cnt_cycle);

        for (i = 0; i < num_ref_frames_in_pic_order_cnt_cycle; i++)
        {
            // offset_for_ref_frame
            ret = readBits_expGolomb_se(parser, &val_se, 1);
            if (ret < 0)
            {
                fprintf(stderr, "error: failed to read from file\n");
                return ret;
            }
            _readBits += ret;
            if (parser->config.printLogs) printf("---- offset_for_ref_frame[%d] = %d\n", i, val_se);
        }
    }

    // max_num_ref_frames
    ret = readBits_expGolomb_ue(parser, &val, 1);
    if (ret < 0)
    {
        fprintf(stderr, "error: failed to read from file\n");
        return ret;
    }
    _readBits += ret;
    if (parser->config.printLogs) printf("---- max_num_ref_frames = %d\n", val);

    // gaps_in_frame_num_value_allowed_flag
    ret = readBits(parser, 1, &val, 1);
    if (ret < 0)
    {
        fprintf(stderr, "error: failed to read from file\n");
        return ret;
    }
    _readBits += ret;
    if (parser->config.printLogs) printf("---- gaps_in_frame_num_value_allowed_flag = %d\n", val);

    // pic_width_in_mbs_minus1
    ret = readBits_expGolomb_ue(parser, &val, 1);
    if (ret < 0)
    {
        fprintf(stderr, "error: failed to read from file\n");
        return ret;
    }
    _readBits += ret;
    parser->pic_width_in_mbs_minus1 = val;
    width = (parser->pic_width_in_mbs_minus1 + 1) * 16;
    if (parser->config.printLogs) printf("---- pic_width_in_mbs_minus1 = %d (width = %d pixels)\n", val, width);

    // pic_height_in_map_units_minus1
    ret = readBits_expGolomb_ue(parser, &val, 1);
    if (ret < 0)
    {
        fprintf(stderr, "error: failed to read from file\n");
        return ret;
    }
    _readBits += ret;
    parser->pic_height_in_map_units_minus1 = val;
    height = (parser->pic_height_in_map_units_minus1 + 1) * 16;
    if (parser->config.printLogs) printf("---- pic_height_in_map_units_minus1 = %d\n", val);

    // frame_mbs_only_flag
    ret = readBits(parser, 1, &val, 1);
    if (ret < 0)
    {
        fprintf(stderr, "error: failed to read from file\n");
        return ret;
    }
    _readBits += ret;
    parser->frame_mbs_only_flag = val;
    if (!parser->frame_mbs_only_flag) height *= 2;
    if (parser->config.printLogs) printf("---- frame_mbs_only_flag = %d (height = %d pixels)\n", val, height);

    if (!val)
    {
        // mb_adaptive_frame_field_flag
        ret = readBits(parser, 1, &val, 1);
        if (ret < 0)
        {
            fprintf(stderr, "error: failed to read from file\n");
            return ret;
        }
        _readBits += ret;
        if (parser->config.printLogs) printf("---- mb_adaptive_frame_field_flag = %d\n", val);
    }

    // direct_8x8_inference_flag
    ret = readBits(parser, 1, &val, 1);
    if (ret < 0)
    {
        fprintf(stderr, "error: failed to read from file\n");
        return ret;
    }
    _readBits += ret;
    if (parser->config.printLogs) printf("---- direct_8x8_inference_flag = %d\n", val);

    // frame_cropping_flag
    ret = readBits(parser, 1, &val, 1);
    if (ret < 0)
    {
        fprintf(stderr, "error: failed to read from file\n");
        return ret;
    }
    _readBits += ret;
    if (parser->config.printLogs) printf("---- frame_cropping_flag = %d\n", val);

    if (val)
    {
        // frame_crop_left_offset
        ret = readBits_expGolomb_ue(parser, &val, 1);
        if (ret < 0)
        {
            fprintf(stderr, "error: failed to read from file\n");
            return ret;
        }
        _readBits += ret;
        if (parser->config.printLogs) printf("---- frame_crop_left_offset = %d\n", val);

        // frame_crop_right_offset
        ret = readBits_expGolomb_ue(parser, &val, 1);
        if (ret < 0)
        {
            fprintf(stderr, "error: failed to read from file\n");
            return ret;
        }
        _readBits += ret;
        if (parser->config.printLogs) printf("---- frame_crop_right_offset = %d\n", val);

        // frame_crop_top_offset
        ret = readBits_expGolomb_ue(parser, &val, 1);
        if (ret < 0)
        {
            fprintf(stderr, "error: failed to read from file\n");
            return ret;
        }
        _readBits += ret;
        if (parser->config.printLogs) printf("---- frame_crop_top_offset = %d\n", val);

        // frame_crop_bottom_offset
        ret = readBits_expGolomb_ue(parser, &val, 1);
        if (ret < 0)
        {
            fprintf(stderr, "error: failed to read from file\n");
            return ret;
        }
        _readBits += ret;
        if (parser->config.printLogs) printf("---- frame_crop_bottom_offset = %d\n", val);
    }

    // vui_parameters_present_flag
    ret = readBits(parser, 1, &val, 1);
    if (ret < 0)
    {
        fprintf(stderr, "error: failed to read from file\n");
        return ret;
    }
    _readBits += ret;
    if (parser->config.printLogs) printf("---- vui_parameters_present_flag = %d\n", val);

    if (val)
    {
        // vui_parameters
        ret = parseVui(parser);
        if (ret < 0)
        {
            fprintf(stderr, "error: parseVui() failed (%d)\n", ret);
            return ret;
        }
        _readBits += ret;
    }

    // rbsp_trailing_bits
    ret = readBits(parser, 1, &val, 1);
    if (ret < 0)
    {
        fprintf(stderr, "error: failed to read from file\n");
        return ret;
    }
    _readBits += ret;

    ret = bitstreamByteAlign(parser);
    if (ret < 0)
    {
        fprintf(stderr, "error: failed to align the bitstream (%d)\n", ret);
        return ret;
    }
    _readBits += ret;
    readBytes += _readBits / 8;

    return readBytes;
}


static int parsePps(parser_t* parser)
{
    int ret = 0;
    uint32_t val = 0;
    int32_t val_se = 0;
    int readBytes = 0, _readBits = 0;
    int i, len, pic_size_in_map_units_minus1, transform_8x8_mode_flag;

    // pic_parameter_set_rbsp
    if (parser->config.printLogs) printf("-- pic_parameter_set_rbsp()\n");

    // pic_parameter_set_id
    ret = readBits_expGolomb_ue(parser, &val, 1);
    if (ret < 0)
    {
        fprintf(stderr, "error: failed to read from file\n");
        return ret;
    }
    _readBits += ret;
    if (parser->config.printLogs) printf("---- pic_parameter_set_id = %d\n", val);

    // seq_parameter_set_id
    ret = readBits_expGolomb_ue(parser, &val, 1);
    if (ret < 0)
    {
        fprintf(stderr, "error: failed to read from file\n");
        return ret;
    }
    _readBits += ret;
    if (parser->config.printLogs) printf("---- seq_parameter_set_id = %d\n", val);

    // entropy_coding_mode_flag
    ret = readBits(parser, 1, &val, 1);
    if (ret < 0)
    {
        fprintf(stderr, "error: failed to read from file\n");
        return ret;
    }
    _readBits += ret;
    parser->entropy_coding_mode_flag = val;
    if (parser->config.printLogs) printf("---- entropy_coding_mode_flag = %d\n", val);

    // bottom_field_pic_order_in_frame_present_flag
    ret = readBits(parser, 1, &val, 1);
    if (ret < 0)
    {
        fprintf(stderr, "error: failed to read from file\n");
        return ret;
    }
    _readBits += ret;
    parser->bottom_field_pic_order_in_frame_present_flag = val;
    if (parser->config.printLogs) printf("---- bottom_field_pic_order_in_frame_present_flag = %d\n", val);

    // num_slice_groups_minus1
    ret = readBits_expGolomb_ue(parser, &val, 1);
    if (ret < 0)
    {
        fprintf(stderr, "error: failed to read from file\n");
        return ret;
    }
    _readBits += ret;
    parser->num_slice_groups_minus1 = val;
    if (parser->config.printLogs) printf("---- num_slice_groups_minus1 = %d\n", val);

    if (parser->num_slice_groups_minus1 > 0)
    {
        // slice_group_map_type
        ret = readBits_expGolomb_ue(parser, &val, 1);
        if (ret < 0)
        {
            fprintf(stderr, "error: failed to read from file\n");
            return ret;
        }
        _readBits += ret;
        parser->slice_group_map_type = val;
        if (parser->config.printLogs) printf("---- slice_group_map_type = %d\n", val);
        
        if (parser->slice_group_map_type == 0)
        {
            for (i = 0; i <= parser->num_slice_groups_minus1; i++)
            {
                // run_length_minus1[i]
                ret = readBits_expGolomb_ue(parser, &val, 1);
                if (ret < 0)
                {
                    fprintf(stderr, "error: failed to read from file\n");
                    return ret;
                }
                _readBits += ret;
                if (parser->config.printLogs) printf("---- run_length_minus1[%d] = %d\n", i, val);
            }
        }
        else if (parser->slice_group_map_type == 2)
        {
            for (i = 0; i < parser->num_slice_groups_minus1; i++)
            {
                // top_left[i]
                ret = readBits_expGolomb_ue(parser, &val, 1);
                if (ret < 0)
                {
                    fprintf(stderr, "error: failed to read from file\n");
                    return ret;
                }
                _readBits += ret;
                if (parser->config.printLogs) printf("---- top_left[%d] = %d\n", i, val);

                // bottom_right[i]
                ret = readBits_expGolomb_ue(parser, &val, 1);
                if (ret < 0)
                {
                    fprintf(stderr, "error: failed to read from file\n");
                    return ret;
                }
                _readBits += ret;
                if (parser->config.printLogs) printf("---- bottom_right[%d] = %d\n", i, val);
            }
        }
        else if ((parser->slice_group_map_type == 3) || (parser->slice_group_map_type == 4) || (parser->slice_group_map_type == 5))
        {
            // slice_group_change_direction_flag
            ret = readBits(parser, 1, &val, 1);
            if (ret < 0)
            {
                fprintf(stderr, "error: failed to read from file\n");
                return ret;
            }
            _readBits += ret;
            if (parser->config.printLogs) printf("---- slice_group_change_direction_flag = %d\n", val);

            // slice_group_change_rate_minus1
            ret = readBits_expGolomb_ue(parser, &val, 1);
            if (ret < 0)
            {
                fprintf(stderr, "error: failed to read from file\n");
                return ret;
            }
            _readBits += ret;
            parser->slice_group_change_rate_minus1 = val;
            if (parser->config.printLogs) printf("---- slice_group_change_rate_minus1 = %d\n", val);
        }
        else if (parser->slice_group_map_type == 6)
        {
            // pic_size_in_map_units_minus1
            ret = readBits_expGolomb_ue(parser, &val, 1);
            if (ret < 0)
            {
                fprintf(stderr, "error: failed to read from file\n");
                return ret;
            }
            _readBits += ret;
            pic_size_in_map_units_minus1 = val;
            if (parser->config.printLogs) printf("---- pic_size_in_map_units_minus1 = %d\n", val);

            for (i = 0; i <= pic_size_in_map_units_minus1; i++)
            {
                // slice_group_id[i]
                len = (int)ceil(log2(parser->num_slice_groups_minus1 + 1));
                ret = readBits(parser, len, &val, 1);
                if (ret < 0)
                {
                    fprintf(stderr, "error: failed to read from file\n");
                    return ret;
                }
                _readBits += ret;
                if (parser->config.printLogs) printf("---- slice_group_id[%d] = %d\n", i, val);
            }
        }
    }

    // num_ref_idx_l0_default_active_minus1
    ret = readBits_expGolomb_ue(parser, &val, 1);
    if (ret < 0)
    {
        fprintf(stderr, "error: failed to read from file\n");
        return ret;
    }
    _readBits += ret;
    if (parser->config.printLogs) printf("---- num_ref_idx_l0_default_active_minus1 = %d\n", val);

    // num_ref_idx_l1_default_active_minus1
    ret = readBits_expGolomb_ue(parser, &val, 1);
    if (ret < 0)
    {
        fprintf(stderr, "error: failed to read from file\n");
        return ret;
    }
    _readBits += ret;
    if (parser->config.printLogs) printf("---- num_ref_idx_l1_default_active_minus1 = %d\n", val);

    // weighted_pred_flag
    ret = readBits(parser, 1, &val, 1);
    if (ret < 0)
    {
        fprintf(stderr, "error: failed to read from file\n");
        return ret;
    }
    _readBits += ret;
    parser->weighted_pred_flag = val;
    if (parser->config.printLogs) printf("---- weighted_pred_flag = %d\n", val);

    // weighted_bipred_idc
    ret = readBits(parser, 2, &val, 1);
    if (ret < 0)
    {
        fprintf(stderr, "error: failed to read from file\n");
        return ret;
    }
    _readBits += ret;
    parser->weighted_bipred_idc = val;
    if (parser->config.printLogs) printf("---- weighted_bipred_idc = %d\n", val);

    // pic_init_qp_minus26
    ret = readBits_expGolomb_se(parser, &val_se, 1);
    if (ret < 0)
    {
        fprintf(stderr, "error: failed to read from file\n");
        return ret;
    }
    _readBits += ret;
    if (parser->config.printLogs) printf("---- pic_init_qp_minus26 = %d\n", val);

    // pic_init_qs_minus26
    ret = readBits_expGolomb_se(parser, &val_se, 1);
    if (ret < 0)
    {
        fprintf(stderr, "error: failed to read from file\n");
        return ret;
    }
    _readBits += ret;
    if (parser->config.printLogs) printf("---- pic_init_qs_minus26 = %d\n", val);

    // chroma_qp_index_offset
    ret = readBits_expGolomb_se(parser, &val_se, 1);
    if (ret < 0)
    {
        fprintf(stderr, "error: failed to read from file\n");
        return ret;
    }
    _readBits += ret;
    if (parser->config.printLogs) printf("---- chroma_qp_index_offset = %d\n", val);

    // deblocking_filter_control_present_flag
    ret = readBits(parser, 1, &val, 1);
    if (ret < 0)
    {
        fprintf(stderr, "error: failed to read from file\n");
        return ret;
    }
    _readBits += ret;
    parser->deblocking_filter_control_present_flag = val;
    if (parser->config.printLogs) printf("---- deblocking_filter_control_present_flag = %d\n", val);

    // constrained_intra_pred_flag
    ret = readBits(parser, 1, &val, 1);
    if (ret < 0)
    {
        fprintf(stderr, "error: failed to read from file\n");
        return ret;
    }
    _readBits += ret;
    if (parser->config.printLogs) printf("---- constrained_intra_pred_flag = %d\n", val);

    // redundant_pic_cnt_present_flag
    ret = readBits(parser, 1, &val, 1);
    if (ret < 0)
    {
        fprintf(stderr, "error: failed to read from file\n");
        return ret;
    }
    _readBits += ret;
    parser->redundant_pic_cnt_present_flag = val;
    if (parser->config.printLogs) printf("---- redundant_pic_cnt_present_flag = %d\n", val);


    if (moreRbspData(parser))
    {
        // transform_8x8_mode_flag
        ret = readBits(parser, 1, &val, 1);
        if (ret < 0)
        {
            fprintf(stderr, "error: failed to read from file\n");
            return ret;
        }
        _readBits += ret;
        transform_8x8_mode_flag = val;
        if (parser->config.printLogs) printf("---- transform_8x8_mode_flag = %d\n", val);

        // pic_scaling_matrix_present_flag
        ret = readBits(parser, 1, &val, 1);
        if (ret < 0)
        {
            fprintf(stderr, "error: failed to read from file\n");
            return ret;
        }
        _readBits += ret;
        if (parser->config.printLogs) printf("---- pic_scaling_matrix_present_flag = %d\n", val);

        if (val)
        {
            for (i = 0; i < 6 + ((parser->chroma_format_idc != 3) ? 2 : 6) * transform_8x8_mode_flag; i++)
            {
                // pic_scaling_list_present_flag[i]
                ret = readBits(parser, 1, &val, 1);
                if (ret < 0)
                {
                    fprintf(stderr, "error: failed to read from file\n");
                    return ret;
                }
                _readBits += ret;
                if (parser->config.printLogs) printf("---- pic_scaling_list_present_flag[%d] = %d\n", i, val);

                if (val)
                {
                    ret = parseScalingList(parser, (i < 6) ? 16 : 64);
                    if (ret < 0)
                    {
                        fprintf(stderr, "error: parseScalingList() failed (%d)\n", ret);
                        return ret;
                    }
                    _readBits += ret;
                }
            }
        }

        // second_chroma_qp_index_offset
        ret = readBits_expGolomb_se(parser, &val_se, 1);
        if (ret < 0)
        {
            fprintf(stderr, "error: failed to read from file\n");
            return ret;
        }
        _readBits += ret;
        if (parser->config.printLogs) printf("---- second_chroma_qp_index_offset = %d\n", val);
    }


    // rbsp_trailing_bits
    ret = readBits(parser, 1, &val, 1);
    if (ret < 0)
    {
        fprintf(stderr, "error: failed to read from file\n");
        return ret;
    }
    _readBits += ret;

    ret = bitstreamByteAlign(parser);
    if (ret < 0)
    {
        fprintf(stderr, "error: failed to align the bitstream (%d)\n", ret);
        return ret;
    }
    _readBits += ret;
    readBytes += _readBits / 8;

    return readBytes;
}


static int parseSeiPayload_userDataUnregistered(parser_t* parser, int payloadSize)
{
    int ret = 0, i;
    uint32_t val = 0;
    int _readBits = 0;
    uint32_t uuid1, uuid2, uuid3, uuid4;

    if (parser->config.printLogs) printf("---- SEI: user_data_unregistered\n");

    if (payloadSize < 16)
    {
        if (parser->config.printLogs) printf("---- wrong size for user_data_unregistered (%d < 16)\n", payloadSize);
        return _readBits;
    }
    
    // uuid_iso_iec_11578
    ret = readBits(parser, 32, &val, 1);
    if (ret < 0)
    {
        fprintf(stderr, "error: failed to read from file\n");
        return ret;
    }
    _readBits += ret;
    uuid1 = val;
    ret = readBits(parser, 32, &val, 1);
    if (ret < 0)
    {
        fprintf(stderr, "error: failed to read from file\n");
        return ret;
    }
    _readBits += ret;
    uuid2 = val;
    ret = readBits(parser, 32, &val, 1);
    if (ret < 0)
    {
        fprintf(stderr, "error: failed to read from file\n");
        return ret;
    }
    _readBits += ret;
    uuid3 = val;
    ret = readBits(parser, 32, &val, 1);
    if (ret < 0)
    {
        fprintf(stderr, "error: failed to read from file\n");
        return ret;
    }
    _readBits += ret;
    uuid4 = val;
    if (parser->config.printLogs) printf("------ uuid_iso_iec_11578 = %08x-%04x-%04x-%08x-%08x\n", uuid1, uuid2 >> 16, uuid2 & 0xFFFF, uuid3, uuid4);

    if (parser->config.extractUserDataSei)
    {
        if ((!parser->pUserDataBuf) || (parser->userDataBufSize < payloadSize))
        {
            parser->pUserDataBuf = (uint8_t*)realloc(parser->pUserDataBuf, payloadSize);
            if (!parser->pUserDataBuf)
            {
                fprintf(stderr, "error: allocation failed (size %d)\n", payloadSize);
                return -1;
            }
        }

        ret = seekToByte(parser, -(_readBits / 8), SEEK_CUR);
        if (ret < 0)
        {
            fprintf(stderr, "error: failed to seek %d bytes backwards\n", (_readBits / 8));
            return ret;
        }
        _readBits = 0;

        for (i = 0; i < payloadSize; i++)
        {
            ret = readBits(parser, 8, &val, 1);
            if (ret < 0)
            {
                fprintf(stderr, "error: failed to read from file\n");
                return ret;
            }
            _readBits += ret;
            parser->pUserDataBuf[i] = (uint8_t)val;
        }
        parser->userDataSize = payloadSize;
    }
    else
    {
        ret = skipBytes(parser, payloadSize - 16);
        if (ret < 0)
        {
            fprintf(stderr, "error: skipBytes() failed (%d)\n", ret);
            return ret;
        }
        _readBits += ret * 8;
    }
    return _readBits;
}


static int parseSeiPayload_bufferingPeriod(parser_t* parser, int payloadSize)
{
    int ret = 0;
    uint32_t val = 0;
    int _readBits = 0;
    int i;

    if (parser->config.printLogs) printf("---- SEI: buffering_period\n");

    // seq_parameter_set_id
    ret = readBits_expGolomb_ue(parser, &val, 1);
    if (ret < 0)
    {
        fprintf(stderr, "error: failed to read from file\n");
        return ret;
    }
    _readBits += ret;
    if (parser->config.printLogs) printf("------ seq_parameter_set_id = %d\n", val);

    if (parser->nal_hrd_parameters_present_flag)
    {
        for (i = 0; i <= parser->cpb_cnt_minus1; i++)
        {
            // initial_cpb_removal_delay[i]
            ret = readBits(parser, parser->initial_cpb_removal_delay_length_minus1 + 1, &val, 1);
            if (ret < 0)
            {
                fprintf(stderr, "error: failed to read from file\n");
                return ret;
            }
            _readBits += ret;
            if (parser->config.printLogs) printf("------ initial_cpb_removal_delay[%d] = %d\n", i, val);

            // initial_cpb_removal_delay_offset[i]
            ret = readBits(parser, parser->initial_cpb_removal_delay_length_minus1 + 1, &val, 1);
            if (ret < 0)
            {
                fprintf(stderr, "error: failed to read from file\n");
                return ret;
            }
            _readBits += ret;
            if (parser->config.printLogs) printf("------ initial_cpb_removal_delay_offset[%d] = %d\n", i, val);
        }
    }
    
    if (parser->vcl_hrd_parameters_present_flag)
    {
        for (i = 0; i <= parser->cpb_cnt_minus1; i++)
        {
            // initial_cpb_removal_delay[i]
            ret = readBits(parser, parser->initial_cpb_removal_delay_length_minus1 + 1, &val, 1);
            if (ret < 0)
            {
                fprintf(stderr, "error: failed to read from file\n");
                return ret;
            }
            _readBits += ret;
            if (parser->config.printLogs) printf("------ initial_cpb_removal_delay[%d] = %d\n", i, val);

            // initial_cpb_removal_delay_offset[i]
            ret = readBits(parser, parser->initial_cpb_removal_delay_length_minus1 + 1, &val, 1);
            if (ret < 0)
            {
                fprintf(stderr, "error: failed to read from file\n");
                return ret;
            }
            _readBits += ret;
            if (parser->config.printLogs) printf("------ initial_cpb_removal_delay_offset[%d] = %d\n", i, val);
        }
    }

    return _readBits;
}


static int parseSeiPayload_picTiming(parser_t* parser, int payloadSize)
{
    int ret = 0;
    uint32_t val = 0;
    int _readBits = 0;
    int i, full_timestamp_flag, pic_struct;

    if (parser->config.printLogs) printf("---- SEI: pic_timing\n");

    if (parser->nal_hrd_parameters_present_flag || parser->vcl_hrd_parameters_present_flag)
    {
        // cpb_removal_delay
        ret = readBits(parser, parser->cpb_removal_delay_length_minus1 + 1, &val, 1);
        if (ret < 0)
        {
            fprintf(stderr, "error: failed to read from file\n");
            return ret;
        }
        _readBits += ret;
        if (parser->config.printLogs) printf("------ cpb_removal_delay = %d\n", val);

        // dpb_output_delay
        ret = readBits(parser, parser->dpb_output_delay_length_minus1 + 1, &val, 1);
        if (ret < 0)
        {
            fprintf(stderr, "error: failed to read from file\n");
            return ret;
        }
        _readBits += ret;
        if (parser->config.printLogs) printf("------ dpb_output_delay = %d\n", val);
    }

    if (parser->pic_struct_present_flag)
    {
        // pic_struct
        ret = readBits(parser, 4, &val, 1);
        if (ret < 0)
        {
            fprintf(stderr, "error: failed to read from file\n");
            return ret;
        }
        _readBits += ret;
        pic_struct = val;
        if (parser->config.printLogs) printf("------ pic_struct = %d\n", val);
        
        for (i = 0; i < picStructToNumClockTS[pic_struct]; i++)
        {
            // clock_timestamp_flag[i]
            ret = readBits(parser, 1, &val, 1);
            if (ret < 0)
            {
                fprintf(stderr, "error: failed to read from file\n");
                return ret;
            }
            _readBits += ret;
            if (parser->config.printLogs) printf("------ clock_timestamp_flag[%d] = %d\n", i, val);
            
            if (val)
            {
                // ct_type
                ret = readBits(parser, 2, &val, 1);
                if (ret < 0)
                {
                    fprintf(stderr, "error: failed to read from file\n");
                    return ret;
                }
                _readBits += ret;
                if (parser->config.printLogs) printf("------ ct_type = %d\n", val);

                // nuit_field_based_flag
                ret = readBits(parser, 1, &val, 1);
                if (ret < 0)
                {
                    fprintf(stderr, "error: failed to read from file\n");
                    return ret;
                }
                _readBits += ret;
                if (parser->config.printLogs) printf("------ nuit_field_based_flag = %d\n", val);

                // counting_type
                ret = readBits(parser, 5, &val, 1);
                if (ret < 0)
                {
                    fprintf(stderr, "error: failed to read from file\n");
                    return ret;
                }
                _readBits += ret;
                if (parser->config.printLogs) printf("------ counting_type = %d\n", val);

                // full_timestamp_flag
                ret = readBits(parser, 1, &val, 1);
                if (ret < 0)
                {
                    fprintf(stderr, "error: failed to read from file\n");
                    return ret;
                }
                _readBits += ret;
                full_timestamp_flag = val;
                if (parser->config.printLogs) printf("------ full_timestamp_flag = %d\n", val);

                // discontinuity_flag
                ret = readBits(parser, 1, &val, 1);
                if (ret < 0)
                {
                    fprintf(stderr, "error: failed to read from file\n");
                    return ret;
                }
                _readBits += ret;
                if (parser->config.printLogs) printf("------ discontinuity_flag = %d\n", val);

                // cnt_dropped_flag
                ret = readBits(parser, 1, &val, 1);
                if (ret < 0)
                {
                    fprintf(stderr, "error: failed to read from file\n");
                    return ret;
                }
                _readBits += ret;
                if (parser->config.printLogs) printf("------ cnt_dropped_flag = %d\n", val);

                // n_frames
                ret = readBits(parser, 8, &val, 1);
                if (ret < 0)
                {
                    fprintf(stderr, "error: failed to read from file\n");
                    return ret;
                }
                _readBits += ret;
                if (parser->config.printLogs) printf("------ n_frames = %d\n", val);
                
                if (full_timestamp_flag)
                {
                    // seconds_value
                    ret = readBits(parser, 6, &val, 1);
                    if (ret < 0)
                    {
                        fprintf(stderr, "error: failed to read from file\n");
                        return ret;
                    }
                    _readBits += ret;
                    if (parser->config.printLogs) printf("------ seconds_value = %d\n", val);

                    // minutes_value
                    ret = readBits(parser, 6, &val, 1);
                    if (ret < 0)
                    {
                        fprintf(stderr, "error: failed to read from file\n");
                        return ret;
                    }
                    _readBits += ret;
                    if (parser->config.printLogs) printf("------ minutes_value = %d\n", val);

                    // hours_value
                    ret = readBits(parser, 5, &val, 1);
                    if (ret < 0)
                    {
                        fprintf(stderr, "error: failed to read from file\n");
                        return ret;
                    }
                    _readBits += ret;
                    if (parser->config.printLogs) printf("------ hours_value = %d\n", val);
                }
                else
                {
                    // seconds_flag
                    ret = readBits(parser, 1, &val, 1);
                    if (ret < 0)
                    {
                        fprintf(stderr, "error: failed to read from file\n");
                        return ret;
                    }
                    _readBits += ret;
                    if (parser->config.printLogs) printf("------ seconds_flag = %d\n", val);

                    if (val)
                    {
                        // seconds_value
                        ret = readBits(parser, 6, &val, 1);
                        if (ret < 0)
                        {
                            fprintf(stderr, "error: failed to read from file\n");
                            return ret;
                        }
                        _readBits += ret;
                        if (parser->config.printLogs) printf("------ seconds_value = %d\n", val);

                        // minutes_flag
                        ret = readBits(parser, 1, &val, 1);
                        if (ret < 0)
                        {
                            fprintf(stderr, "error: failed to read from file\n");
                            return ret;
                        }
                        _readBits += ret;
                        if (parser->config.printLogs) printf("------ minutes_flag = %d\n", val);

                        if (val)
                        {
                            // minutes_value
                            ret = readBits(parser, 6, &val, 1);
                            if (ret < 0)
                            {
                                fprintf(stderr, "error: failed to read from file\n");
                                return ret;
                            }
                            _readBits += ret;
                            if (parser->config.printLogs) printf("------ minutes_value = %d\n", val);

                            // hours_flag
                            ret = readBits(parser, 1, &val, 1);
                            if (ret < 0)
                            {
                                fprintf(stderr, "error: failed to read from file\n");
                                return ret;
                            }
                            _readBits += ret;
                            if (parser->config.printLogs) printf("------ hours_flag = %d\n", val);

                            if (val)
                            {
                                // hours_value
                                ret = readBits(parser, 5, &val, 1);
                                if (ret < 0)
                                {
                                    fprintf(stderr, "error: failed to read from file\n");
                                    return ret;
                                }
                                _readBits += ret;
                                if (parser->config.printLogs) printf("------ hours_value = %d\n", val);
                            }
                        }
                    }
                }

                if (parser->time_offset_length)
                {
                    // time_offset
                    ret = readBits(parser, parser->time_offset_length, &val, 1); //TODO: signed value
                    if (ret < 0)
                    {
                        fprintf(stderr, "error: failed to read from file\n");
                        return ret;
                    }
                    _readBits += ret;
                    if (parser->config.printLogs) printf("------ time_offset = %d\n", val);
                }
            }
        }
    }

    return _readBits;
}


static int parseSei(parser_t* parser)
{
    int ret = 0;
    uint32_t val = 0;
    int readBytes = 0, _readBits = 0, _readBits2;
    int payloadType, payloadSize;

    // sei_rbsp
    if (parser->config.printLogs) printf("-- sei_rbsp()\n");
    
    parser->userDataSize = 0;
    
    do
    {
        // sei_message
        
        payloadType = 0;
        payloadSize = 0;
        _readBits2 = 0;
        
        // last_payload_type_byte
        do
        {
            ret = readBits(parser, 8, &val, 1);
            if (ret < 0)
            {
                fprintf(stderr, "error: failed to read from file\n");
                return ret;
            }
            _readBits += ret;
            payloadType += val;
        }
        while (val == 0xFF);
        if (parser->config.printLogs) printf("---- last_payload_type_byte = %d (payloadType = %d)\n", val, payloadType);

        // last_payload_size_byte
        do
        {
            ret = readBits(parser, 8, &val, 1);
            if (ret < 0)
            {
                fprintf(stderr, "error: failed to read from file\n");
                return ret;
            }
            _readBits += ret;
            payloadSize += val;
        }
        while (val == 0xFF);
        if (parser->config.printLogs) printf("---- last_payload_size_byte = %d (payloadSize = %d)\n", val, payloadSize);
        
        // sei_payload
        switch(payloadType)
        {
            /*case SEI_PAYLOAD_TYPE_BUFFERING_PERIOD:
                ret = parseSeiPayload_bufferingPeriod(parser, payloadSize);
                if (ret < 0)
                {
                    fprintf(stderr, "error: parseSeiPayload_bufferingPeriod() failed (%d)\n", ret);
                    return ret;
                }
                _readBits2 += ret;
                break;
            case SEI_PAYLOAD_TYPE_PIC_TIMING:
                ret = parseSeiPayload_picTiming(parser, payloadSize);
                if (ret < 0)
                {
                    fprintf(stderr, "error: parseSeiPayload_picTiming() failed (%d)\n", ret);
                    return ret;
                }
                _readBits2 += ret;
                break;*/
            case SEI_PAYLOAD_TYPE_USER_DATA_UNREGISTERED:
                ret = parseSeiPayload_userDataUnregistered(parser, payloadSize);
                if (ret < 0)
                {
                    fprintf(stderr, "error: parseSeiPayload_userDataUnregistered() failed (%d)\n", ret);
                    return ret;
                }
                _readBits2 += ret;
                break;
            default:
                if (parser->config.printLogs) printf("---- unsupported payload type (skipping)\n");
                ret = skipBytes(parser, payloadSize);
                if (ret < 0)
                {
                    fprintf(stderr, "error: skipBytes() failed (%d)\n", ret);
                    return ret;
                }
                _readBits2 += ret * 8;
                break;
        }

        // Byte align
        ret = bitstreamByteAlign(parser);
        if (ret < 0)
        {
            fprintf(stderr, "error: failed to align the bitstream (%d)\n", ret);
            return ret;
        }
        _readBits2 += ret;
        
        if (_readBits2 != payloadSize * 8)
        {
            if (parser->config.printLogs) printf("---- warning: read bits != payload size (%d != %d)\n", _readBits2, payloadSize * 8);
        }
        if (_readBits2 < payloadSize * 8)
        {
            // Skip what we should have read
            ret = skipBytes(parser, payloadSize - (_readBits2 / 8));
            if (ret < 0)
            {
                fprintf(stderr, "error: skipBytes() failed (%d)\n", ret);
                return ret;
            }
            _readBits2 += ret * 8;
        }
        
        _readBits += _readBits2;
    }
    while (moreRbspData(parser));

    // rbsp_trailing_bits
    ret = readBits(parser, 1, &val, 1);
    if (ret < 0)
    {
        fprintf(stderr, "error: failed to read from file\n");
        return ret;
    }
    _readBits += ret;

    ret = bitstreamByteAlign(parser);
    if (ret < 0)
    {
        fprintf(stderr, "error: failed to align the bitstream (%d)\n", ret);
        return ret;
    }
    _readBits += ret;
    readBytes += _readBits / 8;

    return readBytes;
}


static int parseAud(parser_t* parser)
{
    int ret = 0;
    uint32_t val = 0;
    int readBytes = 0, _readBits = 0;

    // access_unit_delimiter_rbsp
    if (parser->config.printLogs) printf("-- access_unit_delimiter_rbsp()\n");

    // primary_pic_type
    ret = readBits(parser, 3, &val, 1);
    if (ret < 0)
    {
        fprintf(stderr, "error: failed to read from file\n");
        return ret;
    }
    _readBits += ret;
    if (parser->config.printLogs) printf("---- primary_pic_type = %d\n", val);

    // rbsp_trailing_bits
    ret = readBits(parser, 1, &val, 1);
    if (ret < 0)
    {
        fprintf(stderr, "error: failed to read from file\n");
        return ret;
    }
    _readBits += ret;

    ret = bitstreamByteAlign(parser);
    if (ret < 0)
    {
        fprintf(stderr, "error: failed to align the bitstream (%d)\n", ret);
        return ret;
    }
    _readBits += ret;
    readBytes += _readBits / 8;

    return readBytes;
}


static int parseFillerData(parser_t* parser)
{
    int ret = 0;
    uint32_t val = 0;
    int readBytes = 0, _readBits = 0;
    int size = 0;

    // filler_data_rbsp
    if (parser->config.printLogs) printf("-- filler_data_rbsp()\n");

    //TODO

    return readBytes;
}


static int parseRefPicListModification(parser_t* parser, int sliceTypeMod5)
{
    int ret = 0;
    uint32_t val = 0;
    int _readBits = 0;
    int modification_of_pic_nums_idc;

    // ref_pic_list_modification
    if (parser->config.printLogs) printf("------ ref_pic_list_modification()\n");

    if ((sliceTypeMod5 != SLICE_TYPE_I) && (sliceTypeMod5 != SLICE_TYPE_SI))
    {
        // ref_pic_list_modification_flag_l0
        ret = readBits(parser, 1, &val, 1);
        if (ret < 0)
        {
            fprintf(stderr, "error: failed to read from file\n");
            return ret;
        }
        _readBits += ret;
        if (parser->config.printLogs) printf("-------- ref_pic_list_modification_flag_l0 = %d\n", val);
        
        if (val)
        {
            do
            {
                // modification_of_pic_nums_idc
                ret = readBits_expGolomb_ue(parser, &val, 1);
                if (ret < 0)
                {
                    fprintf(stderr, "error: failed to read from file\n");
                    return ret;
                }
                _readBits += ret;
                modification_of_pic_nums_idc = val;
                if (parser->config.printLogs) printf("-------- modification_of_pic_nums_idc = %d\n", val);
                
                if ((modification_of_pic_nums_idc == 0) || (modification_of_pic_nums_idc == 1))
                {
                    // abs_diff_pic_num_minus1
                    ret = readBits_expGolomb_ue(parser, &val, 1);
                    if (ret < 0)
                    {
                        fprintf(stderr, "error: failed to read from file\n");
                        return ret;
                    }
                    _readBits += ret;
                    if (parser->config.printLogs) printf("-------- abs_diff_pic_num_minus1 = %d\n", val);
                }
                else if (modification_of_pic_nums_idc == 2)
                {
                    // long_term_pic_num
                    ret = readBits_expGolomb_ue(parser, &val, 1);
                    if (ret < 0)
                    {
                        fprintf(stderr, "error: failed to read from file\n");
                        return ret;
                    }
                    _readBits += ret;
                    if (parser->config.printLogs) printf("-------- long_term_pic_num = %d\n", val);
                }
            }
            while (modification_of_pic_nums_idc != 3);
        }
    }

    if (sliceTypeMod5 == SLICE_TYPE_B)
    {
        // ref_pic_list_modification_flag_l1
        ret = readBits(parser, 1, &val, 1);
        if (ret < 0)
        {
            fprintf(stderr, "error: failed to read from file\n");
            return ret;
        }
        _readBits += ret;
        if (parser->config.printLogs) printf("-------- ref_pic_list_modification_flag_l1 = %d\n", val);
        
        if (val)
        {
            do
            {
                // modification_of_pic_nums_idc
                ret = readBits_expGolomb_ue(parser, &val, 1);
                if (ret < 0)
                {
                    fprintf(stderr, "error: failed to read from file\n");
                    return ret;
                }
                _readBits += ret;
                modification_of_pic_nums_idc = val;
                if (parser->config.printLogs) printf("-------- modification_of_pic_nums_idc = %d\n", val);
                
                if ((modification_of_pic_nums_idc == 0) || (modification_of_pic_nums_idc == 1))
                {
                    // abs_diff_pic_num_minus1
                    ret = readBits_expGolomb_ue(parser, &val, 1);
                    if (ret < 0)
                    {
                        fprintf(stderr, "error: failed to read from file\n");
                        return ret;
                    }
                    _readBits += ret;
                    if (parser->config.printLogs) printf("-------- abs_diff_pic_num_minus1 = %d\n", val);
                }
                else if (modification_of_pic_nums_idc == 2)
                {
                    // long_term_pic_num
                    ret = readBits_expGolomb_ue(parser, &val, 1);
                    if (ret < 0)
                    {
                        fprintf(stderr, "error: failed to read from file\n");
                        return ret;
                    }
                    _readBits += ret;
                    if (parser->config.printLogs) printf("-------- long_term_pic_num = %d\n", val);
                }
            }
            while (modification_of_pic_nums_idc != 3);
        }
    }

    return _readBits;
}


static int parsePredWeightTable(parser_t* parser)
{
    int ret = 0;
    uint32_t val = 0;
    int _readBits = 0;

    // pred_weight_table
    if (parser->config.printLogs) printf("------ pred_weight_table()\n");

    // luma_log2_weight_denom
    ret = readBits_expGolomb_ue(parser, &val, 1);
    if (ret < 0)
    {
        fprintf(stderr, "error: failed to read from file\n");
        return ret;
    }
    _readBits += ret;
    if (parser->config.printLogs) printf("-------- luma_log2_weight_denom = %d\n", val);

    exit(-1);
    //TODO

    return _readBits;
}


static int parseDecRefPicMarking(parser_t* parser)
{
    int ret = 0;
    uint32_t val = 0;
    int _readBits = 0;

    // dec_ref_pic_marking
    if (parser->config.printLogs) printf("------ dec_ref_pic_marking()\n");

    if (parser->idrPicFlag)
    {
        // no_output_of_prior_pics_flag
        ret = readBits(parser, 1, &val, 1);
        if (ret < 0)
        {
            fprintf(stderr, "error: failed to read from file\n");
            return ret;
        }
        _readBits += ret;
        if (parser->config.printLogs) printf("-------- no_output_of_prior_pics_flag = %d\n", val);

        // long_term_reference_flag
        ret = readBits(parser, 1, &val, 1);
        if (ret < 0)
        {
            fprintf(stderr, "error: failed to read from file\n");
            return ret;
        }
        _readBits += ret;
        if (parser->config.printLogs) printf("-------- long_term_reference_flag = %d\n", val);
    }
    else
    {
        // adaptive_ref_pic_marking_mode_flag
        ret = readBits(parser, 1, &val, 1);
        if (ret < 0)
        {
            fprintf(stderr, "error: failed to read from file\n");
            return ret;
        }
        _readBits += ret;
        if (parser->config.printLogs) printf("-------- adaptive_ref_pic_marking_mode_flag = %d\n", val);
        
        if (val)
        {
            int memory_management_control_operation = 0;
            
            do
            {
                // memory_management_control_operation
                ret = readBits_expGolomb_ue(parser, &val, 1);
                if (ret < 0)
                {
                    fprintf(stderr, "error: failed to read from file\n");
                    return ret;
                }
                _readBits += ret;
                memory_management_control_operation = val;
                if (parser->config.printLogs) printf("-------- memory_management_control_operation = %d\n", val);
                
                if ((memory_management_control_operation == 1) || (memory_management_control_operation == 3))
                {
                    // difference_of_pic_nums_minus1
                    ret = readBits_expGolomb_ue(parser, &val, 1);
                    if (ret < 0)
                    {
                        fprintf(stderr, "error: failed to read from file\n");
                        return ret;
                    }
                    _readBits += ret;
                    if (parser->config.printLogs) printf("-------- difference_of_pic_nums_minus1 = %d\n", val);
                }

                if (memory_management_control_operation == 2)
                {
                    // long_term_pic_num
                    ret = readBits_expGolomb_ue(parser, &val, 1);
                    if (ret < 0)
                    {
                        fprintf(stderr, "error: failed to read from file\n");
                        return ret;
                    }
                    _readBits += ret;
                    if (parser->config.printLogs) printf("-------- long_term_pic_num = %d\n", val);
                }

                if ((memory_management_control_operation == 3) || (memory_management_control_operation == 6))
                {
                    // long_term_frame_idx
                    ret = readBits_expGolomb_ue(parser, &val, 1);
                    if (ret < 0)
                    {
                        fprintf(stderr, "error: failed to read from file\n");
                        return ret;
                    }
                    _readBits += ret;
                    if (parser->config.printLogs) printf("-------- long_term_frame_idx = %d\n", val);

                if (memory_management_control_operation == 4)
                {
                    // max_long_term_frame_idx_plus1
                    ret = readBits_expGolomb_ue(parser, &val, 1);
                    if (ret < 0)
                    {
                        fprintf(stderr, "error: failed to read from file\n");
                        return ret;
                    }
                    _readBits += ret;
                    if (parser->config.printLogs) printf("-------- max_long_term_frame_idx_plus1 = %d\n", val);
                }
                }
            }
            while (memory_management_control_operation != 0);
        }
    }

    return _readBits;
}


static int parseSlice(parser_t* parser)
{
    int ret = 0;
    uint32_t val = 0;
    int32_t val_se = 0;
    int readBytes = 0, _readBits = 0;
    int field_pic_flag = 0, sliceTypeMod5;

    memset(&parser->sliceInfo, 0, sizeof (H264P_SliceInfo_t));
    parser->sliceInfo.idrPicFlag = parser->idrPicFlag;
    parser->sliceInfo.nal_ref_idc = parser->nal_ref_idc;
    parser->sliceInfo.nal_unit_type = parser->nal_unit_type;

    // slice_layer_without_partitioning_rbsp
    if (parser->config.printLogs) printf("-- slice_layer_without_partitioning_rbsp()\n");

    // slice_header
    if (parser->config.printLogs) printf("---- slice_header()\n");

    // first_mb_in_slice
    ret = readBits_expGolomb_ue(parser, &val, 1);
    if (ret < 0)
    {
        fprintf(stderr, "error: failed to read from file\n");
        return ret;
    }
    _readBits += ret;
    parser->sliceInfo.first_mb_in_slice = val;
    if (parser->config.printLogs) printf("------ first_mb_in_slice = %d\n", val);

    // slice_type
    ret = readBits_expGolomb_ue(parser, &val, 1);
    if (ret < 0)
    {
        fprintf(stderr, "error: failed to read from file\n");
        return ret;
    }
    _readBits += ret;
    sliceTypeMod5 = val % 5;
    parser->sliceInfo.slice_type = val;
    parser->sliceInfo.sliceTypeMod5 = sliceTypeMod5;
    if (parser->config.printLogs) printf("------ slice_type = %d (%s)\n", val, (val <= 9) ? sliceTypeStr[val] : "(invalid)");

    // pic_parameter_set_id
    ret = readBits_expGolomb_ue(parser, &val, 1);
    if (ret < 0)
    {
        fprintf(stderr, "error: failed to read from file\n");
        return ret;
    }
    _readBits += ret;
    if (parser->config.printLogs) printf("------ pic_parameter_set_id = %d\n", val);

    if (parser->separate_colour_plane_flag == 1)
    {
        // colour_plane_id
        ret = readBits(parser, 2, &val, 1);
        if (ret < 0)
        {
            fprintf(stderr, "error: failed to read from file\n");
            return ret;
        }
        _readBits += ret;
        if (parser->config.printLogs) printf("------ colour_plane_id = %d\n", val);
    }

    // frame_num
    ret = readBits(parser, parser->log2_max_frame_num_minus4 + 4, &val, 1);
    if (ret < 0)
    {
        fprintf(stderr, "error: failed to read from file\n");
        return ret;
    }
    _readBits += ret;
    parser->sliceInfo.frame_num = val;
    if (parser->config.printLogs) printf("------ frame_num = %d\n", val);

    if (!parser->frame_mbs_only_flag)
    {
        // field_pic_flag
        ret = readBits(parser, 1, &val, 1);
        if (ret < 0)
        {
            fprintf(stderr, "error: failed to read from file\n");
            return ret;
        }
        _readBits += ret;
        field_pic_flag = val;
        if (parser->config.printLogs) printf("------ field_pic_flag = %d\n", val);

        if (val)
        {
            // bottom_field_flag
            ret = readBits(parser, 1, &val, 1);
            if (ret < 0)
            {
                fprintf(stderr, "error: failed to read from file\n");
                return ret;
            }
            _readBits += ret;
            if (parser->config.printLogs) printf("------ bottom_field_flag = %d\n", val);
        }
    }

    if (parser->idrPicFlag)
    {
        // idr_pic_id
        ret = readBits_expGolomb_ue(parser, &val, 1);
        if (ret < 0)
        {
            fprintf(stderr, "error: failed to read from file\n");
            return ret;
        }
        _readBits += ret;
        parser->sliceInfo.idr_pic_id = val;
        if (parser->config.printLogs) printf("------ idr_pic_id = %d\n", val);
    }

    if (parser->pic_order_cnt_type == 0)
    {
        // pic_order_cnt_lsb
        ret = readBits(parser, parser->log2_max_pic_order_cnt_lsb_minus4 + 4, &val, 1);
        if (ret < 0)
        {
            fprintf(stderr, "error: failed to read from file\n");
            return ret;
        }
        _readBits += ret;
        if (parser->config.printLogs) printf("------ pic_order_cnt_lsb = %d\n", val);

        if ((parser->bottom_field_pic_order_in_frame_present_flag) && (!field_pic_flag))
        {
            // delta_pic_order_cnt_bottom
            ret = readBits_expGolomb_se(parser, &val_se, 1);
            if (ret < 0)
            {
                fprintf(stderr, "error: failed to read from file\n");
                return ret;
            }
            _readBits += ret;
            if (parser->config.printLogs) printf("------ delta_pic_order_cnt_bottom = %d\n", val);
        }
    }

    if ((parser->pic_order_cnt_type == 1) && (!parser->delta_pic_order_always_zero_flag))
    {
        // delta_pic_order_cnt[0]
        ret = readBits_expGolomb_se(parser, &val_se, 1);
        if (ret < 0)
        {
            fprintf(stderr, "error: failed to read from file\n");
            return ret;
        }
        _readBits += ret;
        if (parser->config.printLogs) printf("------ delta_pic_order_cnt[0] = %d\n", val);

        if ((parser->bottom_field_pic_order_in_frame_present_flag) && (!field_pic_flag))
        {
            // delta_pic_order_cnt[1]
            ret = readBits_expGolomb_se(parser, &val_se, 1);
            if (ret < 0)
            {
                fprintf(stderr, "error: failed to read from file\n");
                return ret;
            }
            _readBits += ret;
            if (parser->config.printLogs) printf("------ delta_pic_order_cnt[1] = %d\n", val);
        }
    }

    if (parser->redundant_pic_cnt_present_flag)
    {
        // redundant_pic_cnt
        ret = readBits_expGolomb_ue(parser, &val, 1);
        if (ret < 0)
        {
            fprintf(stderr, "error: failed to read from file\n");
            return ret;
        }
        _readBits += ret;
        if (parser->config.printLogs) printf("------ redundant_pic_cnt = %d\n", val);
    }
    
    if (sliceTypeMod5 == SLICE_TYPE_B)
    {
        // direct_spatial_mv_pred_flag
        ret = readBits(parser, 1, &val, 1);
        if (ret < 0)
        {
            fprintf(stderr, "error: failed to read from file\n");
            return ret;
        }
        _readBits += ret;
        if (parser->config.printLogs) printf("------ direct_spatial_mv_pred_flag = %d\n", val);
    }
    
    if ((sliceTypeMod5 == SLICE_TYPE_P) || (sliceTypeMod5 == SLICE_TYPE_SP) || (sliceTypeMod5 == SLICE_TYPE_B))
    {
        // num_ref_idx_active_override_flag
        ret = readBits(parser, 1, &val, 1);
        if (ret < 0)
        {
            fprintf(stderr, "error: failed to read from file\n");
            return ret;
        }
        _readBits += ret;
        if (parser->config.printLogs) printf("------ num_ref_idx_active_override_flag = %d\n", val);
        
        if (val)
        {
            // num_ref_idx_l0_active_minus1
            ret = readBits_expGolomb_ue(parser, &val, 1);
            if (ret < 0)
            {
                fprintf(stderr, "error: failed to read from file\n");
                return ret;
            }
            _readBits += ret;
            if (parser->config.printLogs) printf("------ num_ref_idx_l0_active_minus1 = %d\n", val);

            if (sliceTypeMod5 == SLICE_TYPE_B)
            {
                // num_ref_idx_l1_active_minus1
                ret = readBits_expGolomb_ue(parser, &val, 1);
                if (ret < 0)
                {
                    fprintf(stderr, "error: failed to read from file\n");
                    return ret;
                }
                _readBits += ret;
                if (parser->config.printLogs) printf("------ num_ref_idx_l1_active_minus1 = %d\n", val);
            }
        }
    }
    
    if ((parser->nal_unit_type == 20) || (parser->nal_unit_type == 21))
    {    
        // ref_pic_list_mvc_modification()
        //TODO
        exit(-1);
    }
    else
    {
        // ref_pic_list_modification()
        ret = parseRefPicListModification(parser, sliceTypeMod5);
        if (ret < 0)
        {
            fprintf(stderr, "error: parseRefPicListModification() failed (%d)\n", ret);
            return ret;
        }
        _readBits += ret;
    }
    
    
    if ((parser->weighted_pred_flag && ((sliceTypeMod5 == SLICE_TYPE_P) || (sliceTypeMod5 == SLICE_TYPE_SP))) 
            || ((parser->weighted_bipred_idc == 1) && (sliceTypeMod5 == SLICE_TYPE_B)))
    {
        // pred_weight_table()
        ret = parsePredWeightTable(parser);
        if (ret < 0)
        {
            fprintf(stderr, "error: parsePredWeightTable() failed (%d)\n", ret);
            return ret;
        }
        _readBits += ret;
    }
    
    if (parser->nal_ref_idc != 0)
    {
        // dec_ref_pic_marking()
        ret = parseDecRefPicMarking(parser);
        if (ret < 0)
        {
            fprintf(stderr, "error: parseDecRefPicMarking() failed (%d)\n", ret);
            return ret;
        }
        _readBits += ret;
    }
    
    if ((parser->entropy_coding_mode_flag) && (sliceTypeMod5 != SLICE_TYPE_I) && (sliceTypeMod5 != SLICE_TYPE_SI))
    {
        // cabac_init_idc
        ret = readBits_expGolomb_ue(parser, &val, 1);
        if (ret < 0)
        {
            fprintf(stderr, "error: failed to read from file\n");
            return ret;
        }
        _readBits += ret;
        if (parser->config.printLogs) printf("------ cabac_init_idc = %d\n", val);
    }
    
    // slice_qp_delta
    ret = readBits_expGolomb_se(parser, &val_se, 1);
    if (ret < 0)
    {
        fprintf(stderr, "error: failed to read from file\n");
        return ret;
    }
    _readBits += ret;
    parser->sliceInfo.slice_qp_delta = val;
    if (parser->config.printLogs) printf("------ slice_qp_delta = %d\n", val);

    if ((sliceTypeMod5 == SLICE_TYPE_SP) || (sliceTypeMod5 == SLICE_TYPE_SI))
    {
        if (sliceTypeMod5 == SLICE_TYPE_SP)
        {
            // sp_for_switch_flag
            ret = readBits(parser, 1, &val, 1);
            if (ret < 0)
            {
                fprintf(stderr, "error: failed to read from file\n");
                return ret;
            }
            _readBits += ret;
            if (parser->config.printLogs) printf("------ sp_for_switch_flag = %d\n", val);
        }

        // slice_qs_delta
        ret = readBits_expGolomb_se(parser, &val_se, 1);
        if (ret < 0)
        {
            fprintf(stderr, "error: failed to read from file\n");
            return ret;
        }
        _readBits += ret;
        if (parser->config.printLogs) printf("------ slice_qs_delta = %d\n", val);
    }
    
    if (parser->deblocking_filter_control_present_flag)
    {
        // disable_deblocking_filter_idc
        ret = readBits_expGolomb_ue(parser, &val, 1);
        if (ret < 0)
        {
            fprintf(stderr, "error: failed to read from file\n");
            return ret;
        }
        _readBits += ret;
        parser->sliceInfo.disable_deblocking_filter_idc = val;
        if (parser->config.printLogs) printf("------ disable_deblocking_filter_idc = %d\n", val);
        
        if (val != 1)
        {
            // slice_alpha_c0_offset_div2
            ret = readBits_expGolomb_se(parser, &val_se, 1);
            if (ret < 0)
            {
                fprintf(stderr, "error: failed to read from file\n");
                return ret;
            }
            _readBits += ret;
            if (parser->config.printLogs) printf("------ slice_alpha_c0_offset_div2 = %d\n", val);

            // slice_beta_offset_div2
            ret = readBits_expGolomb_se(parser, &val_se, 1);
            if (ret < 0)
            {
                fprintf(stderr, "error: failed to read from file\n");
                return ret;
            }
            _readBits += ret;
            if (parser->config.printLogs) printf("------ slice_beta_offset_div2 = %d\n", val);
        }
    }
    
    if ((parser->num_slice_groups_minus1 > 0) && (parser->slice_group_map_type >= 3) && (parser->slice_group_map_type <= 5))
    {
        int picSizeInMapUnits, n;

        picSizeInMapUnits = (parser->pic_width_in_mbs_minus1 + 1) * (parser->pic_height_in_map_units_minus1 + 1);
        n = ceil(log2((picSizeInMapUnits / (parser->slice_group_change_rate_minus1 + 1)) + 1));

        // slice_group_change_cycle
        ret = readBits(parser, n, &val, 1);
        if (ret < 0)
        {
            fprintf(stderr, "error: failed to read from file\n");
            return ret;
        }
        _readBits += ret;
        if (parser->config.printLogs) printf("------ slice_group_change_cycle = %d\n", val);
    }


    // rbsp_slice_trailing_bits

    // rbsp_trailing_bits
    ret = readBits(parser, 1, &val, 1);
    if (ret < 0)
    {
        fprintf(stderr, "error: failed to read from file\n");
        return ret;
    }
    _readBits += ret;

    ret = bitstreamByteAlign(parser);
    if (ret < 0)
    {
        fprintf(stderr, "error: failed to align the bitstream (%d)\n", ret);
        return ret;
    }
    _readBits += ret;
    readBytes += _readBits / 8;

    //TODO: cabac_zero_word

    return readBytes;
}


int H264P_ParseNalu(H264P_Handle parserHandle)
{
    parser_t* parser = (parser_t*)parserHandle;
    uint32_t val;
    int ret;
    int readBytes = 0;
    int forbidden_zero_bit;

    if (!parserHandle)
    {
        fprintf(stderr, "Error: invalid handle\n");
        return -1;
    }

    ret = readBits(parser, 8, &val, 0);
    if (ret != 8)
    {
        fprintf(stderr, "Error: failed to read 1 byte from file\n");
        return -1;
    }
    readBytes++;

    forbidden_zero_bit = (val >> 7) & 0x1;
    parser->nal_ref_idc = (val >> 5) & 0x3;
    parser->nal_unit_type = val & 0x1F;

    if (parser->config.printLogs) printf("-- NALU found: nal_ref_idc=%d, nal_unit_type=%d (%s)\n", parser->nal_ref_idc, parser->nal_unit_type, naluTypeStr[parser->nal_unit_type]);

    parser->idrPicFlag = (parser->nal_unit_type == 5) ? 1 : 0;
    
    if (parseNaluType[parser->nal_unit_type])
    {
        ret = parseNaluType[parser->nal_unit_type](parser);
        if (ret < 0)
        {
            fprintf(stderr, "Error: parseNaluType[%d]() failed (%d)\n", parser->nal_unit_type, ret);
            return -1;
        }
        readBytes += ret;
    }
    
    if (forbidden_zero_bit != 0)
    {
        if (parser->config.printLogs) printf("   Warning: forbidden_zero_bit is not 0!\n");
    }
    
    return readBytes;
}


static int startcodeMatch_file(parser_t* parser, FILE* fp, unsigned int fileSize)
{
    int ret, initPos, pos, end, i = 0;
    uint32_t val, shiftVal;

    initPos = pos = ftell(fp);
    end = fileSize;

    if (pos + 4 > end) return -2;

    ret = fread(&val, 4, 1, fp);
    if (ret != 1) return -1;
    shiftVal = val = ntohl(val);

    while ((shiftVal != BYTE_STREAM_NALU_START_CODE) && (pos < end))
    {
        if ((!i) && (pos < end - 4))
        {
            i = 4;

            ret = fread(&val, 4, 1, fp);
            if (ret != 1)
            {
                return -1;
            }
            val = ntohl(val);
        }

        shiftVal <<= 8;
        shiftVal |= (val >> 24) & 0xFF;
        val <<= 8;

        pos++;
        i--;
    }

    if (shiftVal == BYTE_STREAM_NALU_START_CODE)
    {
        pos += 4;
        ret = fseek(fp, pos, SEEK_SET);
        if (ret != 0) return -1;
        ret = pos;
    }
    else
    {
        ret = fseek(fp, initPos, SEEK_SET);
        if (ret != 0) return -1;
        ret = -2;
    }

    return ret;
}



int H264P_ReadNextNalu_file(H264P_Handle parserHandle, FILE* fp, unsigned int fileSize)
{
    parser_t* parser = (parser_t*)parserHandle;
    int ret = 0;
    int naluStart, naluEnd, naluSize = 0;

    if (!parserHandle)
    {
        fprintf(stderr, "Error: invalid handle\n");
        return -1;
    }
    
    // Search for next NALU start code
    ret = startcodeMatch_file(parser, fp, fileSize);
    if (ret >= 0)
    {
        // Start code found
        naluStart = ret;
        if (parser->config.printLogs) printf("Start code at 0x%08X\n", (uint32_t)(naluStart - 4));
        
        // Search for NALU end (next NALU start code or end of file)
        ret = startcodeMatch_file(parser, fp, fileSize);
        if (ret >= 0)
        {
            // Start code found
            naluEnd = ret - 4;            
        }
        else if (ret == -2)
        {
            // No start code found
            naluEnd = fileSize;
        }
        else //if (ret < 0)
        {
            fprintf(stderr, "Error: startcodeMatch_file() failed (%d)\n", ret);
            return ret;
        }
        
        naluSize = naluEnd - naluStart;
        if (naluSize > 0)
        {
            ret = fseek(fp, naluStart, SEEK_SET);
            if (ret != 0)
            {
                fprintf(stderr, "Error: failed to seek in file\n");
                return ret;
            }
            
            parser->naluBufManaged = 1;
            if (naluSize > parser->naluBufSize)
            {
                parser->naluBufSize = naluSize;
                parser->pNaluBuf = (uint8_t*)realloc(parser->pNaluBuf, parser->naluBufSize);
                if (!parser->pNaluBuf)
                {
                    fprintf(stderr, "Error: reallocation failed (size %d)\n", parser->naluBufSize);
                    return ret;
                }
            }
            
            ret = fread(parser->pNaluBuf, naluSize, 1, fp);
            if (ret != 1)
            {
                fprintf(stderr, "Error: failed to read %d bytes in file\n", naluSize);
                return ret;
            }
            parser->naluSize = naluSize;
            parser->remNaluSize = naluSize;
            parser->pNaluBufCur = parser->pNaluBuf;

            // Reset the cache
            parser->cache = 0;
            parser->cacheLength = 0;
            parser->oldZeroCount = 0; // NB: this value is wrong when emulation prevention is in use (inside NAL Units)
        }
        else
        {
            fprintf(stderr, "Error: invalid NALU size\n");
            return -1;
        }
    }
    else if (ret == -2)
    {
        // No start code found
        if (parser->config.printLogs) printf("No start code found\n");
        return ret;
    }
    else //if (ret < 0)
    {
        fprintf(stderr, "Error: startcodeMatch_file() failed (%d)\n", ret);
        return ret;
    }

    return naluSize;
}


static int startcodeMatch_buffer(parser_t* parser, uint8_t* pBuf, unsigned int bufSize)
{
    int ret, pos, end;
    uint32_t shiftVal = 0;
    uint8_t* ptr = pBuf;

    if (bufSize < 4) return -2;

    pos = 0;
    end = bufSize;

    do
    {
        shiftVal <<= 8;
        shiftVal |= (*ptr++) & 0xFF;
        pos++;
    }
    while (((shiftVal != BYTE_STREAM_NALU_START_CODE) && (pos < end)) || (pos < 4));

    if (shiftVal == BYTE_STREAM_NALU_START_CODE)
    {
        ret = pos;
    }
    else
    {
        ret = -2;
    }

    return ret;
}



int H264P_ReadNextNalu_buffer(H264P_Handle parserHandle, void* pBuf, unsigned int bufSize, unsigned int* nextStartCodePos)
{
    parser_t* parser = (parser_t*)parserHandle;
    int ret = 0;
    int naluStart, naluEnd, naluSize;

    if (!parserHandle)
    {
        fprintf(stderr, "Error: invalid handle\n");
        return -1;
    }
    
    if (parser->naluBufManaged)
    {
        fprintf(stderr, "Error: invalid state\n");
        return -1;
    }
    
    if (nextStartCodePos) *nextStartCodePos = 0;
    
    // Search for next NALU start code
    ret = startcodeMatch_buffer(parser, (uint8_t*)pBuf, bufSize);
    if (ret >= 0)
    {
        // Start code found
        naluStart = ret;
        if (parser->config.printLogs) printf("Start code at 0x%08X\n", (uint32_t)(naluStart - 4));
        
        // Search for NALU end (next NALU start code or end of file)
        ret = startcodeMatch_buffer(parser, (uint8_t*)pBuf + naluStart, bufSize - naluStart);
        if (ret >= 0)
        {
            // Start code found
            naluEnd = naluStart + ret - 4;
            if (nextStartCodePos) *nextStartCodePos = naluEnd;
        }
        else if (ret == -2)
        {
            // No start code found
            naluEnd = bufSize;
        }
        else //if (ret < 0)
        {
            fprintf(stderr, "Error: startcodeMatch_buffer() failed (%d)\n", ret);
            return ret;
        }
        
        naluSize = naluEnd - naluStart;
        if (naluSize > 0)
        {
            parser->naluSize = parser->remNaluSize = parser->naluBufSize = naluSize;
            parser->pNaluBufCur = parser->pNaluBuf = (uint8_t*)pBuf + naluStart;

            // Reset the cache
            parser->cache = 0;
            parser->cacheLength = 0;
            parser->oldZeroCount = 0; // NB: this value is wrong when emulation prevention is in use (inside NAL Units)
        }
        else
        {
            fprintf(stderr, "Error: invalid NALU size\n");
            return -1;
        }
        
        ret = naluStart;
    }
    else if (ret == -2)
    {
        // No start code found
        if (parser->config.printLogs) printf("No start code found\n");

        parser->naluSize = parser->remNaluSize = parser->naluBufSize = bufSize;
        parser->pNaluBufCur = parser->pNaluBuf = (uint8_t*)pBuf;

        // Reset the cache
        parser->cache = 0;
        parser->cacheLength = 0;
        parser->oldZeroCount = 0; // NB: this value is wrong when emulation prevention is in use (inside NAL Units)

        ret = 0;
    }
    else //if (ret < 0)
    {
        fprintf(stderr, "Error: startcodeMatch_buffer() failed (%d)\n", ret);
        return ret;
    }

    return ret;
}


int H264P_GetLastNaluType(H264P_Handle parserHandle)
{
    parser_t* parser = (parser_t*)parserHandle;

    if (!parserHandle)
    {
        fprintf(stderr, "Error: invalid handle\n");
        return -1;
    }

    return parser->nal_unit_type;
}


int H264P_GetSliceInfo(H264P_Handle parserHandle, H264P_SliceInfo_t* sliceInfo)
{
    parser_t* parser = (parser_t*)parserHandle;
    int ret = 0;

    if (!parserHandle)
    {
        fprintf(stderr, "Error: invalid handle\n");
        return -1;
    }

    if (!sliceInfo)
    {
        fprintf(stderr, "Error: invalid pointer\n");
        return -1;
    }

    memcpy(sliceInfo, &parser->sliceInfo, sizeof(H264P_SliceInfo_t));
    
    return ret;
}


int H264P_GetUserDataSei(H264P_Handle parserHandle, void** pBuf, unsigned int* bufSize)
{
    parser_t* parser = (parser_t*)parserHandle;
    int ret = 0;

    if (!parserHandle)
    {
        fprintf(stderr, "Error: invalid handle\n");
        return -1;
    }

    if ((parser->config.extractUserDataSei) && (parser->pUserDataBuf) && (parser->userDataSize))
    {
        if (bufSize) *bufSize = parser->userDataSize;
        if (pBuf) *pBuf = (void*)parser->pUserDataBuf;
    }
    else
    {
        if (bufSize) *bufSize = 0;
        if (pBuf) *pBuf = NULL;
    }
    
    return ret;
}


int H264P_Init(H264P_Handle* parserHandle, H264P_Config_t* config)
{
    parser_t* parser;

    if (!parserHandle)
    {
        fprintf(stderr, "Error: invalid pointer for handle\n");
        return -1;
    }
    
    parser = (parser_t*)malloc(sizeof(*parser));
    if (!parser)
    {
        fprintf(stderr, "Error: allocation failed (size %ld)\n", sizeof(*parser));
        return -1;
    }
    memset(parser, 0, sizeof(*parser));

    if (config)
    {
        memcpy(&parser->config, config, sizeof(parser->config));
    }
    
    parser->cache = 0;
    parser->cacheLength = 0;

    parser->naluBufSize = 0;
    parser->pNaluBuf = NULL;
    
    *parserHandle = (H264P_Handle*)parser;
    
    return 0;
}


int H264P_Free(H264P_Handle parserHandle)
{
    parser_t* parser = (parser_t*)parserHandle;

    if (!parserHandle)
    {
        return 0;
    }
    
    if ((parser->pNaluBuf) && (parser->naluBufManaged))
    {
        free(parser->pNaluBuf);
    }
    
    if (parser->pUserDataBuf)
    {
        free(parser->pUserDataBuf);
    }
    
    free(parser);
    
    return 0;
}

