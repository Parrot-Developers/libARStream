/*
    Copyright (C) 2015 Parrot SA

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
/**
 * @file ARSTREAM_Reader2Debug.c
 * @brief Stream reader on network (v2) debug addon
 * @date 05/28/2015
 * @author aurelien.barre@parrot.com
 */

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <math.h>
#include <locale.h>

#include "h264p.h"
#include "h264p_parrot.h"
#include "ARSTREAM_Reader2Debug.h"

#include <libARSAL/ARSAL_Print.h>


#define ARSTREAM_READER2DEBUG_TAG "ARSTREAM_Reader2Debug"

#define ARSTREAM_READER2DEBUG_PSNR_MAX 48.130803609


const char* ARSTREAM_READER2DEBUG_STATS_OUTPUT_PATH_NAP_USB = "/tmp/mnt/STREAMDEBUG";
const char* ARSTREAM_READER2DEBUG_STATS_OUTPUT_PATH_NAP_INTERNAL = "/data/skycontroller/streamdebug";
const char* ARSTREAM_READER2DEBUG_STATS_OUTPUT_PATH_ANDROID_INTERNAL = "/storage/emulated/legacy/FF";
const char* ARSTREAM_READER2DEBUG_STATS_OUTPUT_PATH_PCLINUX = ".";

const char* ARSTREAM_READER2DEBUG_VIDEO_OUTPUT_PATH_NAP_USB = "/tmp/mnt/STREAMDEBUG";
const char* ARSTREAM_READER2DEBUG_VIDEO_OUTPUT_PATH_NAP_INTERNAL = "/data/skycontroller/streamdebug";
const char* ARSTREAM_READER2DEBUG_VIDEO_OUTPUT_PATH_ANDROID_INTERNAL = "/storage/emulated/legacy/FF";
const char* ARSTREAM_READER2DEBUG_VIDEO_OUTPUT_PATH_PCLINUX = ".";


struct ARSTREAM_Reader2Debug_t {
    int curIdrPicId;
    int idrPicIdBase;
    int lastFrameIdx;
    uint32_t previousFrameIndex;
    H264P_Handle h264p;
    FILE* statsFile;
    FILE* videoFile;
};


