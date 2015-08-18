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


#define JNI_READER_TAG "ARSTREAM_JNIReader2"

static jmethodID g_cbWrapper_id = 0;
static JavaVM *g_vm = NULL;

static uint8_t* ARSTREAM_Reader2_NaluCallback(eARSTREAM_READER2_CAUSE cause, uint8_t *naluBuffer, int naluSize, uint64_t auTimestamp,
                                              uint64_t auTimestampShifted, int isFirstNaluInAu, int isLastNaluInAu,
                                              int missingPacketsBefore, int *newNaluBufferSize, void *thizz);

JNIEXPORT void JNICALL
Java_com_parrot_arsdk_arstream_ARStreamReader2_nativeInitClass (JNIEnv *env, jclass clazz)
{
    jint res = (*env)->GetJavaVM(env, &g_vm);
    if (res < 0)
    {
        ARSAL_PRINT (ARSAL_PRINT_ERROR, JNI_READER_TAG, "Unable to get JavaVM pointer");
    }
    g_cbWrapper_id = (*env)->GetMethodID (env, clazz, "callbackWrapper", "(IJIZZJII)[J");
}


JNIEXPORT jlong JNICALL
Java_com_parrot_arsdk_arstream_ARStreamReader2_nativeConstructor (JNIEnv *env, jobject thizz, jstring serverAddress, jint serverStreamPort, jint serverControlPort,
                                                                  jint clientStreamPort, jint clientControlPort, jint maxPacketSize, jint maxBitrate, jint maxLatency, jint maxNetworkLatency)
{
    eARSTREAM_ERROR err = ARSTREAM_OK;
    jobject g_thizz = (*env)->NewGlobalRef(env, thizz);
    const char *c_serverAddress = (*env)->GetStringUTFChars (env, serverAddress, NULL);

    ARSTREAM_Reader2_Config_t config;
    memset(&config, 0, sizeof(config));
    config.serverAddr = c_serverAddress;
    config.mcastAddr = NULL;
    config.mcastIfaceAddr = NULL;
    config.serverStreamPort = serverStreamPort;
    config.serverControlPort = serverControlPort;
    config.clientStreamPort = clientStreamPort;
    config.clientControlPort = clientControlPort;
    config.naluCallback = &ARSTREAM_Reader2_NaluCallback;
    config.naluCallbackUserPtr = (void *)g_thizz;
    config.maxPacketSize = maxPacketSize;
    config.maxBitrate = maxBitrate;
    config.maxLatencyMs = maxLatency;
    config.maxNetworkLatencyMs = maxNetworkLatency;
    config.insertStartCodes = 1;

    ARSTREAM_Reader2_t *retReader = ARSTREAM_Reader2_New(&config, &err);
    if (err != ARSTREAM_OK)
    {
        ARSAL_PRINT (ARSAL_PRINT_ERROR, JNI_READER_TAG, "Error while creating reader : %s", ARSTREAM_Error_ToString (err));
    }

    (*env)->ReleaseStringUTFChars (env, serverAddress, c_serverAddress);
    return (jlong)(intptr_t)retReader;
}

static uint8_t* ARSTREAM_Reader2_NaluCallback(eARSTREAM_READER2_CAUSE cause, uint8_t *naluBuffer, int naluSize, uint64_t auTimestamp,
                                              uint64_t auTimestampShifted, int isFirstNaluInAu, int isLastNaluInAu,
                                              int missingPacketsBefore, int *newNaluBufferSize, void *thizz)
{
    JNIEnv *env = NULL;
    int wasAlreadyAttached = 1;
    int envStatus = (*g_vm)->GetEnv(g_vm, (void **)&env, JNI_VERSION_1_6);
    if (envStatus == JNI_EDETACHED)
    {
        wasAlreadyAttached = 0;
        if ((*g_vm)->AttachCurrentThread(g_vm, &env, NULL) != 0)
        {
            ARSAL_PRINT (ARSAL_PRINT_ERROR, JNI_READER_TAG, "Unable to attach thread to VM");
            *newNaluBufferSize = 0;
            return NULL;
        }
    }
    else if (envStatus != JNI_OK)
    {
        ARSAL_PRINT (ARSAL_PRINT_ERROR, JNI_READER_TAG, "Error %d while getting JNI Environment", envStatus);
        *newNaluBufferSize = 0;
        return NULL;
    }
    jlongArray newNativeDataInfos = (*env)->CallObjectMethod(env, (jobject)thizz, g_cbWrapper_id, (jint)cause, (jlong)(intptr_t)naluBuffer,
            (jint)naluSize, (isFirstNaluInAu ? JNI_TRUE : JNI_FALSE), (isLastNaluInAu ? JNI_TRUE : JNI_FALSE),
            (jlong)auTimestamp, (jint)missingPacketsBefore, (jint)*newNaluBufferSize);

    uint8_t *retVal = NULL;
    *newNaluBufferSize = 0;
    if (newNativeDataInfos != NULL)
    {
        jlong *array = (*env)->GetLongArrayElements(env, newNativeDataInfos, NULL);
        retVal = (uint8_t *)(intptr_t)array[0];
        *newNaluBufferSize = (int)array[1];
        (*env)->ReleaseLongArrayElements(env, newNativeDataInfos, array, 0);
        (*env)->DeleteLocalRef (env, newNativeDataInfos);
    }

    if (wasAlreadyAttached == 0)
    {
        (*g_vm)->DetachCurrentThread(g_vm);
    }

    return retVal;
}

JNIEXPORT void JNICALL
Java_com_parrot_arsdk_arstream_ARStreamReader2_nativeRunStreamThread (JNIEnv *env, jobject thizz, jlong cReader)
{
    ARSTREAM_Reader2_RunStreamThread ((void *)(intptr_t)cReader);
}

JNIEXPORT void JNICALL
Java_com_parrot_arsdk_arstream_ARStreamReader2_nativeRunControlThread (JNIEnv *env, jobject thizz, jlong cReader)
{
    ARSTREAM_Reader2_RunControlThread ((void *)(intptr_t)cReader);
}

JNIEXPORT void JNICALL
Java_com_parrot_arsdk_arstream_ARStreamReader2_nativeStop (JNIEnv *env, jobject thizz, jlong cReader)
{
    ARSTREAM_Reader2_StopReader ((ARSTREAM_Reader2_t *)(intptr_t)cReader);
}

JNIEXPORT jboolean JNICALL
Java_com_parrot_arsdk_arstream_ARStreamReader2_nativeDispose (JNIEnv *env, jobject thizz, jlong cReader)
{
    jboolean retVal = JNI_TRUE;
    ARSTREAM_Reader2_t *reader = (ARSTREAM_Reader2_t *)(intptr_t)cReader;
    void *ref = ARSTREAM_Reader2_GetNaluCallbackUserPtr(reader);
    eARSTREAM_ERROR err = ARSTREAM_Reader2_Delete (&reader);
    if (err != ARSTREAM_OK)
    {
        ARSAL_PRINT (ARSAL_PRINT_ERROR, JNI_READER_TAG, "Unable to delete reader : %s", ARSTREAM_Error_ToString (err));
        retVal = JNI_FALSE;
    }
    if (retVal == JNI_TRUE && ref != NULL)
    {
        (*env)->DeleteGlobalRef(env, (jobject)ref);
    }
    return retVal;
}

