#this is platform common Boardconfig

ifneq ($(MTK_GLOBAL_C_INCLUDES)$(MTK_GLOBAL_CFLAGS)$(MTK_GLOBAL_CONLYFLAGS)$(MTK_GLOBAL_CPPFLAGS)$(MTK_GLOBAL_LDFLAGS),)
$(info *** MTK-specific global flags are changed)
$(info *** MTK_GLOBAL_C_INCLUDES: $(MTK_GLOBAL_C_INCLUDES))
$(info *** MTK_GLOBAL_CFLAGS: $(MTK_GLOBAL_CFLAGS))
$(info *** MTK_GLOBAL_CONLYFLAGS: $(MTK_GLOBAL_CONLYFLAGS))
$(info *** MTK_GLOBAL_CPPFLAGS: $(MTK_GLOBAL_CPPFLAGS))
$(info *** MTK_GLOBAL_LDFLAGS: $(MTK_GLOBAL_LDFLAGS))
$(error bailing...)
endif

MTK_GLOBAL_C_INCLUDES:=
MTK_GLOBAL_CFLAGS:=
MTK_GLOBAL_CONLYFLAGS:=
MTK_GLOBAL_CPPFLAGS:=
MTK_GLOBAL_LDFLAGS:=

# Use the non-open-source part, if present
-include vendor/mediatek/common/BoardConfigVendor.mk

# Use the connectivity Boardconfig
include device/mediatek/common/connectivity/BoardConfig.mk

# for flavor build base project assignment
ifeq ($(strip $(MTK_BASE_PROJECT)),)
  MTK_PROJECT_NAME := $(subst full_,,$(TARGET_PRODUCT))
else
  MTK_PROJECT_NAME := $(MTK_BASE_PROJECT)
endif
MTK_PROJECT := $(MTK_PROJECT_NAME)
MTK_PATH_SOURCE := vendor/mediatek/proprietary
MTK_ROOT := vendor/mediatek/proprietary
MTK_PATH_COMMON := vendor/mediatek/proprietary/custom/common
MTK_PATH_CUSTOM := vendor/mediatek/proprietary/custom/$(MTK_PROJECT)
MTK_PATH_CUSTOM_PLATFORM := vendor/mediatek/proprietary/custom/$(shell echo $(MTK_PLATFORM) | tr '[A-Z]' '[a-z]')
KERNEL_CROSS_COMPILE:= $(abspath $(TOP))/prebuilts/gcc/linux-x86/arm/cit-arm-eabi-4.8/bin/arm-eabi-
TARGET_BOARD_KERNEL_HEADERS := device/mediatek/$(shell echo $(MTK_PLATFORM) | tr '[A-Z]' '[a-z]')/kernel-headers \
                               device/mediatek/common/kernel-headers

MTK_GLOBAL_C_INCLUDES += $(TOPDIR)vendor/mediatek/proprietary/hardware/audio/common/include
MTK_GLOBAL_C_INCLUDES += $(MTK_PATH_CUSTOM)/cgen/cfgdefault $(MTK_PATH_CUSTOM)/cgen/cfgfileinc $(MTK_PATH_CUSTOM)/cgen/inc $(MTK_PATH_CUSTOM)/cgen
MTK_GLOBAL_C_INCLUDES += $(MTK_PATH_CUSTOM_PLATFORM)/cgen/cfgdefault $(MTK_PATH_CUSTOM_PLATFORM)/cgen/cfgfileinc $(MTK_PATH_CUSTOM_PLATFORM)/cgen/inc $(MTK_PATH_CUSTOM_PLATFORM)/cgen
MTK_GLOBAL_C_INCLUDES += $(MTK_PATH_COMMON)/cgen/cfgdefault $(MTK_PATH_COMMON)/cgen/cfgfileinc $(MTK_PATH_COMMON)/cgen/inc $(MTK_PATH_COMMON)/cgen

