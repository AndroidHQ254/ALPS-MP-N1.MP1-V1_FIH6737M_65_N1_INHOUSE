# mt6735 platform boardconfig

# Use the non-open-source part, if present
-include vendor/mediatek/mt6735/BoardConfigVendor.mk

# Use the common part
include device/mediatek/common/BoardConfig.mk

ifneq ($(MTK_K64_SUPPORT), yes)
TARGET_ARCH := arm

ifneq ($(MTK_BASIC_PACKAGE), yes)
TARGET_CPU_VARIANT := cortex-a53
TARGET_2ND_CPU_VARIANT := cortex-a53
else
TARGET_CPU_VARIANT := cortex-a7
TARGET_2ND_CPU_VARIANT := cortex-a7
endif

TARGET_CPU_ABI := armeabi-v7a
TARGET_CPU_ABI2 := armeabi
TARGET_CPU_SMP := true
TARGET_ARCH_VARIANT := armv7-a-neon

# Don't use cit 4.8 compiler for M to avoid build break
#TARGET_TOOLCHAIN_ROOT := prebuilts/gcc/$(HOST_PREBUILT_TAG)/arm/cit-arm-linux-androideabi-4.8
#TARGET_TOOLS_PREFIX := $(TARGET_TOOLCHAIN_ROOT)/bin/arm-linux-androideabi-

else
TARGET_ARCH := arm64
TARGET_ARCH_VARIANT := armv8-a
TARGET_CPU_ABI := arm64-v8a
TARGET_CPU_ABI2 :=

TARGET_CPU_VARIANT := cortex-a53
TARGET_2ND_CPU_VARIANT := cortex-a53

TARGET_CPU_SMP := true

TARGET_2ND_ARCH := arm
TARGET_2ND_ARCH_VARIANT := armv7-a-neon
TARGET_2ND_CPU_ABI := armeabi-v7a
TARGET_2ND_CPU_ABI2 := armeabi
TARGET_TOOLCHAIN_ROOT := prebuilts/gcc/$(HOST_PREBUILT_TAG)/aarch64/aarch64-linux-android-4.9
TARGET_TOOLS_PREFIX := $(TARGET_TOOLCHAIN_ROOT)/bin/aarch64-linux-android-

KERNEL_CROSS_COMPILE:= $(abspath $(TOP))/$(TARGET_TOOLS_PREFIX)

endif

ARCH_ARM_HAVE_TLS_REGISTER := true
TARGET_BOARD_PLATFORM ?= mt6735
TARGET_USERIMAGES_USE_EXT4 := true
TARGET_NO_FACTORYIMAGE := true
KERNELRELEASE := 3.4

# MTK, Nick Ko, 20140305, Add Display {
TARGET_FORCE_HWC_FOR_VIRTUAL_DISPLAYS := true
NUM_FRAMEBUFFER_SURFACE_BUFFERS := 3
TARGET_RUNNING_WITHOUT_SYNC_FRAMEWORK := true
# Basic package can not set VSYNC_EVENT_PHASE_OFFSET_NS
# If VSYNC_EVENT_PHASE_OFFSET_NS is not 0, it will cause compiler error of SF
ifneq ($(MTK_BASIC_PACKAGE), yes)
VSYNC_EVENT_PHASE_OFFSET_NS := -8000000
SF_VSYNC_EVENT_PHASE_OFFSET_NS := -8000000
endif
PRESENT_TIME_OFFSET_FROM_VSYNC_NS := 0
ifneq ($(FPGA_EARLY_PORTING), yes)
MTK_HWC_SUPPORT := yes
else
MTK_HWC_SUPPORT := no
endif

MTK_HWC_VERSION := 1.4.1
# MTK, Nick Ko, 20140305, Add Display }


BOARD_CONNECTIVITY_VENDOR := MediaTek
BOARD_USES_MTK_AUDIO := true

ifeq ($(MTK_AGPS_APP), yes)
   BOARD_AGPS_SUPL_LIBRARIES := true
else
   BOARD_AGPS_SUPL_LIBRARIES := false
endif

ifeq ($(strip $(BOARD_CONNECTIVITY_VENDOR)), MediaTek)
BOARD_CONNECTIVITY_MODULE := conn_soc 
BOARD_MEDIATEK_USES_GPS := true
endif

# Bluetooth
BOARD_BLUETOOTH_BDROID_BUILDCFG_INCLUDE_DIR := device/generic/common/bluetooth
BOARD_BLUETOOTH_BDROID_HCILP_INCLUDED := 0

# mkbootimg header, which is used in LK
BOARD_KERNEL_BASE = 0x40000000
ifneq ($(MTK_K64_SUPPORT), yes)
BOARD_KERNEL_OFFSET = 0x00008000
else
BOARD_KERNEL_OFFSET = 0x00080000
endif
BOARD_RAMDISK_OFFSET = 0x04000000
BOARD_TAGS_OFFSET = 0xE000000
ifneq ($(MTK_K64_SUPPORT), yes)
BOARD_KERNEL_CMDLINE = bootopt=64S3,32S1,32S1
else
TARGET_USES_64_BIT_BINDER := true
TARGET_IS_64_BIT := true
BOARD_KERNEL_CMDLINE = bootopt=64S3,32N2,64N2
endif
BOARD_MKBOOTIMG_ARGS := --kernel_offset $(BOARD_KERNEL_OFFSET) --ramdisk_offset $(BOARD_RAMDISK_OFFSET) --tags_offset $(BOARD_TAGS_OFFSET)

# ptgen
ifneq ($(strip $(MTK_BASIC_PACKAGE)), yes)
MTK_PTGEN_CHIP ?= $(shell echo $(TARGET_BOARD_PLATFORM) | tr '[a-z]' '[A-Z]')
include device/mediatek/build/build/tools/ptgen/$(MTK_PTGEN_CHIP)/ptgen.mk
endif

#SELinux Policy File Configuration
ifeq ($(strip $(MTK_BASIC_PACKAGE)), yes)
BOARD_SEPOLICY_DIRS += \
        device/mediatek/mt6735/sepolicy/basic
endif
ifeq ($(strip $(MTK_BSP_PACKAGE)), yes)
BOARD_SEPOLICY_DIRS += \
        device/mediatek/mt6735/sepolicy/basic \
        device/mediatek/mt6735/sepolicy/bsp
endif
ifneq ($(strip $(MTK_BASIC_PACKAGE)), yes)
ifneq ($(strip $(MTK_BSP_PACKAGE)), yes)
BOARD_SEPOLICY_DIRS += \
        device/mediatek/mt6735/sepolicy/basic \
        device/mediatek/mt6735/sepolicy/bsp \
        device/mediatek/mt6735/sepolicy/full
endif
endif

MTK_GPU_VERSION := mali midgard r12p1
