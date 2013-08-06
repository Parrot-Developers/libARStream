#include <jni.h>
#include <libARVideo/ARVIDEO_Sender.h>
#include <libARSAL/ARSAL_Print.h>

#define JNI_SENDER_TAG "ARVIDEO_JNISender"

static jmethodID g_cbWrapper_id = 0;
static JavaVM *g_vm = NULL;