# Add MTK compile options to wrap MTK's modifications on AOSP.
ifneq ($(strip $(MTK_BASIC_PACKAGE)),yes)
ifneq ($(strip $(MTK_EMULATOR_SUPPORT)),yes)
  MTK_GLOBAL_CFLAGS += -DMTK_AOSP_ENHANCEMENT
endif
endif

#MTK_PLATFORM := $(shell echo $(MTK_PROJECT_NAME) | awk -F "_" {'print $$1'})
MTK_PATH_PLATFORM := $(MTK_PATH_SOURCE)/platform/$(shell echo $(MTK_PLATFORM) | tr '[A-Z]' '[a-z]')
MTK_PATH_KERNEL := kernel
GOOGLE_RELEASE_RIL := no
BUILD_NUMBER := $(shell date +%s)

#Enable HWUI by default
USE_OPENGL_RENDERER := true

#SELinux Policy File Configuration
ifeq ($(strip $(MTK_BASIC_PACKAGE)), yes)
BOARD_SEPOLICY_DIRS := \
        device/mediatek/common/sepolicy/basic
endif
ifeq ($(strip $(MTK_BSP_PACKAGE)), yes)
BOARD_SEPOLICY_DIRS := \
        device/mediatek/common/sepolicy/basic \
        device/mediatek/common/sepolicy/bsp
endif
ifneq ($(strip $(MTK_BASIC_PACKAGE)), yes)
ifneq ($(strip $(MTK_BSP_PACKAGE)), yes)
BOARD_SEPOLICY_DIRS := \
        device/mediatek/common/sepolicy/basic \
        device/mediatek/common/sepolicy/bsp \
        device/mediatek/common/sepolicy/full
endif
endif

BOARD_SEPOLICY_DIRS += $(wildcard device/mediatek/common/sepolicy/secure)

# Include an expanded selection of fonts
EXTENDED_FONT_FOOTPRINT := true

# ptgen
# Add MTK's MTK_PTGEN_OUT definitions
ifeq (,$(strip $(OUT_DIR)))
  ifeq (,$(strip $(OUT_DIR_COMMON_BASE)))
    MTK_PTGEN_OUT_DIR := $(TOPDIR)out
  else
    MTK_PTGEN_OUT_DIR := $(OUT_DIR_COMMON_BASE)/$(notdir $(PWD))
  endif
else
    MTK_PTGEN_OUT_DIR := $(strip $(OUT_DIR))
endif
ifneq ($(strip $(MTK_TARGET_PROJECT)), $(strip $(MTK_BASE_PROJECT)))
MTK_PTGEN_PRODUCT_OUT := $(MTK_PTGEN_OUT_DIR)/target/product/$(MTK_TARGET_PROJECT)
else
MTK_PTGEN_PRODUCT_OUT := $(MTK_PTGEN_OUT_DIR)/target/product/$(TARGET_DEVICE)
endif
MTK_PTGEN_OUT := $(MTK_PTGEN_PRODUCT_OUT)/obj/PTGEN
MTK_PTGEN_MK_OUT := $(MTK_PTGEN_PRODUCT_OUT)/obj/PTGEN
MTK_PTGEN_TMP_OUT := $(MTK_PTGEN_PRODUCT_OUT)/obj/PTGEN_TMP

#Add MTK's Recovery fstab definitions
TARGET_RECOVERY_FSTAB := $(MTK_PTGEN_PRODUCT_OUT)/root/fstab.$(MTK_PLATFORM_DIR)

ifeq ($(BUILD_GMS),yes)
  DONT_DEXPREOPT_PREBUILTS := true
else
  ifeq ($(TARGET_BUILD_VARIANT),userdebug)
    DEX_PREOPT_DEFAULT := nostripping
  endif
endif

#Add MTK's hook
-include device/mediatek/build/build/tools/base_rule_hook.mk
-include device/mediatek/build/build/tools/base_rule_jack.mk
-include device/mediatek/build/build/tools/rpgen.mk
