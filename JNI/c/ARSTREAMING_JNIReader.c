#include <jni.h>
#include <libARStreaming/ARSTREAMING_Reader.h>
#include <libARSAL/ARSAL_Print.h>

#define JNI_READER_TAG "ARSTREAMING_JNIReader"

static jmethodID g_cbWrapper_id = 0;
static JavaVM *g_vm = NULL;
static jobject g_thizz = NULL;

uint8_t* internalCallback (eARSTREAMING_READER_CAUSE cause, uint8_t *framePointer, uint32_t frameSize, int isFlushFrame, int numberOfSkippedFrames, uint32_t *newBufferCapacity, void *thizz)
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
            *newBufferCapacity = 0;
            return NULL;
        }
    }
    else if (envStatus != JNI_OK)
    {
        ARSAL_PRINT (ARSAL_PRINT_ERROR, JNI_READER_TAG, "Error %d while getting JNI Environment", envStatus);
        *newBufferCapacity = 0;
        return NULL;
    }

    jboolean isFlush = (isFlushFrame == 1) ? JNI_TRUE : JNI_FALSE;
    jlongArray *newNativeDataInfos = (*env)->CallObjectMethod(env, (jobject)thizz, g_cbWrapper_id, (jint)cause, (jlong)(intptr_t)framePointer, (jint)frameSize, isFlush, (jint)numberOfSkippedFrames);

    uint8_t *retVal = NULL;
    *newBufferCapacity = 0;
    if (newNativeDataInfos != NULL)
    {
        jlong *array = (*env)->GetLongArrayElements(env, newNativeDataInfos, NULL);
        retVal = (uint8_t *)(intptr_t)array[0];
        *newBufferCapacity = (int)array[1];
        (*env)->ReleaseLongArrayElements(env, newNativeDataInfos, array, 0);
    }

    if (wasAlreadyAttached == 0)
    {
        (*g_vm)->DetachCurrentThread(g_vm);
    }

    return retVal;
}

JNIEXPORT void JNICALL
Java_com_parrot_arsdk_arstreaming_ARStreamingReader_nativeInitClass (JNIEnv *env, jclass clazz)
{
    jint res = (*env)->GetJavaVM(env, &g_vm);
    if (res < 0)
    {
        ARSAL_PRINT (ARSAL_PRINT_ERROR, JNI_READER_TAG, "Unable to get JavaVM pointer");
    }
    g_cbWrapper_id = (*env)->GetMethodID (env, clazz, "callbackWrapper", "(IJIZI)[J");
}

JNIEXPORT void JNICALL
Java_com_parrot_arsdk_arstreaming_ARStreamingReader_nativeSetDataBufferParams (JNIEnv *env, jclass clazz, jlong cParams, jint id)
{
    ARSTREAMING_Reader_InitStreamingDataBuffer ((ARNETWORK_IOBufferParam_t *)(intptr_t)cParams, id);
}

JNIEXPORT void JNICALL
Java_com_parrot_arsdk_arstreaming_ARStreamingReader_nativeSetAckBufferParams (JNIEnv *env, jclass clazz, jlong cParams, jint id)
{
    ARSTREAMING_Reader_InitStreamingAckBuffer ((ARNETWORK_IOBufferParam_t *)(intptr_t)cParams, id);
}

JNIEXPORT jlong JNICALL
Java_com_parrot_arsdk_arstreaming_ARStreamingReader_nativeConstructor (JNIEnv *env, jobject thizz, jlong cNetManager, jint dataBufferId, jint ackBufferId, jlong frameBuffer, jint frameBufferSize)
{
    eARSTREAMING_ERROR err = ARSTREAMING_OK;
    g_thizz = (*env)->NewGlobalRef(env, thizz);
    ARSTREAMING_Reader_t *retReader = ARSTREAMING_Reader_New ((ARNETWORK_Manager_t *)(intptr_t)cNetManager, dataBufferId, ackBufferId, internalCallback, (uint8_t *)(intptr_t)frameBuffer, frameBufferSize, (void *)g_thizz, &err);
    if (err != ARSTREAMING_OK)
    {
        ARSAL_PRINT (ARSAL_PRINT_ERROR, JNI_READER_TAG, "Error while creating reader : %s", ARSTREAMING_Error_ToString (err));
    }
    return (jlong)(intptr_t)retReader;
}

JNIEXPORT void JNICALL
Java_com_parrot_arsdk_arstreaming_ARStreamingReader_nativeRunDataThread (JNIEnv *env, jobject thizz, jlong cReader)
{
    ARSTREAMING_Reader_RunDataThread ((void *)(intptr_t)cReader);
}

JNIEXPORT void JNICALL
Java_com_parrot_arsdk_arstreaming_ARStreamingReader_nativeRunAckThread (JNIEnv *env, jobject thizz, jlong cReader)
{
    ARSTREAMING_Reader_RunAckThread ((void *)(intptr_t)cReader);
}

JNIEXPORT void JNICALL
Java_com_parrot_arsdk_arstreaming_ARStreamingReader_nativeStop (JNIEnv *env, jobject thizz, jlong cReader)
{
    ARSTREAMING_Reader_StopReader ((ARSTREAMING_Reader_t *)(intptr_t)cReader);
}

JNIEXPORT jboolean JNICALL
Java_com_parrot_arsdk_arstreaming_ARStreamingReader_nativeDispose (JNIEnv *env, jobject thizz, jlong cReader)
{
    jboolean retVal = JNI_TRUE;
    ARSTREAMING_Reader_t *reader = (ARSTREAMING_Reader_t *)(intptr_t)cReader;
    eARSTREAMING_ERROR err = ARSTREAMING_Reader_Delete (&reader);
    if (err != ARSTREAMING_OK)
    {
        ARSAL_PRINT (ARSAL_PRINT_ERROR, JNI_READER_TAG, "Unable to delete reader : %s", ARSTREAMING_Error_ToString (err));
        retVal = JNI_FALSE;
    }
    if (retVal == JNI_TRUE)
    {
        (*env)->DeleteGlobalRef(env, g_thizz);
    }
    return retVal;
}

JNIEXPORT jfloat JNICALL
Java_com_parrot_arsdk_arstreaming_ARStreamingReader_nativeGetEfficiency (JNIEnv *env, jobject thizz, jlong cReader)
{
    return ARSTREAMING_Reader_GetEstimatedEfficiency ((ARSTREAMING_Reader_t *)(intptr_t)cReader);
}
