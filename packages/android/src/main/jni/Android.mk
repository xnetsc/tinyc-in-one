# ndk-build equivalent of CMakeLists.txt: builds the Android variant of
# the dual-form file (libtcc kernel + JNI glue in one libtinyc.so).
# TINYC_LIBTCC_PATH / LIBTCC_INCLUDE must be set by the kernel build.
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE    := tinyc
LOCAL_SRC_FILES := tinyc_jni.c
LOCAL_C_INCLUDES := $(LIBTCC_INCLUDE) $(LOCAL_PATH)
LOCAL_STATIC_LIBRARIES := tcc_kernel
LOCAL_LDLIBS    := -llog
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := tcc_kernel
LOCAL_SRC_FILES := $(TINYC_LIBTCC_PATH)
include $(PREBUILT_STATIC_LIBRARY)
