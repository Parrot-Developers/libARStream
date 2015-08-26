LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_CATEGORY_PATH := dragon/libs
LOCAL_MODULE := libARStream
LOCAL_DESCRIPTION := ARSDK Stream library

LOCAL_LIBRARIES := ARSDKBuildUtils libARSAL libARNetworkAL libARNetwork
LOCAL_EXPORT_LDLIBS := -larstream

# Copy in build dir so bootstrap files are generated in build dir
LOCAL_AUTOTOOLS_COPY_TO_BUILD_DIR := 1

# Configure script is not at the root
LOCAL_AUTOTOOLS_CONFIGURE_SCRIPT := Build/configure

#Autotools variables
LOCAL_AUTOTOOLS_CONFIGURE_ARGS := \
	--with-libARSALInstallDir="" \
	--with-libARNetworkALInstallDir="" \
	--with-libARNetworkInstallDir=""

ifdef ARSDK_BUILD_FOR_APP

ifeq ("$(TARGET_OS_FLAVOUR)","android")

LOCAL_AUTOTOOLS_CONFIGURE_ARGS += \
	--disable-static \
	--enable-shared \
	--disable-so-version

else ifneq ($(filter iphoneos iphonesimulator, $(TARGET_OS_FLAVOUR)),)

LOCAL_AUTOTOOLS_CONFIGURE_ARGS += \
	--enable-static \
	--disable-shared \
	OBJCFLAGS=" -x objective-c -fobjc-arc -std=gnu99 $(TARGET_GLOBAL_CFLAGS)" \
	OBJC="$(TARGET_CC)" \
	CFLAGS=" -std=gnu99 -x c $(TARGET_GLOBAL_CFLAGS)"

endif

LOCAL_AUTOTOOLS_CONFIGURE_ARGS += \
	LIBS=" -lm"

endif

# User define command to be launch before configure step.
# Generates files used by configure
define LOCAL_AUTOTOOLS_CMD_POST_UNPACK
	$(Q) cd $(PRIVATE_SRC_DIR)/Build && ./bootstrap
endef

include $(BUILD_AUTOTOOLS)