ARSTREAM_Reader2Debug_t* ARSTREAM_Reader2Debug_New(int outputStatFile, int outputVideoFile)
{
    int i;
    char szStatsFileName[128];
    char szVideoFileName[128];

    ARSTREAM_Reader2Debug_t* rdbg = malloc(sizeof(ARSTREAM_Reader2Debug_t));
    if (!rdbg)
    {
        ARSAL_PRINT(ARSAL_PRINT_ERROR, ARSTREAM_READER2DEBUG_TAG, "allocation failed in %s", __FUNCTION__);
        return NULL;
    }
    memset(rdbg, 0, sizeof(ARSTREAM_Reader2Debug_t));

    H264P_Config_t h264pConfig;
    memset(&h264pConfig, 0, sizeof(h264pConfig));
    
    h264pConfig.extractUserDataSei = 1;
    h264pConfig.printLogs = 0;
    
    int ret = H264P_Init(&rdbg->h264p, &h264pConfig);
    if (ret < 0)
    {
        ARSAL_PRINT(ARSAL_PRINT_ERROR, ARSTREAM_READER2DEBUG_TAG, "H264P_Init() failed (%d) in %s", ret, __FUNCTION__);
        free(rdbg);
        return NULL;
    }

    if (outputStatFile)
    {
        const char* pszStatsFilePath = NULL;
        szStatsFileName[0] = '\0';
        if ((access(ARSTREAM_READER2DEBUG_STATS_OUTPUT_PATH_NAP_USB, F_OK) == 0) && (access(ARSTREAM_READER2DEBUG_STATS_OUTPUT_PATH_NAP_USB, W_OK) == 0))
        {
            pszStatsFilePath = ARSTREAM_READER2DEBUG_STATS_OUTPUT_PATH_NAP_USB;
        }
        else if ((access(ARSTREAM_READER2DEBUG_STATS_OUTPUT_PATH_NAP_INTERNAL, F_OK) == 0) && (access(ARSTREAM_READER2DEBUG_STATS_OUTPUT_PATH_NAP_INTERNAL, W_OK) == 0))
        {
            pszStatsFilePath = ARSTREAM_READER2DEBUG_STATS_OUTPUT_PATH_NAP_INTERNAL;
        }
        else if ((access(ARSTREAM_READER2DEBUG_STATS_OUTPUT_PATH_ANDROID_INTERNAL, F_OK) == 0) && (access(ARSTREAM_READER2DEBUG_STATS_OUTPUT_PATH_ANDROID_INTERNAL, W_OK) == 0))
        {
            pszStatsFilePath = ARSTREAM_READER2DEBUG_STATS_OUTPUT_PATH_ANDROID_INTERNAL;
        }
        else if ((access(ARSTREAM_READER2DEBUG_STATS_OUTPUT_PATH_PCLINUX, F_OK) == 0) && (access(ARSTREAM_READER2DEBUG_STATS_OUTPUT_PATH_PCLINUX, W_OK) == 0))
        {
            pszStatsFilePath = ARSTREAM_READER2DEBUG_STATS_OUTPUT_PATH_PCLINUX;
        }
        if (pszStatsFilePath)
        {
            for (i = 0; i < 100; i++)
            {
                snprintf(szStatsFileName, 128, "%s/stats_%02d.dat", pszStatsFilePath, i);
                if (access(szStatsFileName, F_OK) == -1)
                {
                    // file does not exist
                    break;
                }
                szStatsFileName[0] = '\0';
            }
        }
    }

    if (strlen(szStatsFileName))
    {
        rdbg->statsFile = fopen(szStatsFileName, "w");
        if (!rdbg->statsFile)
        {
            ARSAL_PRINT(ARSAL_PRINT_ERROR, ARSTREAM_READER2DEBUG_TAG, "unable to open stats file '%s' in %s", szStatsFileName, __FUNCTION__);
        }
    }
    else
    {
        rdbg->statsFile = NULL;
    }
    if (rdbg->statsFile)
    {
        fprintf(rdbg->statsFile, "frameIndex acquisitionTs rtpTs receptionTs systemTs frameSize psnr estimatedLostFrames sliceInfoIdrPicFlag sliceInfoSliceTypeMod5 ");
        fprintf(rdbg->statsFile, "batteryPercentage latitude longitude altitude absoluteHeight relativeHeight xSpeed ySpeed zSpeed distance heading yaw pitch roll cameraPan cameraTilt ");
        fprintf(rdbg->statsFile, "targetLiveVideoBitrate wifiRssi wifiMcsRate wifiTxRate wifiRxRate wifiTxFailRate wifiTxErrorRate ");
        fprintf(rdbg->statsFile, "postReprojTimestampDelta postEeTimestampDelta postScalingTimestampDelta postLiveEncodingTimestampDelta postNetworkTimestampDelta ");
        fprintf(rdbg->statsFile, "streamingSrcMeanAcqToNetworkTime streamingSrcAcqToNetworkJitter streamingSrcBytesSent streamingSrcMeanPacketSize streamingSrcPacketSizeStdDev ");
        fprintf(rdbg->statsFile, "streamingSinkMissingPackets streamingSinkTotalPackets\n");
    }

    if (outputVideoFile)
    {
        const char* pszVideoFilePath = NULL;
        szVideoFileName[0] = '\0';
        if ((access(ARSTREAM_READER2DEBUG_VIDEO_OUTPUT_PATH_NAP_USB, F_OK) == 0) && (access(ARSTREAM_READER2DEBUG_VIDEO_OUTPUT_PATH_NAP_USB, W_OK) == 0))
        {
            pszVideoFilePath = ARSTREAM_READER2DEBUG_VIDEO_OUTPUT_PATH_NAP_USB;
        }
        else if ((access(ARSTREAM_READER2DEBUG_VIDEO_OUTPUT_PATH_NAP_INTERNAL, F_OK) == 0) && (access(ARSTREAM_READER2DEBUG_VIDEO_OUTPUT_PATH_NAP_INTERNAL, W_OK) == 0))
        {
            pszVideoFilePath = ARSTREAM_READER2DEBUG_VIDEO_OUTPUT_PATH_NAP_INTERNAL;
        }
        else if ((access(ARSTREAM_READER2DEBUG_VIDEO_OUTPUT_PATH_ANDROID_INTERNAL, F_OK) == 0) && (access(ARSTREAM_READER2DEBUG_VIDEO_OUTPUT_PATH_ANDROID_INTERNAL, W_OK) == 0))
        {
            pszVideoFilePath = ARSTREAM_READER2DEBUG_VIDEO_OUTPUT_PATH_ANDROID_INTERNAL;
        }
        else if ((access(ARSTREAM_READER2DEBUG_VIDEO_OUTPUT_PATH_PCLINUX, F_OK) == 0) && (access(ARSTREAM_READER2DEBUG_VIDEO_OUTPUT_PATH_PCLINUX, W_OK) == 0))
        {
            pszVideoFilePath = ARSTREAM_READER2DEBUG_VIDEO_OUTPUT_PATH_PCLINUX;
        }
        if (pszVideoFilePath)
        {
            for (i = 0; i < 100; i++)
            {
                snprintf(szVideoFileName, 128, "%s/stream_%02d.264", pszVideoFilePath, i);
                if (access(szVideoFileName, F_OK) == -1)
                {
                    // file does not exist
                    break;
                }
                szVideoFileName[0] = '\0';
            }
        }
    }

    if (strlen(szVideoFileName))
    {
        rdbg->videoFile = fopen(szVideoFileName, "wb");
        if (!rdbg->videoFile)
        {
            ARSAL_PRINT(ARSAL_PRINT_ERROR, ARSTREAM_READER2DEBUG_TAG, "unable to open video file '%s' in %s", szVideoFileName, __FUNCTION__);
        }
    }
    else
    {
        rdbg->videoFile = NULL;
    }
    
    rdbg->lastFrameIdx = 0;
    rdbg->previousFrameIndex = 0;
    
    return rdbg;
}


