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


#define ARSTREAM_READER2DEBUG_RTPSTATS_OUTPUT_ALLOW_DRONE
#define ARSTREAM_READER2DEBUG_RTPSTATS_OUTPUT_PATH_DRONE "/data/ftp/internal_000/streamdebug"
#define ARSTREAM_READER2DEBUG_RTPSTATS_OUTPUT_ALLOW_NAP_USB
#define ARSTREAM_READER2DEBUG_RTPSTATS_OUTPUT_PATH_NAP_USB "/tmp/mnt/STREAMDEBUG/streamdebug"
//#define ARSTREAM_READER2DEBUG_RTPSTATS_OUTPUT_ALLOW_NAP_INTERNAL
#define ARSTREAM_READER2DEBUG_RTPSTATS_OUTPUT_PATH_NAP_INTERNAL "/data/skycontroller/streamdebug"
#define ARSTREAM_READER2DEBUG_RTPSTATS_OUTPUT_ALLOW_ANDROID_INTERNAL
#define ARSTREAM_READER2DEBUG_RTPSTATS_OUTPUT_PATH_ANDROID_INTERNAL "/storage/emulated/legacy/FF/streamdebug"
#define ARSTREAM_READER2DEBUG_RTPSTATS_OUTPUT_ALLOW_PCLINUX
#define ARSTREAM_READER2DEBUG_RTPSTATS_OUTPUT_PATH_PCLINUX "./streamdebug"

#define ARSTREAM_READER2DEBUG_RTPSTATS_OUTPUT_FILENAME "reader_monitor"

#define ARSTREAM_READER2DEBUG_FRAMESTATS_OUTPUT_ALLOW_DRONE
#define ARSTREAM_READER2DEBUG_FRAMESTATS_OUTPUT_PATH_DRONE "/data/ftp/internal_000/frameinfo"
#define ARSTREAM_READER2DEBUG_FRAMESTATS_OUTPUT_ALLOW_NAP_USB
#define ARSTREAM_READER2DEBUG_FRAMESTATS_OUTPUT_PATH_NAP_USB "/tmp/mnt/STREAMDEBUG/frameinfo"
//#define ARSTREAM_READER2DEBUG_FRAMESTATS_OUTPUT_ALLOW_NAP_INTERNAL
#define ARSTREAM_READER2DEBUG_FRAMESTATS_OUTPUT_PATH_NAP_INTERNAL "/data/skycontroller/frameinfo"
#define ARSTREAM_READER2DEBUG_FRAMESTATS_OUTPUT_ALLOW_ANDROID_INTERNAL
#define ARSTREAM_READER2DEBUG_FRAMESTATS_OUTPUT_PATH_ANDROID_INTERNAL "/storage/emulated/legacy/FF/frameinfo"
#define ARSTREAM_READER2DEBUG_FRAMESTATS_OUTPUT_ALLOW_PCLINUX
#define ARSTREAM_READER2DEBUG_FRAMESTATS_OUTPUT_PATH_PCLINUX "./frameinfo"

#define ARSTREAM_READER2DEBUG_FRAMESTATS_OUTPUT_FILENAME "frameinfo_recvstream"

#define ARSTREAM_READER2DEBUG_VIDEO_OUTPUT_ALLOW_DRONE
#define ARSTREAM_READER2DEBUG_VIDEO_OUTPUT_PATH_DRONE "/data/ftp/internal_000/stream"
#define ARSTREAM_READER2DEBUG_VIDEO_OUTPUT_ALLOW_NAP_USB
#define ARSTREAM_READER2DEBUG_VIDEO_OUTPUT_PATH_NAP_USB "/tmp/mnt/STREAMDEBUG/stream"
//#define ARSTREAM_READER2DEBUG_VIDEO_OUTPUT_ALLOW_NAP_INTERNAL
#define ARSTREAM_READER2DEBUG_VIDEO_OUTPUT_PATH_NAP_INTERNAL "/data/skycontroller/stream"
#define ARSTREAM_READER2DEBUG_VIDEO_OUTPUT_ALLOW_ANDROID_INTERNAL
#define ARSTREAM_READER2DEBUG_VIDEO_OUTPUT_PATH_ANDROID_INTERNAL "/storage/emulated/legacy/FF/stream"
#define ARSTREAM_READER2DEBUG_VIDEO_OUTPUT_ALLOW_PCLINUX
#define ARSTREAM_READER2DEBUG_VIDEO_OUTPUT_PATH_PCLINUX "./stream"

#define ARSTREAM_READER2DEBUG_VIDEO_OUTPUT_FILENAME "stream"


struct ARSTREAM_Reader2Debug_t {
    int curIdrPicId;
    int idrPicIdBase;
    int lastFrameIdx;
    int auSize;
    int auMissingPackets;
    int auTotalPackets;
    uint64_t lastReceptionTs;
    uint32_t previousFrameIndex;
    H264P_Handle h264p;
    FILE* rtpStatsFile;
    FILE* frameStatsFile;
    FILE* videoFile;
};


