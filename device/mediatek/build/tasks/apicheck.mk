# Copyright Statement:
#
# This software/firmware and related documentation ("MediaTek Software") are
# protected under relevant copyright laws. The information contained herein
# is confidential and proprietary to MediaTek Inc. and/or its licensors.
# Without the prior written permission of MediaTek inc. and/or its licensors,
# any reproduction, modification, use or disclosure of MediaTek Software,
# and information contained herein, in whole or in part, shall be strictly prohibited.
#
# MediaTek Inc. (C) 2010. All rights reserved.
#
# BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
# THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
# RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER ON
# AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
# NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
# SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
# SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
# THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
# THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
# CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
# SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
# STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
# CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
# AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
# OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
# MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.

ifdef MTK_TARGET_PROJECT
ifneq ($(strip $(MTK_BSP_PACKAGE)),yes)
ifneq ($(strip $(MTK_BASIC_PACKAGE)),yes)

SRC_MTK_API_DIR := vendor/mediatek/proprietary/frameworks/base/api
SRC_MTK_PLUGIN_API_DIR := vendor/mediatek/proprietary/packages/apicheck/api
CHECK_MTK_PLUGIN_API_MK := vendor/mediatek/proprietary/packages/apicheck/check_plugin.mk
CHECK_MTK_INTERNAL_API_MK := vendor/mediatek/proprietary/frameworks/base/apicheck/internal_api_check.mk
CHECK_MTK_SDK_API_MK := vendor/mediatek/proprietary/frameworks/base/apicheck/sdk_api_check.mk

ifeq ($(strip $(MTK_INTERNAL)),yes)
ifneq (,$(strip $(ONE_SHOT_MAKEFILE)$(BUILD_MODULES_IN_PATHS)))
miar: all_modules checkmtkapi
  ifeq ($(filter $(call doc-timestamp-for,mediatek-internal-api-stubs),$(ALL_DOCS)),)
include $(wildcard vendor/mediatek/proprietary/frameworks/base/Android.mk)
  endif
  ifeq ($(filter $(call doc-timestamp-for,mediatek-plugin-api-stubs),$(ALL_DOCS)),)
include $(wildcard vendor/mediatek/proprietary/packages/apicheck/Android.mk)
  endif
endif
endif

.PHONY: checkmtkapi

# eval this to define a rule that runs apicheck.
#
# Args:
#    $(1)  target
#    $(2)  stable api file
#    $(3)  api file to be tested
#    $(4)  stable removed api file
#    $(5)  removed api file to be tested
#    $(6)  arguments for apicheck
#    $(7)  command to run if apicheck failed
#    xxxx  target dependent on this api check
#    $(8)  additional dependencies
define check-mtk-api
$(TARGET_OUT_COMMON_INTERMEDIATES)/PACKAGING/$(strip $(1))-timestamp: $(2) $(3) $(4) $(APICHECK) $(8)
	@echo "Checking MediaTek API:" $(1)
	$(hide) ( $(APICHECK_COMMAND) $(6) $(2) $(3) $(4) $(5) || ( $(7) ; exit 38 ) )
	$(hide) mkdir -p $$(dir $$@)
	$(hide) touch $$@
checkmtkapi: $(TARGET_OUT_COMMON_INTERMEDIATES)/PACKAGING/$(strip $(1))-timestamp
endef

checkapi: checkmtkapi

BUILD_SYSTEM_MTK_EXTENSION := $(TOPDIR)device/mediatek/build/tasks

##################Check Mediatek SDK API ##################
# Check Mediatek SDK api list if available
-include $(CHECK_MTK_SDK_API_MK)

##################Check Mediatek LEGO API ##################
# Check Mediatek internal lego api list if available
-include $(CHECK_MTK_INTERNAL_API_MK)

####################Check Mtk Plugin API####################
# Check mtk plugin api if available
-include $(CHECK_MTK_PLUGIN_API_MK)

endif
endif
endif

