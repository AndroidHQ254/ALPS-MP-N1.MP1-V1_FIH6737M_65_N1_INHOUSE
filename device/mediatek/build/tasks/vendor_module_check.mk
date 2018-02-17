#
# Copyright (C) 2011 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

ifeq ($(MTK_INTERNAL),yes)

# Restrict the vendor module owners here.
_vendor_owner_whitelist := \
        mtk \
        dynastream \
        widevine


ifeq (,$(PRODUCTS.$(INTERNAL_PRODUCT).PRODUCT_RESTRICT_VENDOR_FILES))

$(info MTK vendor module check - start)

_vendor_check_exception_system := \
        wpa_supplicant.conf wpa_supplicant_overlay.conf p2p_supplicant_overlay.conf \
        mediatek-common mediatek-framework mediatek-telephony-common \
        bootaudio.mp3 bootanimation.zip shutanimation.zip shutaudio.mp3 \
        DuraSpeed mediatek-feature-runningbooster \
        liba3m_32 libja3m_32 liba3m libja3m

_vendor_check_exception_root := \
        multi_init \
        init.wod.rc \
        fuelgauged_static

_vendor_check_exception_data := \
        calib.dat \
        param.dat \
        sensors.dat

_vendor_check_exception_other_partition :=
ifeq (yes,$(strip $(SPM_FW_USE_PARTITION)))
_vendor_check_exception_other_partition += \
        spmfw.bin
endif

_vendor_check_exception := $(_vendor_check_exception_system)
_vendor_check_exception += $(_vendor_check_exception_root)
_vendor_check_exception += $(_vendor_check_exception_data)
_vendor_check_exception += $(_vendor_check_exception_other_partition)

_vendor_check_modules := $(filter-out $(_vendor_check_exception), $(product_MODULES))

_vendor_module_owner_info :=
# Restrict owners
ifeq (,$(filter true owner all, $(PRODUCTS.$(INTERNAL_PRODUCT).PRODUCT_RESTRICT_VENDOR_FILES)))

ifneq (,$(filter vendor/%, $(PRODUCT_PACKAGE_OVERLAYS) $(DEVICE_PACKAGE_OVERLAYS)))
$(info Warning: Product "$(TARGET_PRODUCT)" cannot have overlay in vendor tree: \
    $(filter vendor/%, $(PRODUCT_PACKAGE_OVERLAYS) $(DEVICE_PACKAGE_OVERLAYS)))
endif
_vendor_check_copy_files := $(filter vendor/%, $(PRODUCT_COPY_FILES))
ifneq (,$(_vendor_check_copy_files))
$(foreach c, $(_vendor_check_copy_files), \
  $(if $(filter $(_vendor_owner_whitelist), $(call word-colon,3,$(c))),,\
    $(info Warning: vendor PRODUCT_COPY_FILES file "$(c)" has unknown owner))\
  $(eval _vendor_module_owner_info += $(call word-colon,2,$(c)):$(call word-colon,3,$(c))))
endif
_vendor_check_copy_files :=

$(foreach m, $(_vendor_check_modules), \
  $(if $(filter vendor/%, $(ALL_MODULES.$(m).PATH)),\
    $(if $(filter-out FAKE, $(ALL_MODULES.$(m).CLASS)),\
      $(if $(filter $(_vendor_owner_whitelist), $(ALL_MODULES.$(m).OWNER)),,\
        $(info Warning: vendor module "$(m)" in $(ALL_MODULES.$(m).PATH) with unknown owner \
          "$(ALL_MODULES.$(m).OWNER)" in product "$(TARGET_PRODUCT)"))\
      $(if $(ALL_MODULES.$(m).INSTALLED),\
        $(eval _vendor_module_owner_info += $(patsubst $(PRODUCT_OUT)/%,%,$(ALL_MODULES.$(m).INSTALLED)):$(ALL_MODULES.$(m).OWNER))))))

endif


# Restrict paths
ifeq (,$(filter path all, $(PRODUCTS.$(INTERNAL_PRODUCT).PRODUCT_RESTRICT_VENDOR_FILES)))

ifeq ($(strip $(MTK_CIP_SUPPORT)),yes)
_vendor_check_modules_except_custom :=
$(foreach m, $(_vendor_check_modules), \
  $(if $(filter-out $(TARGET_CUSTOM_OUT)/%, $(ALL_MODULES.$(m).INSTALLED)),\
    $(eval _vendor_check_modules_except_custom += $(m))))
_vendor_check_modules := $(_vendor_check_modules_except_custom)
_vendor_check_modules_except_custom :=
endif

$(foreach m, $(_vendor_check_modules), \
  $(if $(filter vendor/%, $(ALL_MODULES.$(m).PATH)),\
    $(if $(filter-out FAKE, $(ALL_MODULES.$(m).CLASS)),\
      $(if $(filter-out ,$(ALL_MODULES.$(m).INSTALLED)),\
        $(if $(filter $(TARGET_OUT_VENDOR)/% $(HOST_OUT)/%, $(ALL_MODULES.$(m).INSTALLED)),,\
          $(info Warning: vendor module "$(m)" in $(ALL_MODULES.$(m).PATH) \
            in product "$(TARGET_PRODUCT)" being installed to \
            $(ALL_MODULES.$(m).INSTALLED) which is not in the vendor tree))))))

endif

_vendor_module_owner_info_txt := $(call intermediates-dir-for,PACKAGING,vendor_owner_info)/vendor_owner_info.txt
$(_vendor_module_owner_info_txt): PRIVATE_INFO := $(_vendor_module_owner_info)
$(_vendor_module_owner_info_txt):
	@echo "Write vendor module owner info $@"
	@mkdir -p $(dir $@) && rm -f $@
ifdef _vendor_module_owner_info
	@for w in $(PRIVATE_INFO); \
	  do \
	    echo $$w >> $@; \
	done
else
	@echo "No vendor module owner info." > $@
endif

$(call dist-for-goals, droidcore, $(_vendor_module_owner_info_txt))

_vendor_module_owner_info_txt :=
_vendor_module_owner_info :=
_vendor_check_modules :=

$(info MTK vendor module check - end)

endif

endif
