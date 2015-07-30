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
#include <jni.h>
#include <libARStream/ARSTREAM_Reader2.h>
#include <libARSAL/ARSAL_Print.h>
#include "ARSTREAM_JNI.h"


#define JNI_READER_TAG "ARSTREAM_JNIReader2Resender"


JNIEXPORT jlong JNICALL
Java_com_parrot_arsdk_arstream_ARStreamReader2Resender_nativeConstructor (JNIEnv *env, jobject thizz, jlong cReader, jstring targetAddress)
{
    eARSTREAM_ERROR err = ARSTREAM_OK;
    const char *c_targetAddress = (*env)->GetStringUTFChars (env, targetAddress, NULL);

    ARSTREAM_Reader2_Resender_Config_t config;
    config.ifaceAddr = NULL;
    config.sendAddr = c_targetAddress;
    config.sendPort = ARSTREAM2_PORT;
    config.maxPacketSize = ARSTREAM2_MAX_PACKET_SIZE;
    config.targetPacketSize = ARSTREAM2_RESENDER_TARGET_PACKET_SIZE;
    config.maxBitrate = ARSTREAM2_RESENDER_MAX_BITRATE;
    config.maxLatencyMs = ARSTREAM2_RESENDER_MAX_LATENCY_MS;
    ARSTREAM_Reader2_Resender_t *resender = ARSTREAM_Reader2_Resender_New((ARSTREAM_Reader2_t *)(intptr_t)cReader, &config, &err);

    (*env)->ReleaseStringUTFChars (env, targetAddress, c_targetAddress);

    if (err != ARSTREAM_OK)
    {
        ARSAL_PRINT (ARSAL_PRINT_ERROR, JNI_READER_TAG, "Error while creating resender : %s", ARSTREAM_Error_ToString (err));
    }
    return (jlong)(intptr_t)resender;
}

JNIEXPORT void JNICALL
Java_com_parrot_arsdk_arstream_ARStreamReader2Resender_nativeRunSendThread (JNIEnv *env, jobject thizz, jlong cResender)
{
    ARSTREAM_Reader2_Resender_RunSendThread ((void *)(intptr_t)cResender);
}

JNIEXPORT void JNICALL
Java_com_parrot_arsdk_arstream_ARStreamReader2Resender_nativeRunRecvThread (JNIEnv *env, jobject thizz, jlong cResender)
{
    ARSTREAM_Reader2_Resender_RunRecvThread ((void *)(intptr_t)cResender);
}

JNIEXPORT void JNICALL
Java_com_parrot_arsdk_arstream_ARStreamReader2Resender_nativeStop (JNIEnv *env, jobject thizz, jlong cResender)
{
    ARSTREAM_Reader2_Resender_Stop ((ARSTREAM_Reader2_Resender_t *)(intptr_t)cResender);
}

JNIEXPORT jboolean JNICALL
Java_com_parrot_arsdk_arstream_ARStreamReader2Resender_nativeDispose (JNIEnv *env, jobject thizz, jlong cResender)
{
    jboolean retVal = JNI_TRUE;
    eARSTREAM_ERROR err = ARSTREAM_Reader2_Resender_Delete (&cResender);
    if (err != ARSTREAM_OK)
    {
        ARSAL_PRINT (ARSAL_PRINT_ERROR, JNI_READER_TAG, "Unable to delete resender : %s", ARSTREAM_Error_ToString (err));
        retVal = JNI_FALSE;
    }
    return retVal;
}