void ARSTREAM_Reader2Debug_Delete(ARSTREAM_Reader2Debug_t **rdbg)
{
    if ((!rdbg) || (!*rdbg))
    {
        return;
    }

    H264P_Free((*rdbg)->h264p);
    (*rdbg)->h264p = NULL;

    if ((*rdbg)->statsFile)
    {
        fclose((*rdbg)->statsFile);
    }

    if ((*rdbg)->videoFile)
    {
        fclose((*rdbg)->videoFile);
    }
    
    free((*rdbg));
    *rdbg = NULL;
}


void ARSTREAM_Reader2Debug_ProcessAu(ARSTREAM_Reader2Debug_t *rdbg, uint8_t* pAu, uint32_t auSize, uint64_t timestamp, uint64_t receptionTs, int missingPackets, int totalPackets)
{
    int ret;
    unsigned int offset = 0, nextStartCodeOffset = 0;
    void* pUserDataBuf;
    unsigned int userDataSize;
    double psnr = 0.0;
    uint64_t acquisitionTs = 0, systemTs = 0;
    int estimatedLostFrames = 0, curFrameIdx = 0;
    H264P_SliceInfo_t sliceInfo;
    H264P_ParrotDragonBasicUserDataSei_t parrotDragonBasicUserData;
    H264P_ParrotDragonExtendedUserDataSei_t parrotDragonExtendedUserData;
    H264P_ParrotUserDataSeiTypes_t parrotUserDataType = H264P_UNKNOWN_USER_DATA_SEI;

    if ((!rdbg) || (!pAu) || (!auSize))
    {
        return;
    }

    memset(&sliceInfo, 0, sizeof(sliceInfo));
    memset(&parrotDragonBasicUserData, 0, sizeof(parrotDragonBasicUserData));
    memset(&parrotDragonExtendedUserData, 0, sizeof(parrotDragonExtendedUserData));

    do
    {
        ret = H264P_ReadNextNalu_buffer(rdbg->h264p, pAu + offset, auSize - offset, &nextStartCodeOffset);
        if (ret < 0)
        {
            ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARSTREAM_READER2DEBUG_TAG, "H264P_ReadNextNalu_buffer() failed (%d)", ret);
            break;
        }
        offset += nextStartCodeOffset;
        ret = H264P_ParseNalu(rdbg->h264p);
        if (ret < 0)
        {
            ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARSTREAM_READER2DEBUG_TAG, "H264P_ParseNalu() failed (%d)", ret);
            break;
        }
    }
    while (nextStartCodeOffset);
    
    ret = H264P_GetSliceInfo(rdbg->h264p, &sliceInfo);
    if (ret >= 0)
    {
        if (sliceInfo.idrPicFlag)
        {
            if (rdbg->idrPicIdBase + sliceInfo.idr_pic_id < rdbg->curIdrPicId)
            {
                rdbg->idrPicIdBase = rdbg->curIdrPicId + 1;
            }
            rdbg->curIdrPicId = rdbg->idrPicIdBase + sliceInfo.idr_pic_id;
        }
        curFrameIdx = rdbg->curIdrPicId * 30 + sliceInfo.frame_num;
        rdbg->lastFrameIdx = curFrameIdx;
    }
    else
    {
        ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARSTREAM_READER2DEBUG_TAG, "H264P_GetSliceInfo() failed (%d)", ret);
    }

    ret = H264P_GetUserDataSei(rdbg->h264p, &pUserDataBuf, &userDataSize);
    if (ret >= 0)
    {
        parrotUserDataType = H264P_getParrotUserDataSeiType(pUserDataBuf, userDataSize);
        
        switch (parrotUserDataType)
        {
            case H264P_PARROT_DRAGON_BASIC_USER_DATA_SEI:
                ret = H264P_parseParrotDragonBasicUserDataSei(pUserDataBuf, userDataSize, &parrotDragonBasicUserData);
                if (ret >= 0)
                {
                    acquisitionTs = ((uint64_t)parrotDragonBasicUserData.acquisitionTsH << 32) + (uint64_t)parrotDragonBasicUserData.acquisitionTsL;
                    if (rdbg->previousFrameIndex != 0)
                    {
                        estimatedLostFrames = parrotDragonBasicUserData.frameIndex - rdbg->previousFrameIndex - 1;
                    }
                    rdbg->previousFrameIndex = parrotDragonBasicUserData.frameIndex;
                    if (parrotDragonBasicUserData.prevMse_fp8)
                    {
                        psnr = ARSTREAM_READER2DEBUG_PSNR_MAX - 10. * log10((double)parrotDragonBasicUserData.prevMse_fp8 / 256.);
                    }
                }
                else
                {
                    ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARSTREAM_READER2DEBUG_TAG, "H264P_parseParrotDragonBasicUserDataSei() failed (%d)", ret);
                }
                break;
            case H264P_PARROT_DRAGON_EXTENDED_USER_DATA_SEI:
                ret = H264P_parseParrotDragonExtendedUserDataSei(pUserDataBuf, userDataSize, &parrotDragonExtendedUserData);
                if (ret >= 0)
                {
                    acquisitionTs = ((uint64_t)parrotDragonExtendedUserData.acquisitionTsH << 32) + (uint64_t)parrotDragonExtendedUserData.acquisitionTsL;
                    systemTs = ((uint64_t)parrotDragonExtendedUserData.systemTsH << 32) + (uint64_t)parrotDragonExtendedUserData.systemTsL;
                    if (rdbg->previousFrameIndex != 0)
                    {
                        estimatedLostFrames = parrotDragonExtendedUserData.frameIndex - rdbg->previousFrameIndex - 1;
                    }
                    rdbg->previousFrameIndex = parrotDragonExtendedUserData.frameIndex;
                    if (parrotDragonExtendedUserData.prevMse_fp8)
                    {
                        psnr = ARSTREAM_READER2DEBUG_PSNR_MAX - 10. * log10((double)parrotDragonExtendedUserData.prevMse_fp8 / 256.);
                    }
                }
                else
                {
                    ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARSTREAM_READER2DEBUG_TAG, "H264P_parseParrotDragonExtendedUserDataSei() failed (%d)", ret);
                }
                break;
            default:
                break;
        }
    }
    else
    {
        ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARSTREAM_READER2DEBUG_TAG, "H264P_GetUserDataSei() failed (%d)", ret);
    }
    
    setlocale(LC_ALL, "C");
    switch (parrotUserDataType)
    {
        case H264P_PARROT_DRAGON_BASIC_USER_DATA_SEI:
            if (rdbg->statsFile)
            {
                fprintf(rdbg->statsFile, "%lu ", (long unsigned int)parrotDragonBasicUserData.frameIndex);
                fprintf(rdbg->statsFile, "%llu ", (long long unsigned int)acquisitionTs);
                fprintf(rdbg->statsFile, "%llu ", (long long unsigned int)timestamp);
                fprintf(rdbg->statsFile, "%llu ", (long long unsigned int)receptionTs);
                fprintf(rdbg->statsFile, "%llu ", (long long unsigned int)0);
                fprintf(rdbg->statsFile, "%lu %.3f %d %d %d ", 
                        (long unsigned int)auSize, 
                        psnr, 
                        estimatedLostFrames,
                        sliceInfo.idrPicFlag,
                        sliceInfo.sliceTypeMod5);
                fprintf(rdbg->statsFile, "%lu %.8f %.8f %.3f ", 
                        (long unsigned int)0, 0., 0., 0.);
                fprintf(rdbg->statsFile, "%.3f %.3f %.3f %.3f %.3f %.3f %.3f ", 
                        0., 0., 0., 0., 0., 0., 0.);
                fprintf(rdbg->statsFile, "%.3f %.3f %.3f %.3f %.3f ", 
                        0., 0., 0., 0., 0.);
                fprintf(rdbg->statsFile, "%d %d %d %d %d %d %d ", 
                        0, 0, 0, 0, 0, 0, 0);
                fprintf(rdbg->statsFile, "%lu %lu %lu %lu %lu ", 
                        (long unsigned int)0, (long unsigned int)0, (long unsigned int)0, (long unsigned int)0, (long unsigned int)0);
                fprintf(rdbg->statsFile, "%lu %lu %lu %lu %lu\n", 
                        (long unsigned int)0, (long unsigned int)0, (long unsigned int)0, (long unsigned int)0, (long unsigned int)0);
                fprintf(rdbg->statsFile, "%d %d\n", 
                        missingPackets, 
                        totalPackets);
                fflush(rdbg->statsFile);
            }
            break;
        case H264P_PARROT_DRAGON_EXTENDED_USER_DATA_SEI:
            if (rdbg->statsFile)
            {
                fprintf(rdbg->statsFile, "%lu ", (long unsigned int)parrotDragonExtendedUserData.frameIndex);
                fprintf(rdbg->statsFile, "%llu ", (long long unsigned int)acquisitionTs);
                fprintf(rdbg->statsFile, "%llu ", (long long unsigned int)timestamp);
                fprintf(rdbg->statsFile, "%llu ", (long long unsigned int)receptionTs);
                fprintf(rdbg->statsFile, "%llu ", (long long unsigned int)systemTs);
                fprintf(rdbg->statsFile, "%lu %.3f %d %d %d ", 
                        (long unsigned int)auSize,
                        psnr, 
                        estimatedLostFrames,
                        sliceInfo.idrPicFlag,
                        sliceInfo.sliceTypeMod5);
                fprintf(rdbg->statsFile, "%lu %.8f %.8f %.3f ", 
                        (long unsigned int)parrotDragonExtendedUserData.batteryPercentage, 
                        (double)parrotDragonExtendedUserData.latitude_fp20 / 1048576., 
                        (double)parrotDragonExtendedUserData.longitude_fp20 / 1048576., 
                        (double)parrotDragonExtendedUserData.altitude_fp16 / 65536.);
                fprintf(rdbg->statsFile, "%.3f %.3f %.3f %.3f %.3f %.3f %.3f ", 
                        (float)parrotDragonExtendedUserData.absoluteHeight_fp16 / 65536., 
                        (float)parrotDragonExtendedUserData.relativeHeight_fp16 / 65536., 
                        (float)parrotDragonExtendedUserData.xSpeed_fp16 / 65536., 
                        (float)parrotDragonExtendedUserData.ySpeed_fp16 / 65536., 
                        (float)parrotDragonExtendedUserData.zSpeed_fp16 / 65536., 
                        (float)parrotDragonExtendedUserData.distance_fp16 / 65536., 
                        (float)parrotDragonExtendedUserData.heading_fp16 / 65536.);
                fprintf(rdbg->statsFile, "%.3f %.3f %.3f %.3f %.3f ", 
                        (float)parrotDragonExtendedUserData.yaw_fp16 / 65536., 
                        (float)parrotDragonExtendedUserData.pitch_fp16 / 65536., 
                        (float)parrotDragonExtendedUserData.roll_fp16 / 65536., 
                        (float)parrotDragonExtendedUserData.cameraPan_fp16 / 65536., 
                        (float)parrotDragonExtendedUserData.cameraTilt_fp16 / 65536.);
                fprintf(rdbg->statsFile, "%d %d %d %d %d %d %d ", 
                        parrotDragonExtendedUserData.targetLiveVideoBitrate,
                        parrotDragonExtendedUserData.wifiRssi, 
                        parrotDragonExtendedUserData.wifiMcsRate, 
                        parrotDragonExtendedUserData.wifiTxRate,
                        parrotDragonExtendedUserData.wifiRxRate,
                        parrotDragonExtendedUserData.wifiTxFailRate,
                        parrotDragonExtendedUserData.wifiTxErrorRate);
                fprintf(rdbg->statsFile, "%lu %lu %lu %lu %lu ", 
                        (long unsigned int)parrotDragonExtendedUserData.postReprojTimestampDelta, 
                        (long unsigned int)parrotDragonExtendedUserData.postEeTimestampDelta, 
                        (long unsigned int)parrotDragonExtendedUserData.postScalingTimestampDelta, 
                        (long unsigned int)parrotDragonExtendedUserData.postLiveEncodingTimestampDelta, 
                        (long unsigned int)parrotDragonExtendedUserData.postNetworkTimestampDelta);
                fprintf(rdbg->statsFile, "%lu %lu %lu %lu %lu ", 
                        (long unsigned int)(((float)parrotDragonExtendedUserData.streamingMeanAcqToNetworkTime / (float)parrotDragonExtendedUserData.streamingMonitorTimeInterval) * 1000000.), 
                        (long unsigned int)(((float)parrotDragonExtendedUserData.streamingAcqToNetworkJitter / (float)parrotDragonExtendedUserData.streamingMonitorTimeInterval) * 1000000.), 
                        (long unsigned int)(((float)parrotDragonExtendedUserData.streamingBytesSent / (float)parrotDragonExtendedUserData.streamingMonitorTimeInterval) * 1000000.), 
                        (long unsigned int)(((float)parrotDragonExtendedUserData.streamingMeanPacketSize / (float)parrotDragonExtendedUserData.streamingMonitorTimeInterval) * 1000000.), 
                        (long unsigned int)(((float)parrotDragonExtendedUserData.streamingPacketSizeStdDev / (float)parrotDragonExtendedUserData.streamingMonitorTimeInterval) * 1000000.));
                fprintf(rdbg->statsFile, "%d %d\n", 
                        missingPackets, 
                        totalPackets);
                fflush(rdbg->statsFile);
            }
            break;
        default:
            if (rdbg->statsFile)
            {
                fprintf(rdbg->statsFile, "%lu ", (long unsigned int)0);
                fprintf(rdbg->statsFile, "%llu ", (long long unsigned int)0);
                fprintf(rdbg->statsFile, "%llu ", (long long unsigned int)timestamp);
                fprintf(rdbg->statsFile, "%llu ", (long long unsigned int)receptionTs);
                fprintf(rdbg->statsFile, "%llu ", (long long unsigned int)0);
                fprintf(rdbg->statsFile, "%lu %.3f %d %d %d ", 
                        (long unsigned int)auSize, 
                        0., 
                        0,
                        sliceInfo.idrPicFlag,
                        sliceInfo.sliceTypeMod5);
                fprintf(rdbg->statsFile, "%lu %.8f %.8f %.3f ", 
                        (long unsigned int)0, 0., 0., 0.);
                fprintf(rdbg->statsFile, "%.3f %.3f %.3f %.3f %.3f %.3f %.3f ", 
                        0., 0., 0., 0., 0., 0., 0.);
                fprintf(rdbg->statsFile, "%.3f %.3f %.3f %.3f %.3f ", 
                        0., 0., 0., 0., 0.);
                fprintf(rdbg->statsFile, "%d %d %d %d %d %d %d ", 
                        0, 0, 0, 0, 0, 0, 0);
                fprintf(rdbg->statsFile, "%lu %lu %lu %lu %lu ", 
                        (long unsigned int)0, (long unsigned int)0, (long unsigned int)0, (long unsigned int)0, (long unsigned int)0);
                fprintf(rdbg->statsFile, "%lu %lu %lu %lu %lu\n", 
                        (long unsigned int)0, (long unsigned int)0, (long unsigned int)0, (long unsigned int)0, (long unsigned int)0);
                fprintf(rdbg->statsFile, "%d %d\n", 
                        missingPackets, 
                        totalPackets);
                fflush(rdbg->statsFile);
            }
            break;
    }
    setlocale(LC_ALL, "");

    if (rdbg->videoFile)
    {
        fwrite(pAu, auSize, 1, rdbg->videoFile);
        fflush(rdbg->videoFile);
    }
}

