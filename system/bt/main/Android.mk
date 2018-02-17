LOCAL_PATH:= $(call my-dir)

# Bluetooth main HW module / shared library for target
# ========================================================
include $(CLEAR_VARS)

# platform specific
LOCAL_SRC_FILES+= \
    bte_main.c \
    bte_init.c \
    bte_logmsg.c \
    bte_conf.c \
    stack_config.c

# sbc encoder
LOCAL_SRC_FILES+= \
    ../embdrv/sbc/encoder/srce/sbc_analysis.c \
    ../embdrv/sbc/encoder/srce/sbc_dct.c \
    ../embdrv/sbc/encoder/srce/sbc_dct_coeffs.c \
    ../embdrv/sbc/encoder/srce/sbc_enc_bit_alloc_mono.c \
    ../embdrv/sbc/encoder/srce/sbc_enc_bit_alloc_ste.c \
    ../embdrv/sbc/encoder/srce/sbc_enc_coeffs.c \
    ../embdrv/sbc/encoder/srce/sbc_encoder.c \
    ../embdrv/sbc/encoder/srce/sbc_packing.c \

LOCAL_SRC_FILES+= \
    ../udrv/ulinux/uipc.c

LOCAL_C_INCLUDES+= . \
	$(LOCAL_PATH)/../ \
	$(LOCAL_PATH)/../bta/include \
	$(LOCAL_PATH)/../bta/sys \
	$(LOCAL_PATH)/../bta/dm \
	$(LOCAL_PATH)/../btcore/include \
	$(LOCAL_PATH)/../include \
	$(LOCAL_PATH)/../stack/include \
	$(LOCAL_PATH)/../stack/l2cap \
	$(LOCAL_PATH)/../stack/a2dp \
	$(LOCAL_PATH)/../stack/btm \
	$(LOCAL_PATH)/../stack/avdt \
	$(LOCAL_PATH)/../hcis \
	$(LOCAL_PATH)/../hcis/include \
	$(LOCAL_PATH)/../hcis/patchram \
	$(LOCAL_PATH)/../udrv/include \
	$(LOCAL_PATH)/../btif/include \
	$(LOCAL_PATH)/../btif/co \
	$(LOCAL_PATH)/../hci/include\
	$(LOCAL_PATH)/../vnd/include \
	$(LOCAL_PATH)/../brcm/include \
	$(LOCAL_PATH)/../embdrv/sbc/encoder/include \
	$(LOCAL_PATH)/../embdrv/sbc/decoder/include \
	$(LOCAL_PATH)/../audio_a2dp_hw \
	$(LOCAL_PATH)/../utils/include \
	$(bluetooth_C_INCLUDES) \
	external/tinyxml2 \
	external/zlib

LOCAL_SHARED_LIBRARIES := \
    libcutils \
    libdl \
    liblog \
    libz \
    libpower \
    libprotobuf-cpp-full \
    libmedia \
    libutils \
    libchrome

LOCAL_STATIC_LIBRARIES := \
    libtinyxml2 \
    libbt-qcom_sbc_decoder

LOCAL_WHOLE_STATIC_LIBRARIES := \
    libbt-bta \
    libbtdevice \
    libbtif \
    libbt-hci \
    libbt-protos \
    libbt-stack \
    libbt-utils \
    libbtcore \
    libosi

LOCAL_MODULE := bluetooth.default
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := SHARED_LIBRARIES

#
# Shared library link options.
# References to global symbols and functions should bind to the library
# itself. This is to avoid issues with some of the unit/system tests
# that might link statically with some of the code in the library, and
# also dlopen(3) the shared library.
#
LOCAL_LDLIBS := -Wl,-Bsymbolic,-Bsymbolic-functions

LOCAL_REQUIRED_MODULES := \
    bt_did.conf \
    bt_stack.conf \
    libbt-hci \
    libbt-vendor

LOCAL_CFLAGS += $(bluetooth_CFLAGS) -DBUILDCFG
LOCAL_CONLYFLAGS += $(bluetooth_CONLYFLAGS)
LOCAL_CPPFLAGS += $(bluetooth_CPPFLAGS)

# M: common customization include
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../../../vendor/mediatek/proprietary/frameworks/base/custom/inc
LOCAL_SHARED_LIBRARIES += libcustom_prop

# M: include compile condition
ifeq ($(MTK_BT_BLUEDROID_PLUS), yes)

# CSR aptX codec specific definitions
ifeq ($(MTK_BT_BLUEDROID_A2DP_APTX), yes)
   LOCAL_C_INCLUDES+= $(TOP)/vendor/csr/aptX
   LOCAL_SHARED_LIBRARIES += libbt-aptX-ARM-4.2.2
endif
# AAC codec specific definitions
   LOCAL_C_INCLUDES+= \
        $(TOP)/external/aac/libAACenc/include \
        $(TOP)/external/aac/libSYS/include \
        $(TOP)/frameworks/av/include/media \
        $(TOP)/system/core/include/system \
        $(TOP)/frameworks/av/include/private/media \
        $(TOP)/frameworks/base/include/media \
        $(TOP)/frameworks/av/media/libstagefright/include
    LOCAL_STATIC_LIBRARIES += \
        libFraunhoferAAC
    LOCAL_SHARED_LIBRARIES += \
        libcutils \
        libstagefright \
        libmedia

LOCAL_REQUIRED_MODULES += \
    bt_stack.conf.sqc \
    bt_stack.conf.debug

LOCAL_WHOLE_STATIC_LIBRARIES += libbt-mtk_cust
endif

# M: comment off 32bit lib
ifeq ($(MTK_BT_BLUEDROID_A2DP_APTX), yes)
 LOCAL_MULTILIB := 32
endif

include $(BUILD_SHARED_LIBRARY)
