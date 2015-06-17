#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <arpa/inet.h>

#include "h264p_parrot.h"


int H264P_parseParrotDragonBasicUserDataSei(const void* pBuf, unsigned int bufSize, H264P_ParrotDragonBasicUserDataSei_t *userDataSei)
{
    const uint32_t* pdwBuf = (uint32_t*)pBuf;
    
    if (!pBuf)
    {
        return -1;
    }
    
    if (bufSize < sizeof(H264P_ParrotDragonBasicUserDataSei_t))
    {
        return -1;
    }

    userDataSei->uuid[0] = ntohl(*(pdwBuf++));
    userDataSei->uuid[1] = ntohl(*(pdwBuf++));
    userDataSei->uuid[2] = ntohl(*(pdwBuf++));
    userDataSei->uuid[3] = ntohl(*(pdwBuf++));
    if ((userDataSei->uuid[0] != H264P_PARROT_DRAGON_BASIC_USER_DATA_SEI_UUID_0) || (userDataSei->uuid[1] != H264P_PARROT_DRAGON_BASIC_USER_DATA_SEI_UUID_1) 
            || (userDataSei->uuid[2] != H264P_PARROT_DRAGON_BASIC_USER_DATA_SEI_UUID_2) || (userDataSei->uuid[3] != H264P_PARROT_DRAGON_BASIC_USER_DATA_SEI_UUID_3))
    {
        return -1;
    }
    
    userDataSei->frameIndex = ntohl(*(pdwBuf++));
    userDataSei->acquisitionTsH = ntohl(*(pdwBuf++));
    userDataSei->acquisitionTsL = ntohl(*(pdwBuf++));
    userDataSei->prevMse_fp8 = ntohl(*(pdwBuf++));
    
    return 0;
}


int H264P_parseParrotDragonExtendedUserDataSei(const void* pBuf, unsigned int bufSize, H264P_ParrotDragonExtendedUserDataSei_t *userDataSei)
{
    const uint32_t* pdwBuf = (uint32_t*)pBuf;
    const char* pszBuf;
    
    if (!pBuf)
    {
        return -1;
    }
    
    if (bufSize < sizeof(H264P_ParrotDragonExtendedUserDataSei_t))
    {
        return -1;
    }

    userDataSei->uuid[0] = ntohl(*(pdwBuf++));
    userDataSei->uuid[1] = ntohl(*(pdwBuf++));
    userDataSei->uuid[2] = ntohl(*(pdwBuf++));
    userDataSei->uuid[3] = ntohl(*(pdwBuf++));
    if ((userDataSei->uuid[0] != H264P_PARROT_DRAGON_EXTENDED_USER_DATA_SEI_UUID_0) || (userDataSei->uuid[1] != H264P_PARROT_DRAGON_EXTENDED_USER_DATA_SEI_UUID_1) 
            || (userDataSei->uuid[2] != H264P_PARROT_DRAGON_EXTENDED_USER_DATA_SEI_UUID_2) || (userDataSei->uuid[3] != H264P_PARROT_DRAGON_EXTENDED_USER_DATA_SEI_UUID_3))
    {
        return -1;
    }
    
    userDataSei->frameIndex = ntohl(*(pdwBuf++));
    userDataSei->acquisitionTsH = ntohl(*(pdwBuf++));
    userDataSei->acquisitionTsL = ntohl(*(pdwBuf++));
    userDataSei->prevMse_fp8 = ntohl(*(pdwBuf++));
    userDataSei->batteryPercentage = ntohl(*(pdwBuf++));
    userDataSei->latitude_fp20 = (int32_t)ntohl(*(pdwBuf++));
    userDataSei->longitude_fp20 = (int32_t)ntohl(*(pdwBuf++));
    userDataSei->altitude_fp16 = (int32_t)ntohl(*(pdwBuf++));
    userDataSei->absoluteHeight_fp16 = (int32_t)ntohl(*(pdwBuf++));
    userDataSei->relativeHeight_fp16 = (int32_t)ntohl(*(pdwBuf++));
    userDataSei->xSpeed_fp16 = (int32_t)ntohl(*(pdwBuf++));
    userDataSei->ySpeed_fp16 = (int32_t)ntohl(*(pdwBuf++));
    userDataSei->zSpeed_fp16 = (int32_t)ntohl(*(pdwBuf++));
    userDataSei->distance_fp16 = ntohl(*(pdwBuf++));
    userDataSei->heading_fp16 = (int32_t)ntohl(*(pdwBuf++));
    userDataSei->yaw_fp16 = (int32_t)ntohl(*(pdwBuf++));
    userDataSei->pitch_fp16 = (int32_t)ntohl(*(pdwBuf++));
    userDataSei->roll_fp16 = (int32_t)ntohl(*(pdwBuf++));
    userDataSei->cameraPan_fp16 = (int32_t)ntohl(*(pdwBuf++));
    userDataSei->cameraTilt_fp16 = (int32_t)ntohl(*(pdwBuf++));
    userDataSei->targetLiveVideoBitrate = ntohl(*(pdwBuf++));
    userDataSei->wifiRssi = (int32_t)ntohl(*(pdwBuf++));
    userDataSei->wifiMcsRate = ntohl(*(pdwBuf++));
    userDataSei->wifiTxRate = ntohl(*(pdwBuf++));
    userDataSei->wifiRxRate = ntohl(*(pdwBuf++));
    userDataSei->wifiTxFailRate = ntohl(*(pdwBuf++));
    userDataSei->wifiTxErrorRate = ntohl(*(pdwBuf++));
    userDataSei->postReprojTimestampDelta = ntohl(*(pdwBuf++));
    userDataSei->postEeTimestampDelta = ntohl(*(pdwBuf++));
    userDataSei->postScalingTimestampDelta = ntohl(*(pdwBuf++));
    userDataSei->postLiveEncodingTimestampDelta = ntohl(*(pdwBuf++));
    userDataSei->postNetworkTimestampDelta = ntohl(*(pdwBuf++));
    userDataSei->systemTsH = ntohl(*(pdwBuf++));
    userDataSei->systemTsL = ntohl(*(pdwBuf++));
    userDataSei->streamingMonitorTimeInterval = ntohl(*(pdwBuf++));
    userDataSei->streamingMeanAcqToNetworkTime = ntohl(*(pdwBuf++));
    userDataSei->streamingAcqToNetworkJitter = ntohl(*(pdwBuf++));
    userDataSei->streamingBytesSent = ntohl(*(pdwBuf++));
    userDataSei->streamingMeanPacketSize = ntohl(*(pdwBuf++));
    userDataSei->streamingPacketSizeStdDev = ntohl(*(pdwBuf++));
    userDataSei->streamingPacketsSent = ntohl(*(pdwBuf++));

    pszBuf = (char*)pdwBuf;
    strncpy(userDataSei->serialNumberH, pszBuf, H264P_PARROT_DRAGON_SERIAL_NUMBER_PART_LENGTH + 1);
    pszBuf += H264P_PARROT_DRAGON_SERIAL_NUMBER_PART_LENGTH + 1;
    strncpy(userDataSei->serialNumberL, pszBuf, H264P_PARROT_DRAGON_SERIAL_NUMBER_PART_LENGTH + 1);

    return 0;
}


