LOCAL_PATH := $(call my-dir)

# JNI Wrapper
include $(CLEAR_VARS)

LOCAL_CFLAGS := -g
LOCAL_MODULE := libarstream_android
LOCAL_SRC_FILES := JNI/c/ARSTREAM_JNIReader.c JNI/c/ARSTREAM_JNISender.c JNI/c/ARSTREAM_JNIReader2Resender.c JNI/c/ARSTREAM_JNIReader2.c
LOCAL_LDLIBS := -llog -lz
LOCAL_SHARED_LIBRARIES := libARStream-prebuilt libARSAL-prebuilt
include $(BUILD_SHARED_LIBRARY)
