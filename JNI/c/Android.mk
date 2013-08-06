LOCAL_PATH := $(call my-dir)

# libARSAL
include $(CLEAR_VARS)

LOCAL_MODULE := LIBARSAL-prebuilt
LOCAL_SRC_FILES := $(TARGET_ARCH_ABI)/libarsal.a

include $(PREBUILT_STATIC_LIBRARY)

# libNetworkAL
include $(CLEAR_VARS)

LOCAL_MODULE := LIBARNETWORKAL-prebuilt
LOCAL_SRC_FILES := $(TARGET_ARCH_ABI)/libarnetworkal.a

include $(PREBUILT_STATIC_LIBRARY)

# libNetwork
include $(CLEAR_VARS)

LOCAL_MODULE := LIBARNETWORK-prebuilt
LOCAL_SRC_FILES := $(TARGET_ARCH_ABI)/libarnetwork.a

include $(PREBUILT_STATIC_LIBRARY)

# libVideo
include $(CLEAR_VARS)

LOCAL_MODULE := LIBARVIDEO-prebuilt
LOCAL_SRC_FILES := $(TARGET_ARCH_ABI)/libarvideo.a

include $(PREBUILT_STATIC_LIBRARY)

# WRAPPER_LIB
include $(CLEAR_VARS)

LOCAL_C_INCLUDES:= $(LOCAL_PATH)/include
LOCAL_LDLIBS := -llog
LOCAL_MODULE := libarvideo_android
LOCAL_SRC_FILES := ARVIDEO_JNISender.c  ARVIDEO_JNIReader.c
LOCAL_CFLAGS := -O0 -g
LOCAL_STATIC_LIBRARIES := LIBARVIDEO-prebuilt LIBARNETWORKAL-prebuilt LIBARNETWORK-prebuilt LIBARSAL-prebuilt
include $(BUILD_SHARED_LIBRARY)