ARSTREAM_Reader2Debug_t* ARSTREAM_Reader2Debug_New(int outputRtpStatFile, int outputFrameStatFile, int outputVideoFile)
{
    int i;
    char szFileName[128];
    const char* pszFilePath = NULL;

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

    pszFilePath = NULL;
    szFileName[0] = '\0';
    if (outputRtpStatFile)
    {
        if (0)
        {
        }
#ifdef ARSTREAM_READER2DEBUG_RTPSTATS_OUTPUT_ALLOW_DRONE
        else if ((access(ARSTREAM_READER2DEBUG_RTPSTATS_OUTPUT_PATH_DRONE, F_OK) == 0) && (access(ARSTREAM_READER2DEBUG_RTPSTATS_OUTPUT_PATH_DRONE, W_OK) == 0))
        {
            pszFilePath = ARSTREAM_READER2DEBUG_RTPSTATS_OUTPUT_PATH_DRONE;
        }
#endif
#ifdef ARSTREAM_READER2DEBUG_RTPSTATS_OUTPUT_ALLOW_NAP_USB
        else if ((access(ARSTREAM_READER2DEBUG_RTPSTATS_OUTPUT_PATH_NAP_USB, F_OK) == 0) && (access(ARSTREAM_READER2DEBUG_RTPSTATS_OUTPUT_PATH_NAP_USB, W_OK) == 0))
        {
            pszFilePath = ARSTREAM_READER2DEBUG_RTPSTATS_OUTPUT_PATH_NAP_USB;
        }
#endif
#ifdef ARSTREAM_READER2DEBUG_RTPSTATS_OUTPUT_ALLOW_NAP_INTERNAL
        else if ((access(ARSTREAM_READER2DEBUG_RTPSTATS_OUTPUT_PATH_NAP_INTERNAL, F_OK) == 0) && (access(ARSTREAM_READER2DEBUG_RTPSTATS_OUTPUT_PATH_NAP_INTERNAL, W_OK) == 0))
        {
            pszFilePath = ARSTREAM_READER2DEBUG_RTPSTATS_OUTPUT_PATH_NAP_INTERNAL;
        }
#endif
#ifdef ARSTREAM_READER2DEBUG_RTPSTATS_OUTPUT_ALLOW_ANDROID_INTERNAL
        else if ((access(ARSTREAM_READER2DEBUG_RTPSTATS_OUTPUT_PATH_ANDROID_INTERNAL, F_OK) == 0) && (access(ARSTREAM_READER2DEBUG_RTPSTATS_OUTPUT_PATH_ANDROID_INTERNAL, W_OK) == 0))
        {
            pszFilePath = ARSTREAM_READER2DEBUG_RTPSTATS_OUTPUT_PATH_ANDROID_INTERNAL;
        }
#endif
#ifdef ARSTREAM_READER2DEBUG_RTPSTATS_OUTPUT_ALLOW_PCLINUX
        else if ((access(ARSTREAM_READER2DEBUG_RTPSTATS_OUTPUT_PATH_PCLINUX, F_OK) == 0) && (access(ARSTREAM_READER2DEBUG_RTPSTATS_OUTPUT_PATH_PCLINUX, W_OK) == 0))
        {
            pszFilePath = ARSTREAM_READER2DEBUG_RTPSTATS_OUTPUT_PATH_PCLINUX;
        }
#endif
        if (pszFilePath)
        {
            for (i = 0; i < 1000; i++)
            {
                snprintf(szFileName, 128, "%s/%s_%03d.dat", pszFilePath, ARSTREAM_READER2DEBUG_RTPSTATS_OUTPUT_FILENAME, i);
                if (access(szFileName, F_OK) == -1)
                {
                    // file does not exist
                    break;
                }
                szFileName[0] = '\0';
            }
        }
    }

    if (strlen(szFileName))
    {
        rdbg->rtpStatsFile = fopen(szFileName, "w");
        if (!rdbg->rtpStatsFile)
        {
            ARSAL_PRINT(ARSAL_PRINT_ERROR, ARSTREAM_READER2DEBUG_TAG, "unable to open stats file '%s' in %s", szFileName, __FUNCTION__);
        }
    }
    else
    {
        rdbg->rtpStatsFile = NULL;
    }
    if (rdbg->rtpStatsFile)
    {
        fprintf(rdbg->rtpStatsFile, "recvTimestamp rtpTimestamp rtpSeqNum rtpMarkerBit bytes\n");
    }

    pszFilePath = NULL;
    szFileName[0] = '\0';
    if (outputFrameStatFile)
    {
        if (0)
        {
        }
#ifdef ARSTREAM_READER2DEBUG_FRAMESTATS_OUTPUT_ALLOW_DRONE
        else if ((access(ARSTREAM_READER2DEBUG_FRAMESTATS_OUTPUT_PATH_DRONE, F_OK) == 0) && (access(ARSTREAM_READER2DEBUG_FRAMESTATS_OUTPUT_PATH_DRONE, W_OK) == 0))
        {
            pszFilePath = ARSTREAM_READER2DEBUG_FRAMESTATS_OUTPUT_PATH_DRONE;
        }
#endif
#ifdef ARSTREAM_READER2DEBUG_FRAMESTATS_OUTPUT_ALLOW_NAP_USB
        else if ((access(ARSTREAM_READER2DEBUG_FRAMESTATS_OUTPUT_PATH_NAP_USB, F_OK) == 0) && (access(ARSTREAM_READER2DEBUG_FRAMESTATS_OUTPUT_PATH_NAP_USB, W_OK) == 0))
        {
            pszFilePath = ARSTREAM_READER2DEBUG_FRAMESTATS_OUTPUT_PATH_NAP_USB;
        }
#endif
#ifdef ARSTREAM_READER2DEBUG_FRAMESTATS_OUTPUT_ALLOW_NAP_INTERNAL
        else if ((access(ARSTREAM_READER2DEBUG_FRAMESTATS_OUTPUT_PATH_NAP_INTERNAL, F_OK) == 0) && (access(ARSTREAM_READER2DEBUG_FRAMESTATS_OUTPUT_PATH_NAP_INTERNAL, W_OK) == 0))
        {
            pszFilePath = ARSTREAM_READER2DEBUG_FRAMESTATS_OUTPUT_PATH_NAP_INTERNAL;
        }
#endif
#ifdef ARSTREAM_READER2DEBUG_FRAMESTATS_OUTPUT_ALLOW_ANDROID_INTERNAL
        else if ((access(ARSTREAM_READER2DEBUG_FRAMESTATS_OUTPUT_PATH_ANDROID_INTERNAL, F_OK) == 0) && (access(ARSTREAM_READER2DEBUG_FRAMESTATS_OUTPUT_PATH_ANDROID_INTERNAL, W_OK) == 0))
        {
            pszFilePath = ARSTREAM_READER2DEBUG_FRAMESTATS_OUTPUT_PATH_ANDROID_INTERNAL;
        }
#endif
#ifdef ARSTREAM_READER2DEBUG_FRAMESTATS_OUTPUT_ALLOW_PCLINUX
        else if ((access(ARSTREAM_READER2DEBUG_FRAMESTATS_OUTPUT_PATH_PCLINUX, F_OK) == 0) && (access(ARSTREAM_READER2DEBUG_FRAMESTATS_OUTPUT_PATH_PCLINUX, W_OK) == 0))
        {
            pszFilePath = ARSTREAM_READER2DEBUG_FRAMESTATS_OUTPUT_PATH_PCLINUX;
        }
#endif
        if (pszFilePath)
        {
            for (i = 0; i < 1000; i++)
            {
                snprintf(szFileName, 128, "%s/%s_%03d.dat", pszFilePath, ARSTREAM_READER2DEBUG_FRAMESTATS_OUTPUT_FILENAME, i);
                if (access(szFileName, F_OK) == -1)
                {
                    // file does not exist
                    break;
                }
                szFileName[0] = '\0';
            }
        }
    }

    if (strlen(szFileName))
    {
        rdbg->frameStatsFile = fopen(szFileName, "w");
        if (!rdbg->frameStatsFile)
        {
            ARSAL_PRINT(ARSAL_PRINT_ERROR, ARSTREAM_READER2DEBUG_TAG, "unable to open stats file '%s' in %s", szFileName, __FUNCTION__);
        }
    }
    else
    {
        rdbg->frameStatsFile = NULL;
    }
    if (rdbg->frameStatsFile)
    {
        fprintf(rdbg->frameStatsFile, "frameIndex acquisitionTs rtpTs receptionTs systemTs frameSize estimatedLostFrames ");
        fprintf(rdbg->frameStatsFile, "batteryPercentage latitude longitude altitude absoluteHeight relativeHeight xSpeed ySpeed zSpeed distance yaw pitch roll cameraPan cameraTilt ");
        fprintf(rdbg->frameStatsFile, "videoStreamingTargetBitrate videoStreamingDecimation videoStreamingGopLength videoStreamingFrameType videoStreamingFrameSize videoStreamingFramePsnrY ");
        fprintf(rdbg->frameStatsFile, "videoRecordingFrameType videoRecordingFrameSize videoRecordingFramePsnrY ");
        fprintf(rdbg->frameStatsFile, "wifiRssi wifiMcsRate wifiTxRate wifiRxRate wifiTxFailRate wifiTxErrorRate ");
        fprintf(rdbg->frameStatsFile, "preReprojTimestampDelta postReprojTimestampDelta postEeTimestampDelta postScalingTimestampDelta postStreamingEncodingTimestampDelta postRecordingEncodingTimestampDelta postNetworkTimestampDelta ");
        fprintf(rdbg->frameStatsFile, "streamingSrcMonitorTimeInterval streamingSrcMeanAcqToNetworkTime streamingSrcAcqToNetworkJitter streamingSrcMeanNetworkTime streamingSrcNetworkJitter ");
        fprintf(rdbg->frameStatsFile, "streamingSrcBytesSent streamingSrcMeanPacketSize streamingSrcPacketSizeStdDev streamingSrcPacketsSent streamingSrcBytesDropped streamingSrcNaluDropped ");
        fprintf(rdbg->frameStatsFile, "streamingSinkMissingPackets streamingSinkTotalPackets\n");
    }

    pszFilePath = NULL;
    szFileName[0] = '\0';
    if (outputVideoFile)
    {
        if (0)
        {
        }
#ifdef ARSTREAM_READER2DEBUG_VIDEO_OUTPUT_ALLOW_DRONE
        else if ((access(ARSTREAM_READER2DEBUG_VIDEO_OUTPUT_PATH_DRONE, F_OK) == 0) && (access(ARSTREAM_READER2DEBUG_VIDEO_OUTPUT_PATH_DRONE, W_OK) == 0))
        {
            pszFilePath = ARSTREAM_READER2DEBUG_VIDEO_OUTPUT_PATH_DRONE;
        }
#endif
#ifdef ARSTREAM_READER2DEBUG_VIDEO_OUTPUT_ALLOW_NAP_USB
        else if ((access(ARSTREAM_READER2DEBUG_VIDEO_OUTPUT_PATH_NAP_USB, F_OK) == 0) && (access(ARSTREAM_READER2DEBUG_VIDEO_OUTPUT_PATH_NAP_USB, W_OK) == 0))
        {
            pszFilePath = ARSTREAM_READER2DEBUG_VIDEO_OUTPUT_PATH_NAP_USB;
        }
#endif
#ifdef ARSTREAM_READER2DEBUG_VIDEO_OUTPUT_ALLOW_NAP_INTERNAL
        else if ((access(ARSTREAM_READER2DEBUG_VIDEO_OUTPUT_PATH_NAP_INTERNAL, F_OK) == 0) && (access(ARSTREAM_READER2DEBUG_VIDEO_OUTPUT_PATH_NAP_INTERNAL, W_OK) == 0))
        {
            pszFilePath = ARSTREAM_READER2DEBUG_VIDEO_OUTPUT_PATH_NAP_INTERNAL;
        }
#endif
#ifdef ARSTREAM_READER2DEBUG_VIDEO_OUTPUT_ALLOW_ANDROID_INTERNAL
        else if ((access(ARSTREAM_READER2DEBUG_VIDEO_OUTPUT_PATH_ANDROID_INTERNAL, F_OK) == 0) && (access(ARSTREAM_READER2DEBUG_VIDEO_OUTPUT_PATH_ANDROID_INTERNAL, W_OK) == 0))
        {
            pszFilePath = ARSTREAM_READER2DEBUG_VIDEO_OUTPUT_PATH_ANDROID_INTERNAL;
        }
#endif
#ifdef ARSTREAM_READER2DEBUG_VIDEO_OUTPUT_ALLOW_PCLINUX
        else if ((access(ARSTREAM_READER2DEBUG_VIDEO_OUTPUT_PATH_PCLINUX, F_OK) == 0) && (access(ARSTREAM_READER2DEBUG_VIDEO_OUTPUT_PATH_PCLINUX, W_OK) == 0))
        {
            pszFilePath = ARSTREAM_READER2DEBUG_VIDEO_OUTPUT_PATH_PCLINUX;
        }
#endif
        if (pszFilePath)
        {
            for (i = 0; i < 1000; i++)
            {
                snprintf(szFileName, 128, "%s/%s_%03d.264", pszFilePath, ARSTREAM_READER2DEBUG_VIDEO_OUTPUT_FILENAME, i);
                if (access(szFileName, F_OK) == -1)
                {
                    // file does not exist
                    break;
                }
                szFileName[0] = '\0';
            }
        }
    }

    if (strlen(szFileName))
    {
        rdbg->videoFile = fopen(szFileName, "wb");
        if (!rdbg->videoFile)
        {
            ARSAL_PRINT(ARSAL_PRINT_ERROR, ARSTREAM_READER2DEBUG_TAG, "unable to open video file '%s' in %s", szFileName, __FUNCTION__);
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

    if ((*rdbg)->rtpStatsFile)
    {
        fclose((*rdbg)->rtpStatsFile);
    }

    if ((*rdbg)->frameStatsFile)
    {
        fclose((*rdbg)->frameStatsFile);
    }

    if ((*rdbg)->videoFile)
    {
        fclose((*rdbg)->videoFile);
    }
    
    free((*rdbg));
    *rdbg = NULL;
}


static void ARSTREAM_Reader2Debug_OutputAu(ARSTREAM_Reader2Debug_t *rdbg, uint32_t auSize, uint64_t timestamp, uint64_t receptionTs, int missingPackets, int totalPackets)
{
    int ret;
    void* pUserDataBuf;
    unsigned int userDataSize;
    double psnr = 0.0;
    uint64_t acquisitionTs = 0, systemTs = 0;
    int estimatedLostFrames = 0, curFrameIdx = 0;
    H264P_SliceInfo_t sliceInfo;
    H264P_ParrotDragonBasicUserDataSeiV2_t parrotDragonBasicUserData;
    H264P_ParrotDragonExtendedUserDataSeiV2_t parrotDragonExtendedUserData;
    H264P_ParrotUserDataSeiTypes_t parrotUserDataType = H264P_UNKNOWN_USER_DATA_SEI;

    memset(&sliceInfo, 0, sizeof(sliceInfo));
    memset(&parrotDragonBasicUserData, 0, sizeof(parrotDragonBasicUserData));
    memset(&parrotDragonExtendedUserData, 0, sizeof(parrotDragonExtendedUserData));

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
            case H264P_PARROT_DRAGON_BASIC_USER_DATA_SEI_V2:
                ret = H264P_parseParrotDragonBasicUserDataSeiV2(pUserDataBuf, userDataSize, &parrotDragonBasicUserData);
                if (ret >= 0)
                {
                    acquisitionTs = ((uint64_t)parrotDragonBasicUserData.acquisitionTsH << 32) + (uint64_t)parrotDragonBasicUserData.acquisitionTsL;
                    if (rdbg->previousFrameIndex != 0)
                    {
                        estimatedLostFrames = parrotDragonBasicUserData.frameIndex - rdbg->previousFrameIndex - 1;
                    }
                    rdbg->previousFrameIndex = parrotDragonBasicUserData.frameIndex;
                }
                else
                {
                    ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARSTREAM_READER2DEBUG_TAG, "H264P_parseParrotDragonBasicUserDataSeiV2() failed (%d)", ret);
                }
                break;
            case H264P_PARROT_DRAGON_EXTENDED_USER_DATA_SEI_V2:
                ret = H264P_parseParrotDragonExtendedUserDataSeiV2(pUserDataBuf, userDataSize, &parrotDragonExtendedUserData);
                if (ret >= 0)
                {
                    acquisitionTs = ((uint64_t)parrotDragonExtendedUserData.acquisitionTsH << 32) + (uint64_t)parrotDragonExtendedUserData.acquisitionTsL;
                    systemTs = ((uint64_t)parrotDragonExtendedUserData.systemTsH << 32) + (uint64_t)parrotDragonExtendedUserData.systemTsL;
                    if (rdbg->previousFrameIndex != 0)
                    {
                        estimatedLostFrames = parrotDragonExtendedUserData.frameIndex - rdbg->previousFrameIndex - 1;
                    }
                    rdbg->previousFrameIndex = parrotDragonExtendedUserData.frameIndex;
                    if (parrotDragonExtendedUserData.videoStreamingPrevFrameMseY_fp8)
                    {
                        psnr = ARSTREAM_READER2DEBUG_PSNR_MAX - 10. * log10((double)parrotDragonExtendedUserData.videoStreamingPrevFrameMseY_fp8 / 256.);
                    }
                }
                else
                {
                    ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARSTREAM_READER2DEBUG_TAG, "H264P_parseParrotDragonExtendedUserDataSeiV2() failed (%d)", ret);
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
        case H264P_PARROT_DRAGON_BASIC_USER_DATA_SEI_V2:
            fprintf(rdbg->frameStatsFile, "%lu ", (long unsigned int)parrotDragonBasicUserData.frameIndex);
            fprintf(rdbg->frameStatsFile, "%llu ", (long long unsigned int)acquisitionTs);
            fprintf(rdbg->frameStatsFile, "%llu ", (long long unsigned int)timestamp);
            fprintf(rdbg->frameStatsFile, "%llu ", (long long unsigned int)receptionTs);
            fprintf(rdbg->frameStatsFile, "%llu ", (long long unsigned int)0);
            fprintf(rdbg->frameStatsFile, "%lu %d ", 
                    (long unsigned int)auSize, 
                    estimatedLostFrames);
            fprintf(rdbg->frameStatsFile, "%lu %.8f %.8f %.3f ", 
                    (long unsigned int)0, 0., 0., 0.);
            fprintf(rdbg->frameStatsFile, "%.3f %.3f %.3f %.3f %.3f %.3f ", 
                    0., 0., 0., 0., 0., 0.);
            fprintf(rdbg->frameStatsFile, "%.3f %.3f %.3f %.3f %.3f ", 
                    0., 0., 0., 0., 0.);
            fprintf(rdbg->frameStatsFile, "%d %d %d %d %d %.3f ", 
                    0, 0, 0, 0, 0, 0.);
            fprintf(rdbg->frameStatsFile, "%d %d %.3f ", 
                    0, 0, 0.);
            fprintf(rdbg->frameStatsFile, "%d %d %d %d %d %d ", 
                    0, 0, 0, 0, 0, 0);
            fprintf(rdbg->frameStatsFile, "%lu %lu %lu %lu %lu %lu %lu ", 
                    (long unsigned int)0, (long unsigned int)0, (long unsigned int)0, (long unsigned int)0, (long unsigned int)0, (long unsigned int)0, (long unsigned int)0);
            fprintf(rdbg->frameStatsFile, "%lu ", 
                    (long unsigned int)0);
            fprintf(rdbg->frameStatsFile, "%lu %lu %lu %lu %lu %lu %lu %lu %lu %lu ", 
                    (long unsigned int)0, (long unsigned int)0, (long unsigned int)0, (long unsigned int)0, (long unsigned int)0, (long unsigned int)0, (long unsigned int)0, (long unsigned int)0, (long unsigned int)0, (long unsigned int)0);
            fprintf(rdbg->frameStatsFile, "%d %d\n", 
                    missingPackets, 
                    totalPackets);
            fflush(rdbg->frameStatsFile);
            break;
        case H264P_PARROT_DRAGON_EXTENDED_USER_DATA_SEI_V2:
            fprintf(rdbg->frameStatsFile, "%lu ", (long unsigned int)parrotDragonExtendedUserData.frameIndex);
            fprintf(rdbg->frameStatsFile, "%llu ", (long long unsigned int)acquisitionTs);
            fprintf(rdbg->frameStatsFile, "%llu ", (long long unsigned int)timestamp);
            fprintf(rdbg->frameStatsFile, "%llu ", (long long unsigned int)receptionTs);
            fprintf(rdbg->frameStatsFile, "%llu ", (long long unsigned int)systemTs);
            fprintf(rdbg->frameStatsFile, "%lu %d ", 
                    (long unsigned int)auSize,
                    estimatedLostFrames);
            fprintf(rdbg->frameStatsFile, "%lu %.8f %.8f %.3f ", 
                    (long unsigned int)parrotDragonExtendedUserData.batteryPercentage, 
                    (double)parrotDragonExtendedUserData.latitude_fp20 / 1048576., 
                    (double)parrotDragonExtendedUserData.longitude_fp20 / 1048576., 
                    (double)parrotDragonExtendedUserData.altitude_fp16 / 65536.);
            fprintf(rdbg->frameStatsFile, "%.3f %.3f %.3f %.3f %.3f %.3f ", 
                    (float)parrotDragonExtendedUserData.absoluteHeight_fp16 / 65536., 
                    (float)parrotDragonExtendedUserData.relativeHeight_fp16 / 65536., 
                    (float)parrotDragonExtendedUserData.xSpeed_fp16 / 65536., 
                    (float)parrotDragonExtendedUserData.ySpeed_fp16 / 65536., 
                    (float)parrotDragonExtendedUserData.zSpeed_fp16 / 65536., 
                    (float)parrotDragonExtendedUserData.distance_fp16 / 65536.);
            fprintf(rdbg->frameStatsFile, "%.3f %.3f %.3f %.3f %.3f ", 
                    (float)parrotDragonExtendedUserData.yaw_fp16 / 65536., 
                    (float)parrotDragonExtendedUserData.pitch_fp16 / 65536., 
                    (float)parrotDragonExtendedUserData.roll_fp16 / 65536., 
                    (float)parrotDragonExtendedUserData.cameraPan_fp16 / 65536., 
                    (float)parrotDragonExtendedUserData.cameraTilt_fp16 / 65536.);
            fprintf(rdbg->frameStatsFile, "%d %d %d %d %d %.3f ", 
                    parrotDragonExtendedUserData.videoStreamingTargetBitrate,
                    parrotDragonExtendedUserData.videoStreamingDecimation,
                    parrotDragonExtendedUserData.videoStreamingGopLength,
                    parrotDragonExtendedUserData.videoStreamingPrevFrameType,
                    parrotDragonExtendedUserData.videoStreamingPrevFrameSize,
                    psnr);
            fprintf(rdbg->frameStatsFile, "%d %d %.3f ", 
                    parrotDragonExtendedUserData.videoRecordingPrevFrameType,
                    parrotDragonExtendedUserData.videoRecordingPrevFrameSize,
                    0.);
            fprintf(rdbg->frameStatsFile, "%d %d %d %d %d %d ", 
                    parrotDragonExtendedUserData.wifiRssi, 
                    parrotDragonExtendedUserData.wifiMcsRate, 
                    parrotDragonExtendedUserData.wifiTxRate,
                    parrotDragonExtendedUserData.wifiRxRate,
                    parrotDragonExtendedUserData.wifiTxFailRate,
                    parrotDragonExtendedUserData.wifiTxErrorRate);
            fprintf(rdbg->frameStatsFile, "%lu %lu %lu %lu %lu %lu %lu ", 
                    (long unsigned int)parrotDragonExtendedUserData.preReprojTimestampDelta, 
                    (long unsigned int)parrotDragonExtendedUserData.postReprojTimestampDelta, 
                    (long unsigned int)parrotDragonExtendedUserData.postEeTimestampDelta, 
                    (long unsigned int)parrotDragonExtendedUserData.postScalingTimestampDelta, 
                    (long unsigned int)parrotDragonExtendedUserData.postStreamingEncodingTimestampDelta, 
                    (long unsigned int)parrotDragonExtendedUserData.postRecordingEncodingTimestampDelta, 
                    (long unsigned int)parrotDragonExtendedUserData.postNetworkInputTimestampDelta);
            fprintf(rdbg->frameStatsFile, "%lu ", 
                    (long unsigned int)parrotDragonExtendedUserData.streamingMonitorTimeInterval);
            if (parrotDragonExtendedUserData.streamingMonitorTimeInterval)
            {
                fprintf(rdbg->frameStatsFile, "%lu %lu %lu %lu %lu %lu %lu %lu %lu %lu ", 
                        (long unsigned int)(((float)parrotDragonExtendedUserData.streamingMeanAcqToNetworkTime / (float)parrotDragonExtendedUserData.streamingMonitorTimeInterval) * 1000000.), 
                        (long unsigned int)(((float)parrotDragonExtendedUserData.streamingAcqToNetworkJitter / (float)parrotDragonExtendedUserData.streamingMonitorTimeInterval) * 1000000.), 
                        (long unsigned int)(((float)parrotDragonExtendedUserData.streamingMeanNetworkTime / (float)parrotDragonExtendedUserData.streamingMonitorTimeInterval) * 1000000.), 
                        (long unsigned int)(((float)parrotDragonExtendedUserData.streamingNetworkJitter / (float)parrotDragonExtendedUserData.streamingMonitorTimeInterval) * 1000000.), 
                        (long unsigned int)(((float)parrotDragonExtendedUserData.streamingBytesSent / (float)parrotDragonExtendedUserData.streamingMonitorTimeInterval) * 1000000.), 
                        (long unsigned int)(((float)parrotDragonExtendedUserData.streamingMeanPacketSize / (float)parrotDragonExtendedUserData.streamingMonitorTimeInterval) * 1000000.), 
                        (long unsigned int)(((float)parrotDragonExtendedUserData.streamingPacketSizeStdDev / (float)parrotDragonExtendedUserData.streamingMonitorTimeInterval) * 1000000.), 
                        (long unsigned int)(((float)parrotDragonExtendedUserData.streamingPacketsSent / (float)parrotDragonExtendedUserData.streamingMonitorTimeInterval) * 1000000.), 
                        (long unsigned int)(((float)parrotDragonExtendedUserData.streamingBytesDropped / (float)parrotDragonExtendedUserData.streamingMonitorTimeInterval) * 1000000.), 
                        (long unsigned int)(((float)parrotDragonExtendedUserData.streamingNaluDropped / (float)parrotDragonExtendedUserData.streamingMonitorTimeInterval) * 1000000.));
            }
            else
            {
                fprintf(rdbg->frameStatsFile, "%lu %lu %lu %lu %lu %lu %lu %lu %lu %lu ", 
                        (long unsigned int)0, (long unsigned int)0, (long unsigned int)0, (long unsigned int)0, (long unsigned int)0, (long unsigned int)0, (long unsigned int)0, (long unsigned int)0, (long unsigned int)0, (long unsigned int)0);
            }
            fprintf(rdbg->frameStatsFile, "%d %d\n", 
                    missingPackets, 
                    totalPackets);
            fflush(rdbg->frameStatsFile);
            break;
        default:
            fprintf(rdbg->frameStatsFile, "%lu ", (long unsigned int)0);
            fprintf(rdbg->frameStatsFile, "%llu ", (long long unsigned int)0);
            fprintf(rdbg->frameStatsFile, "%llu ", (long long unsigned int)timestamp);
            fprintf(rdbg->frameStatsFile, "%llu ", (long long unsigned int)receptionTs);
            fprintf(rdbg->frameStatsFile, "%llu ", (long long unsigned int)0);
            fprintf(rdbg->frameStatsFile, "%lu %d ", 
                    (long unsigned int)auSize, 
                    0);
            fprintf(rdbg->frameStatsFile, "%lu %.8f %.8f %.3f ", 
                    (long unsigned int)0, 0., 0., 0.);
            fprintf(rdbg->frameStatsFile, "%.3f %.3f %.3f %.3f %.3f %.3f ", 
                    0., 0., 0., 0., 0., 0.);
            fprintf(rdbg->frameStatsFile, "%.3f %.3f %.3f %.3f %.3f ", 
                    0., 0., 0., 0., 0.);
            fprintf(rdbg->frameStatsFile, "%d %d %d %d %d %.3f ", 
                    0, 0, 0, 0, 0, 0.);
            fprintf(rdbg->frameStatsFile, "%d %d %.3f ", 
                    0, 0, 0.);
            fprintf(rdbg->frameStatsFile, "%d %d %d %d %d %d ", 
                    0, 0, 0, 0, 0, 0);
            fprintf(rdbg->frameStatsFile, "%lu %lu %lu %lu %lu %lu %lu ", 
                    (long unsigned int)0, (long unsigned int)0, (long unsigned int)0, (long unsigned int)0, (long unsigned int)0, (long unsigned int)0, (long unsigned int)0);
            fprintf(rdbg->frameStatsFile, "%lu ", 
                    (long unsigned int)0);
            fprintf(rdbg->frameStatsFile, "%lu %lu %lu %lu %lu %lu %lu %lu %lu %lu ", 
                    (long unsigned int)0, (long unsigned int)0, (long unsigned int)0, (long unsigned int)0, (long unsigned int)0, (long unsigned int)0, (long unsigned int)0, (long unsigned int)0, (long unsigned int)0, (long unsigned int)0);
            fprintf(rdbg->frameStatsFile, "%d %d\n", 
                    missingPackets, 
                    totalPackets);
            fflush(rdbg->frameStatsFile);
            break;
    }
    setlocale(LC_ALL, "");
}


