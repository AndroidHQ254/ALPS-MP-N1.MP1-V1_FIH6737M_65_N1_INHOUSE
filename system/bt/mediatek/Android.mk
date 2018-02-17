LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/include \
	$(LOCAL_PATH)/.. \
	$(LOCAL_PATH)/../include \
	$(LOCAL_PATH)/../btcore/include \
	$(LOCAL_PATH)/../stack/include \
	$(bluetooth_C_INCLUDES)

LOCAL_SRC_FILES := \
	./conf/btif_config_mtk_util.c \
	./conf/mdroid_stack_config.c \
	./hci/twrite.c \
	./hci/hci_fw_core_dump_ctrl.c \
	./interop/interop_mtk.c

LOCAL_MODULE := libbt-mtk_cust
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := STATIC_LIBRARIES

LOCAL_CFLAGS += $(bluetooth_CFLAGS)
LOCAL_CONLYFLAGS += $(bluetooth_CONLYFLAGS)
LOCAL_CPPFLAGS += $(bluetooth_CPPFLAGS)


include $(BUILD_STATIC_LIBRARY)
