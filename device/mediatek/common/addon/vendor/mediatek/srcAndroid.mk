# A workaround for the build hang issue when non-MTK lunch is selected.
# e.g., aosp_arm-eng.
#
# Many Android.mk files under vendor/mediatek will recursively include
# themselves if MTK_PLATFORM isn't defined. e.g.,
#   include $(LOCAL_PATH)/$(shell echo $(MTK_PLATFORM) | tr A-Z a-z )/Android.mk
#
# Only include Android.mk files under vendor/mediatek/ when
# MTK_PROJECT_NAME is defined. This file should be renamed to
# vendor/mediatek/Android.mk, either manually or through <copyfile> setting
# in repo manifest.

ifneq ($(strip $(MTK_PROJECT_NAME)),)
LOCAL_PATH := $(call my-dir)
include $(call first-makefiles-under, $(LOCAL_PATH))
endif
