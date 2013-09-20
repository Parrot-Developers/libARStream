#include <jni.h>
#include <libARStream/ARSTREAM_Reader.h>
#include <libARSAL/ARSAL_Print.h>

#define JNI_READER_TAG "ARSTREAM_JNIReader"

static jmethodID g_cbWrapper_id = 0;
static JavaVM *g_vm = NULL;
static jobject g_thizz = NULL;

uint8_t* internalCallback (eARSTREAM_READER_CAUSE cause, uint8_t *framePointer, uint32_t frameSize, int isFlushFrame, int numberOfSkippedFrames, uint32_t *newBufferCapacity, void *thizz)
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
Java_com_parrot_arsdk_arstream_ARStreamReader_nativeInitClass (JNIEnv *env, jclass clazz)
{
    jint res = (*env)->GetJavaVM(env, &g_vm);
    if (res < 0)
    {
        ARSAL_PRINT (ARSAL_PRINT_ERROR, JNI_READER_TAG, "Unable to get JavaVM pointer");
    }
    g_cbWrapper_id = (*env)->GetMethodID (env, clazz, "callbackWrapper", "(IJIZI)[J");
}

JNIEXPORT void JNICALL
Java_com_parrot_arsdk_arstream_ARStreamReader_nativeSetDataBufferParams (JNIEnv *env, jclass clazz, jlong cParams, jint id)
{
    ARSTREAM_Reader_InitStreamDataBuffer ((ARNETWORK_IOBufferParam_t *)(intptr_t)cParams, id);
}

JNIEXPORT void JNICALL
Java_com_parrot_arsdk_arstream_ARStreamReader_nativeSetAckBufferParams (JNIEnv *env, jclass clazz, jlong cParams, jint id)
{
    ARSTREAM_Reader_InitStreamAckBuffer ((ARNETWORK_IOBufferParam_t *)(intptr_t)cParams, id);
}

JNIEXPORT jlong JNICALL
Java_com_parrot_arsdk_arstream_ARStreamReader_nativeConstructor (JNIEnv *env, jobject thizz, jlong cNetManager, jint dataBufferId, jint ackBufferId, jlong frameBuffer, jint frameBufferSize)
{
    eARSTREAM_ERROR err = ARSTREAM_OK;
    g_thizz = (*env)->NewGlobalRef(env, thizz);
    ARSTREAM_Reader_t *retReader = ARSTREAM_Reader_New ((ARNETWORK_Manager_t *)(intptr_t)cNetManager, dataBufferId, ackBufferId, internalCallback, (uint8_t *)(intptr_t)frameBuffer, frameBufferSize, (void *)g_thizz, &err);
    if (err != ARSTREAM_OK)
    {
        ARSAL_PRINT (ARSAL_PRINT_ERROR, JNI_READER_TAG, "Error while creating reader : %s", ARSTREAM_Error_ToString (err));
    }
    return (jlong)(intptr_t)retReader;
}

JNIEXPORT void JNICALL
Java_com_parrot_arsdk_arstream_ARStreamReader_nativeRunDataThread (JNIEnv *env, jobject thizz, jlong cReader)
{
    ARSTREAM_Reader_RunDataThread ((void *)(intptr_t)cReader);
}

JNIEXPORT void JNICALL
Java_com_parrot_arsdk_arstream_ARStreamReader_nativeRunAckThread (JNIEnv *env, jobject thizz, jlong cReader)
{
    ARSTREAM_Reader_RunAckThread ((void *)(intptr_t)cReader);
}

JNIEXPORT void JNICALL
Java_com_parrot_arsdk_arstream_ARStreamReader_nativeStop (JNIEnv *env, jobject thizz, jlong cReader)
{
    ARSTREAM_Reader_StopReader ((ARSTREAM_Reader_t *)(intptr_t)cReader);
}

JNIEXPORT jboolean JNICALL
Java_com_parrot_arsdk_arstream_ARStreamReader_nativeDispose (JNIEnv *env, jobject thizz, jlong cReader)
{
    jboolean retVal = JNI_TRUE;
    ARSTREAM_Reader_t *reader = (ARSTREAM_Reader_t *)(intptr_t)cReader;
    eARSTREAM_ERROR err = ARSTREAM_Reader_Delete (&reader);
    if (err != ARSTREAM_OK)
    {
        ARSAL_PRINT (ARSAL_PRINT_ERROR, JNI_READER_TAG, "Unable to delete reader : %s", ARSTREAM_Error_ToString (err));
        retVal = JNI_FALSE;
    }
    if (retVal == JNI_TRUE)
    {
        (*env)->DeleteGlobalRef(env, g_thizz);
    }
    return retVal;
}

JNIEXPORT jfloat JNICALL
Java_com_parrot_arsdk_arstream_ARStreamReader_nativeGetEfficiency (JNIEnv *env, jobject thizz, jlong cReader)
{
    return ARSTREAM_Reader_GetEstimatedEfficiency ((ARSTREAM_Reader_t *)(intptr_t)cReader);
}