void ARSTREAM_Reader2Debug_ProcessAu(ARSTREAM_Reader2Debug_t *rdbg, uint8_t* pAu, uint32_t auSize, uint64_t timestamp, uint64_t receptionTs, int missingPackets, int totalPackets)
{
    int ret;
    unsigned int offset = 0, nextStartCodeOffset = 0;

    if ((!rdbg) || (!pAu) || (!auSize))
    {
        return;
    }

    if (rdbg->videoFile)
    {
        fwrite(pAu, auSize, 1, rdbg->videoFile);
        fflush(rdbg->videoFile);
    }

    if (!rdbg->frameStatsFile)
    {
        return;
    }

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
    
    ARSTREAM_Reader2Debug_OutputAu(rdbg, auSize, timestamp, receptionTs, missingPackets, totalPackets);
}


void ARSTREAM_Reader2Debug_ProcessNalu(ARSTREAM_Reader2Debug_t *rdbg, uint8_t* pNalu, uint32_t naluSize, uint64_t timestamp, uint64_t receptionTs, int missingPacketsBefore, int totalPackets, int isFirstNaluInAu, int isLastNaluInAu)
{
    int ret;

    if ((!rdbg) || (!pNalu) || (!naluSize))
    {
        return;
    }

    if (rdbg->videoFile)
    {
        fwrite(pNalu, naluSize, 1, rdbg->videoFile);
        fflush(rdbg->videoFile);
    }

    if (!rdbg->frameStatsFile)
    {
        return;
    }

    if ((isFirstNaluInAu) && (rdbg->auSize > 0))
    {
        ARSTREAM_Reader2Debug_OutputAu(rdbg, rdbg->auSize, timestamp, rdbg->lastReceptionTs, rdbg->auMissingPackets, rdbg->auTotalPackets);
        rdbg->auSize = 0;
        rdbg->auMissingPackets = 0;
        rdbg->auTotalPackets = 0;
    }

    rdbg->auSize += naluSize;
    rdbg->lastReceptionTs = receptionTs;
    rdbg->auMissingPackets += missingPacketsBefore;
    rdbg->auTotalPackets += totalPackets;

    ret = H264P_ReadNextNalu_buffer(rdbg->h264p, pNalu, naluSize, NULL);
    if (ret < 0)
    {
        ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARSTREAM_READER2DEBUG_TAG, "H264P_ReadNextNalu_buffer() failed (%d)", ret);
    }
    else
    {
        ret = H264P_ParseNalu(rdbg->h264p);
        if (ret < 0)
        {
            ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARSTREAM_READER2DEBUG_TAG, "H264P_ParseNalu() failed (%d)", ret);
        }
    }

    if (isLastNaluInAu)
    {
        ARSTREAM_Reader2Debug_OutputAu(rdbg, rdbg->auSize, timestamp, rdbg->lastReceptionTs, rdbg->auMissingPackets, rdbg->auTotalPackets);
        rdbg->auSize = 0;
        rdbg->auMissingPackets = 0;
        rdbg->auTotalPackets = 0;
    }
}


void ARSTREAM_Reader2Debug_ProcessPacket(ARSTREAM_Reader2Debug_t *rdbg, uint64_t recvTimestamp, uint32_t timestamp, uint16_t seqNum, uint16_t markerBit, uint32_t bytes)
{
    if (!rdbg)
    {
        return;
    }

    if (!rdbg->rtpStatsFile)
    {
        return;
    }

    fprintf(rdbg->rtpStatsFile, "%llu %lu %u %u %lu\n", (long long unsigned int)recvTimestamp, (long unsigned int)timestamp, seqNum, markerBit, (long unsigned int)bytes);
}

