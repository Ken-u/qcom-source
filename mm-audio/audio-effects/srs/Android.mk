LOCAL_PATH := $(my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := libtru_media_headers
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/TruMedia
include $(BUILD_HEADER_LIBRARY)