H264P_ParrotUserDataSeiTypes_t H264P_getParrotUserDataSeiType(const void* pBuf, unsigned int bufSize)
{
    uint32_t uuid0, uuid1, uuid2, uuid3;
    
    if (!pBuf)
    {
        return -1;
    }
    
    if (bufSize < 16)
    {
        return -1;
    }

    uuid0 = ntohl(*((uint32_t*)pBuf));
    uuid1 = ntohl(*((uint32_t*)pBuf + 1));
    uuid2 = ntohl(*((uint32_t*)pBuf + 2));
    uuid3 = ntohl(*((uint32_t*)pBuf + 3));

    if ((uuid0 == H264P_PARROT_DRAGON_BASIC_USER_DATA_SEI_UUID_0) && (uuid1 == H264P_PARROT_DRAGON_BASIC_USER_DATA_SEI_UUID_1) 
            && (uuid2 == H264P_PARROT_DRAGON_BASIC_USER_DATA_SEI_UUID_2) && (uuid3 == H264P_PARROT_DRAGON_BASIC_USER_DATA_SEI_UUID_3))
    {
        return H264P_PARROT_DRAGON_BASIC_USER_DATA_SEI;
    }
    else if ((uuid0 == H264P_PARROT_DRAGON_EXTENDED_USER_DATA_SEI_UUID_0) && (uuid1 == H264P_PARROT_DRAGON_EXTENDED_USER_DATA_SEI_UUID_1) 
            && (uuid2 == H264P_PARROT_DRAGON_EXTENDED_USER_DATA_SEI_UUID_2) && (uuid3 == H264P_PARROT_DRAGON_EXTENDED_USER_DATA_SEI_UUID_3))
    {
        return H264P_PARROT_DRAGON_EXTENDED_USER_DATA_SEI;
    }
    else
    {
        return H264P_UNKNOWN_USER_DATA_SEI;
    }
}

