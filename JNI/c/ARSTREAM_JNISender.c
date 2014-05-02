#include <jni.h>
#include <libARStream/ARSTREAM_Sender.h>
#include <libARSAL/ARSAL_Print.h>

#define JNI_SENDER_TAG "ARSTREAM_JNISender"

static jmethodID g_cbWrapper_id = 0;
static JavaVM *g_vm = NULL;


static void internalCallback (eARSTREAM_SENDER_STATUS status, uint8_t *framePointer, uint32_t frameSize, void *thizz)
{
    JNIEnv *env = NULL;
    int wasAlreadyAttached = 1;
    int envStatus = (*g_vm)->GetEnv(g_vm, (void **)&env, JNI_VERSION_1_6);
    if (envStatus == JNI_EDETACHED)
    {
        wasAlreadyAttached = 0;
        if ((*g_vm)->AttachCurrentThread(g_vm, &env, NULL) != 0)
        {
            ARSAL_PRINT (ARSAL_PRINT_ERROR, JNI_SENDER_TAG, "Unable to attach thread to VM");
            return;
        }
    }
    else if (envStatus != JNI_OK)
    {
        ARSAL_PRINT (ARSAL_PRINT_ERROR, JNI_SENDER_TAG, "Error %d while getting JNI Environment", envStatus);
        return;
    }

    (*env)->CallObjectMethod(env, (jobject)thizz, g_cbWrapper_id, (jint)status, (jlong)(intptr_t)framePointer, (jint)frameSize);

    if (wasAlreadyAttached == 0)
    {
        (*g_vm)->DetachCurrentThread(g_vm);
    }

    return;
}

JNIEXPORT void JNICALL
Java_com_parrot_arsdk_arstream_ARStreamSender_nativeInitClass (JNIEnv *env, jclass clazz)
{
    jint res = (*env)->GetJavaVM(env, &g_vm);
    if (res < 0)
    {
        ARSAL_PRINT (ARSAL_PRINT_ERROR, JNI_SENDER_TAG, "Unable to get JavaVM pointer");
    }
    g_cbWrapper_id = (*env)->GetMethodID (env, clazz, "callbackWrapper", "(IJI)V");
}

JNIEXPORT void JNICALL
Java_com_parrot_arsdk_arstream_ARStreamSender_nativeSetDataBufferParams (JNIEnv *env, jclass clazz, jlong cParams, jint id, jint maxFragmentSize, jint maxNumberOfFragment)
{
    ARSTREAM_Sender_InitStreamDataBuffer ((ARNETWORK_IOBufferParam_t *)(intptr_t)cParams, id, maxFragmentSize, maxNumberOfFragment);
}

JNIEXPORT void JNICALL
Java_com_parrot_arsdk_arstream_ARStreamSender_nativeSetAckBufferParams (JNIEnv *env, jclass clazz, jlong cParams, jint id)
{
    ARSTREAM_Sender_InitStreamAckBuffer ((ARNETWORK_IOBufferParam_t *)(intptr_t)cParams, id);
}

JNIEXPORT jlong JNICALL
Java_com_parrot_arsdk_arstream_ARStreamSender_nativeConstructor (JNIEnv *env, jobject thizz, jlong cNetManager, jint dataBufferId, jint ackBufferId, jint framesBufferSize, jint maxFragmentSize, jint maxNumberOfFragment)
{
    eARSTREAM_ERROR err = ARSTREAM_OK;
    jobject g_thizz = (*env)->NewGlobalRef(env, thizz);
    ARSTREAM_Sender_t *retSender = ARSTREAM_Sender_New ((ARNETWORK_Manager_t *)(intptr_t)cNetManager, dataBufferId, ackBufferId, internalCallback, framesBufferSize, maxFragmentSize, maxNumberOfFragment, (void *)g_thizz, &err);

    if (err != ARSTREAM_OK)
    {
        ARSAL_PRINT (ARSAL_PRINT_ERROR, JNI_SENDER_TAG, "Error while creating sender : %s", ARSTREAM_Error_ToString (err));
    }
    return (jlong)(intptr_t)retSender;
}

JNIEXPORT void JNICALL
Java_com_parrot_arsdk_arstream_ARStreamSender_nativeRunDataThread (JNIEnv *env, jobject thizz, jlong cSender)
{
    ARSTREAM_Sender_RunDataThread ((void *)(intptr_t)cSender);
}

JNIEXPORT void JNICALL
Java_com_parrot_arsdk_arstream_ARStreamSender_nativeRunAckThread (JNIEnv *env, jobject thizz, jlong cSender)
{
    ARSTREAM_Sender_RunAckThread ((void *)(intptr_t)cSender);
}

JNIEXPORT void JNICALL
Java_com_parrot_arsdk_arstream_ARStreamSender_nativeStop (JNIEnv *env, jobject thizz, jlong cSender)
{
    ARSTREAM_Sender_StopSender ((ARSTREAM_Sender_t *)(intptr_t)cSender);
}

JNIEXPORT jboolean JNICALL
Java_com_parrot_arsdk_arstream_ARStreamSender_nativeDispose (JNIEnv *env, jobject thizz, jlong cSender)
{
    jboolean retVal = JNI_TRUE;
    ARSTREAM_Sender_t *sender = (ARSTREAM_Sender_t *)(intptr_t)cSender;
    void *ref = ARSTREAM_Sender_GetCustom (sender);
    eARSTREAM_ERROR err = ARSTREAM_Sender_Delete (&sender);
    if (err != ARSTREAM_OK)
    {
        ARSAL_PRINT (ARSAL_PRINT_ERROR, JNI_SENDER_TAG, "Unable to delete sender : %s", ARSTREAM_Error_ToString (err));
        retVal = JNI_FALSE;
    }
    if (retVal == JNI_TRUE && ref != NULL)
    {
        (*env)->DeleteGlobalRef(env, (jobject)ref);
    }
    return retVal;
}

JNIEXPORT jfloat JNICALL
Java_com_parrot_arsdk_arstream_ARStreamSender_nativeGetEfficiency (JNIEnv *env, jobject thizz, jlong cSender)
{
    return ARSTREAM_Sender_GetEstimatedEfficiency ((ARSTREAM_Sender_t *)(intptr_t)cSender);
}

JNIEXPORT jint JNICALL
Java_com_parrot_arsdk_arstream_ARStreamSender_nativeSendNewFrame (JNIEnv *env, jobject thizz, jlong cSender, jlong frameBuffer, jint frameSize, jboolean flushPreviousFrames)
{
    int flush = (flushPreviousFrames == JNI_TRUE) ? 1 : 0;
    eARSTREAM_ERROR err = ARSTREAM_Sender_SendNewFrame((ARSTREAM_Sender_t *)(intptr_t)cSender, (uint8_t *)(intptr_t)frameBuffer, frameSize, flush, NULL);
    return (jint)err;
}
