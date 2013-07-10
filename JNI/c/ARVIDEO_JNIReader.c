#include <jni.h>
#include <libARVideo/ARVIDEO_Reader.h>
#include <libARSAL/ARSAL_Print.h>

#define JNI_READER_TAG "ARVIDEO_JNIReader"

static jmethodID g_cbWrapper_id = 0;
static JavaVM *g_vm = NULL;

uint8_t* internalCallback (eARVIDEO_READER_CAUSE cause, uint8_t *framePointer, uint32_t frameSize, int numberOfSkippedFrames, uint32_t *newBufferCapacity)
{

}

JNIEXPORT void JNICALL
Java_com_parrot_arsdk_arvideo_ARVideoReader_nativeInitClass (JNIEnv *env, jclass clazz)
{
    jint res = (*env)->GetJavaVM(env, &g_vm);
    if (res < 0)
    {
        ARSAL_PRINT (ARSAL_PRINT_ERROR, JNI_READER_TAG, "Unable to get JavaVM pointer");
    }
    g_cbWrapper_id = (*env)->GetMethodID (env, clazz, "callbackWrapper", "(IJII)Lcom/parrot/arsdk/arsal/ARNativeData;");
}

JNIEXPORT void JNICALL
Java_com_parrot_arsdk_arvideo_ARVideoReader_nativeSetDataBufferParams (JNIEnv *env, jclass clazz, jlong cParams, jint id)
{
    ARVIDEO_Reader_InitVideoDataBuffer ((ARNETWORK_IOBufferParam_t *)(intptr_t)cParams, id);
}

JNIEXPORT void JNICALL
Java_com_parrot_arsdk_arvideo_ARVideoReader_nativeSetAckBufferParams (JNIEnv *env, jclass clazz, jlong cParams, jint id)
{
    ARVIDEO_Reader_InitVideoAckBuffer ((ARNETWORK_IOBufferParam_t *)(intptr_t)cParams, id);
}

JNIEXPORT jlong JNICALL
Java_com_parrot_arsdk_arvideo_ARVideoReader_nativeConstructor (JNIEnv *env, jobject thizz, jlong cNetManager, jint dataBufferId, jint ackBufferId, jlong frameBuffer, jint frameBufferSize)
{
    eARVIDEO_ERROR err = ARVIDEO_OK;
    ARVIDEO_Reader_t *retReader = ARVIDEO_Reader_New ((ARNETWORK_Manager_t *)(intptr_t)cNetManager, dataBufferId, ackBufferId, internalCallback, (uint8_t *)(intptr_t)frameBuffer, frameBufferSize, &err);
    if (err != ARVIDEO_OK)
    {
        ARSAL_PRINT (ARSAL_PRINT_ERROR, JNI_READER_TAG, "Error while creating reader : %s", ARVIDEO_Error_ToString (err));
    }
    return (jlong)(intptr_t)retReader;
}

JNIEXPORT void JNICALL
Java_com_parrot_arsdk_arvideo_ARVideoReader_nativeRunDataThread (JNIEnv *env, jobject thizz, jlong cReader)
{
    ARVIDEO_Reader_RunDataThread ((void *)(intptr_t)cReader);
}

JNIEXPORT void JNICALL
Java_com_parrot_arsdk_arvideo_ARVideoReader_nativeRunAckThread (JNIEnv *env, jobject thizz, jlong cReader)
{
    ARVIDEO_Reader_RunAckThread ((void *)(intptr_t)cReader);
}

JNIEXPORT void JNICALL
Java_com_parrot_arsdk_arvideo_ARVideoReader_nativeStop (JNIEnv *env, jobject thizz, jlong cReader)
{
    ARVIDEO_Reader_StopReader ((ARVIDEO_Reader_t *)(intptr_t)cReader);
}

JNIEXPORT jboolean JNICALL
Java_com_parrot_arsdk_arvideo_ARVideoReader_nativeDispose (JNIEnv *env, jobject thizz, jlong cReader)
{
    jboolean retVal = JNI_TRUE;
    ARVIDEO_Reader_t *reader = (ARVIDEO_Reader_t *)(intptr_t)cReader;
    eARVIDEO_ERROR err = ARVIDEO_Reader_Delete (&reader);
    if (err != ARVIDEO_OK)
    {
        ARSAL_PRINT (ARSAL_PRINT_ERROR, JNI_READER_TAG, "Unable to delete reader : %s", ARVIDEO_Error_ToString (err));
        retVal = JNI_FALSE;
    }
    return retVal;
}

JNIEXPORT jfloat JNICALL
Java_com_parrot_arsdk_arvideo_ARVideoReader_nativeGetEfficiency (JNIEnv *env, jobject thizz, jlong cReader)
{
    return ARVIDEO_Reader_GetEstimatedEfficiency ((ARVIDEO_Reader_t *)(intptr_t)cReader);
}
