

# -----------------------------------------------------------------
# cip-build.prop
#
# add build time for custom.img

cxp_support := no

ifdef MTK_CARRIEREXPRESS_PACK
  ifneq ($(strip $(MTK_CARRIEREXPRESS_PACK)),no)
    cxp_support := yes
  endif
endif

ifdef MTK_CARRIEREXPRESS_PACK
ifeq ($(strip $(MTK_CARRIEREXPRESS_PACK)),no)
  include vendor/mediatek/proprietary/operator/common/build/CIP-Properties.mk
endif
else
  include vendor/mediatek/proprietary/operator/common/build/CIP-Properties.mk
endif

ifeq ($(strip $(MTK_CIP_SUPPORT)),yes)
  USP_PATH := $(TARGET_CUSTOM_OUT)/usp
  INSTALLED_BUILD_CIP_PROP_TARGET := $(TARGET_CUSTOM_OUT)/cip-build.prop
else
  USP_PATH := $(TARGET_OUT_VENDOR)/usp
  INSTALLED_BUILD_CIP_PROP_TARGET := $(TARGET_OUT_VENDOR)/cip-build.prop
endif

ifdef MTK_CARRIEREXPRESS_PACK
ifneq ($(strip $(MTK_CARRIEREXPRESS_PACK)),no)
USP_INFO_TARGET := $(USP_PATH)/usp-info.txt
$(USP_INFO_TARGET): $(INSTALLED_BUILD_CIP_PROP_TARGET)
	@mkdir -p $(dir $@)
	$(hide) echo "MTK_REGIONAL_OP_PACK=$(MTK_REGIONAL_OP_PACK)" > $@
	> $(USP_PATH)/usp-packages-all.txt
# save default operator
	$(eval temp_optr := $(OPTR_SPEC_SEG_DEF))
	$(foreach OP_SPEC, $(MTK_REGIONAL_OP_PACK), \
        $(eval OPTR_SPEC_SEG_DEF := $(OP_SPEC)) \
        $(eval OPTR     := $(word 1, $(subst _,$(space),$(OP_SPEC)))) \
        $(eval SPEC     := $(word 2, $(subst _,$(space),$(OP_SPEC)))) \
        $(eval SEG     := $(word 3, $(subst _,$(space),$(OP_SPEC)))) \
        $(eval include vendor/mediatek/proprietary/operator/common/build/UspPackagesInfo.mk) \
        echo "[Package-start]" > $(USP_PATH)/usp-content-$(OPTR).txt; \
        > $(USP_PATH)/usp-apks-path-$(OPTR).txt; \
        $(warning list of packages: $(USP_OPERATOR_PACKAGES)) \
        $(foreach item, $(USP_OPERATOR_PACKAGES), \
            echo $(item) >> $(USP_PATH)/usp-content-$(OPTR).txt; \
            echo $(item) >> $(USP_PATH)/usp-packages-all.txt;) \
        echo "[Package-end]" >> $(USP_PATH)/usp-content-$(OPTR).txt; \
        $(eval include vendor/mediatek/proprietary/operator/common/build/UspApkPathInfo.mk) \
        $(foreach item, $(USP_OPERATOR_APK_PATH), \
            echo $(item) >> $(USP_PATH)/usp-apks-path-$(OPTR).txt;) \
        echo "[Property-start]" >> $(USP_PATH)/usp-content-$(OPTR).txt; \
        echo "persist.operator.optr=$(OPTR)" >> $(USP_PATH)/usp-content-$(OPTR).txt; \
        echo "persist.operator.spec=$(SPEC)" >> $(USP_PATH)/usp-content-$(OPTR).txt; \
        echo "persist.operator.seg=$(SEG)" >> $(USP_PATH)/usp-content-$(OPTR).txt; \
        $(eval include vendor/mediatek/proprietary/operator/common/build/CIPconfig.mk) \
        $(eval include vendor/mediatek/proprietary/operator/common/build/CIP-Properties.mk) \
        $(foreach item, $(CIP_PROPERTY_OVERRIDES), \
            echo $(item) >> $(USP_PATH)/usp-content-$(OPTR).txt;) \
        echo "[Property-end]" >> $(USP_PATH)/usp-content-$(OPTR).txt; \
        )
# restor default operator
	$(eval OPTR_SPEC_SEG_DEF := $(temp_optr))

# Handle OM case
  ifeq ($(strip $(OPTR_SPEC_SEG_DEF)),NONE)
	$(eval include vendor/mediatek/proprietary/operator/common/build/UspPackagesInfo.mk)
	$(foreach item, $(USP_OPERATOR_PACKAGES), \
      echo "[Package-start]" > $(USP_PATH)/usp-content-OM.txt; \
      echo $(item) >> $(USP_PATH)/usp-content-OM.txt; \
      echo $(item) >> $(USP_PATH)/usp-packages-all.txt; \
      echo "[Package-end]" >> $(USP_PATH)/usp-content-OM.txt;)
  endif
else
USP_INFO_TARGET := $(USP_PATH)/usp-info.txt
$(USP_INFO_TARGET):
	@mkdir -p $(dir $@)
endif
else
USP_INFO_TARGET := $(USP_PATH)/usp-info.txt
$(USP_INFO_TARGET):
	@mkdir -p $(dir $@)
endif

$(INSTALLED_BUILD_CIP_PROP_TARGET):
	@mkdir -p $(dir $@)
	$(hide) echo "ro.cip.build.date=`date`" > $@
  ifdef OPTR_SPEC_SEG_DEF
  ifneq ($(strip $(OPTR_SPEC_SEG_DEF)),NONE)
  ifdef MTK_CARRIEREXPRESS_PACK
  ifneq ($(strip $(MTK_CARRIEREXPRESS_PACK)),no)
	$(hide) echo "persist.mtk_usp_md_sbp_code=$(subst OP,,$(OPTR))" >> $@
  endif
  endif
	$(hide) echo "persist.operator.optr=$(OPTR)" >> $@
	$(hide) echo "persist.operator.spec=$(SPEC)" >> $@
	$(hide) echo "persist.operator.seg=$(SEG)" >> $@
	$(hide) echo "ro.mtk_md_sbp_custom_value=$(patsubst OP%,%,$(OPTR))" >> $@
  endif
  endif
  ifdef MTK_CARRIEREXPRESS_PACK
  ifneq ($(strip $(MTK_CARRIEREXPRESS_PACK)),no)
	$(hide) echo "ro.mtk_carrierexpress_pack=$(strip $(MTK_CARRIEREXPRESS_PACK))" >> $@
# restore default operator feature options still need to handle the case for optr none
	$(eval include vendor/mediatek/proprietary/operator/common/build/CIPconfig.mk)
	$(eval include vendor/mediatek/proprietary/operator/common/build/CIP-Properties.mk)
  endif
  endif
# need to modify accordingly later as currently used based on words rather in format "x=y"
	$(foreach item, $(CIP_PROPERTY_OVERRIDES), echo $(item) >> $@;)

ifeq ($(TARGET_USERIMAGES_USE_EXT4),true)

customimage_intermediates := \
    $(call intermediates-dir-for,PACKAGING,custom)

## Generate an ext4 image
define build-customimage-target
    mkdir -p $(TARGET_CUSTOM_OUT)
    mkdir -p $(TARGET_CUSTOM_OUT)/lib
    mkdir -p $(TARGET_CUSTOM_OUT)/lib64
    mkdir -p $(TARGET_CUSTOM_OUT)/app
    mkdir -p $(TARGET_CUSTOM_OUT)/framework
    mkdir -p $(TARGET_CUSTOM_OUT)/plugin
    mkdir -p $(TARGET_CUSTOM_OUT)/media
    mkdir -p $(TARGET_CUSTOM_OUT)/etc
    mkdir -p $(TARGET_CUSTOM_OUT)/usp
    mkuserimg.sh -s $(PRODUCT_OUT)/custom $(PRODUCT_OUT)/custom.img ext4 custom $(strip $(BOARD_CUSTOMIMAGE_PARTITION_SIZE)) $(PRODUCT_OUT)/root/file_contexts
endef

INSTALLED_CUSTOMIMAGE_TARGET := $(PRODUCT_OUT)/custom.img

INTERNAL_CUSTOMIMAGE_FILES := $(filter $(TARGET_CUSTOM_OUT)/%,$(ALL_PREBUILT) $(ALL_COPIED_HEADERS) $(ALL_GENERATED_SOURCES) $(ALL_DEFAULT_INSTALLED_MODULES) $(INSTALLED_BUILD_CIP_PROP_TARGET) $(USP_INFO_TARGET))

$(INSTALLED_CUSTOMIMAGE_TARGET) : $(INTERNAL_USERIMAGES_DEPS) $(INTERNAL_CUSTOMIMAGE_FILES)
	$(build-customimage-target)

ifeq ($(strip $(MTK_CIP_SUPPORT)),yes)
droidcore: $(INSTALLED_CUSTOMIMAGE_TARGET)
endif

ifdef MTK_CARRIEREXPRESS_PACK
ifneq ($(strip $(MTK_CARRIEREXPRESS_PACK)),no)
  ifneq ($(strip $(MTK_CIP_SUPPORT)),yes)
    ifdef BOARD_VENDORIMAGE_FILE_SYSTEM_TYPE
      $(BUILT_VENDORIMAGE_TARGET): $(USP_INFO_TARGET)
    else
      $(BUILT_SYSTEMIMAGE): $(USP_INFO_TARGET)
    endif
  endif
endif
endif

ifneq ($(strip $(MTK_PROJECT_NAME)),)
-include $(TARGET_OUT_INTERMEDIATES)/PTGEN/partition_size.mk

ALL_CUSTOMIMAGE_CLEAN_FILES := \
        $(PRODUCT_OUT)/custom.img \
        $(TARGET_CUSTOM_OUT) \
        $(TARGET_OUT_COMMON_INTERMEDIATES)/JAVA_LIBRARIES/mediatek-op_intermediates \
        $(TARGET_OUT_INTERMEDIATES)/ETC/DmApnInfo.xml_intermediates \
        $(TARGET_OUT_INTERMEDIATES)/ETC/smsSelfRegConfig.xml_intermediates \
        $(TARGET_OUT_INTERMEDIATES)/ETC/CIP_MD_SBP_intermediates


clean-customimage:
	$(hide) echo $(SHOWTIME) $@ing ...
	$(hide) echo ALL_CUSTOMIMAGE_CLEAN_FILES=$(ALL_CUSTOMIMAGE_CLEAN_FILES)
	$(hide) rm -rf $(ALL_CUSTOMIMAGE_CLEAN_FILES)

endif

.PHONY: customimage
customimage: $(INTERNAL_USERIMAGES_DEPS) $(INTERNAL_CUSTOMIMAGE_FILES)
	$(build-customimage-target)

.PHONY: all_customimage
all_customimage:
	echo build all customimage for $(MTK_TARGET_PROJECT)
	perl vendor/mediatek/proprietary/operator/common/build/CIPbuild.pl -ini=vendor/mediatek/proprietary/operator/common/build/$(MTK_TARGET_PROJECT).ini -p=$(MTK_TARGET_PROJECT) -pf=$(TARGET_BOARD_PLATFORM) -cxp=$(MTK_CARRIEREXPRESS_PACK)

ifeq ($(strip $(cxp_support)),no)

.PHONY: op03_customimage
op03_customimage:
	echo build op3 customimage for $(MTK_TARGET_PROJECT)
	perl vendor/mediatek/proprietary/operator/common/build/CIPbuild.pl -ini=vendor/mediatek/proprietary/operator/common/build/$(MTK_TARGET_PROJECT).ini -p=$(MTK_TARGET_PROJECT) -op=OP03 -pf=$(TARGET_BOARD_PLATFORM)

.PHONY: op06_customimage
op06_customimage:
	echo build op06 customimage for $(MTK_TARGET_PROJECT)
	perl vendor/mediatek/proprietary/operator/common/build/CIPbuild.pl -ini=vendor/mediatek/proprietary/operator/common/build/$(MTK_TARGET_PROJECT).ini -p=$(MTK_TARGET_PROJECT) -op=OP06 -pf=$(TARGET_BOARD_PLATFORM)

.PHONY: op07_customimage
op07_customimage:
	echo build op07 customimage for $(MTK_TARGET_PROJECT)
	perl vendor/mediatek/proprietary/operator/common/build/CIPbuild.pl -ini=vendor/mediatek/proprietary/operator/common/build/$(MTK_TARGET_PROJECT).ini -p=$(MTK_TARGET_PROJECT) -op=OP07 -pf=$(TARGET_BOARD_PLATFORM)

.PHONY: op08_customimage
op08_customimage:
	echo build op08 customimage for $(MTK_TARGET_PROJECT)
	perl vendor/mediatek/proprietary/operator/common/build/CIPbuild.pl -ini=vendor/mediatek/proprietary/operator/common/build/$(MTK_TARGET_PROJECT).ini -p=$(MTK_TARGET_PROJECT) -op=OP08 -pf=$(TARGET_BOARD_PLATFORM)

.PHONY: op11_customimage
op11_customimage:
	echo build op11 customimage for $(MTK_TARGET_PROJECT)
	perl vendor/mediatek/proprietary/operator/common/build/CIPbuild.pl -ini=vendor/mediatek/proprietary/operator/common/build/$(MTK_TARGET_PROJECT).ini -p=$(MTK_TARGET_PROJECT) -op=OP11 -pf=$(TARGET_BOARD_PLATFORM)

.PHONY: op17_customimage
op17_customimage:
	echo build op17 customimage for $(MTK_TARGET_PROJECT)
	perl vendor/mediatek/proprietary/operator/common/build/CIPbuild.pl -ini=vendor/mediatek/proprietary/operator/common/build/$(MTK_TARGET_PROJECT).ini -p=$(MTK_TARGET_PROJECT) -op=OP17 -pf=$(TARGET_BOARD_PLATFORM)

.PHONY: op18_customimage
op18_customimage:
	echo build op18 customimage for $(MTK_TARGET_PROJECT)
	perl vendor/mediatek/proprietary/operator/common/build/CIPbuild.pl -ini=vendor/mediatek/proprietary/operator/common/build/$(MTK_TARGET_PROJECT).ini -p=$(MTK_TARGET_PROJECT) -op=OP18 -pf=$(TARGET_BOARD_PLATFORM)

.PHONY: op105_customimage
op105_customimage:
	echo build op105 customimage for $(MTK_TARGET_PROJECT)
	perl vendor/mediatek/proprietary/operator/common/build/CIPbuild.pl -ini=vendor/mediatek/proprietary/operator/common/build/$(MTK_TARGET_PROJECT).ini -p=$(MTK_TARGET_PROJECT) -op=OP105 -pf=$(TARGET_BOARD_PLATFORM)

.PHONY: op15_customimage
op15_customimage:
	echo build op15 customimage for $(MTK_TARGET_PROJECT)
	perl vendor/mediatek/proprietary/operator/common/build/CIPbuild.pl -ini=vendor/mediatek/proprietary/operator/common/build/$(MTK_TARGET_PROJECT).ini -p=$(MTK_TARGET_PROJECT) -op=OP15 -pf=$(TARGET_BOARD_PLATFORM)

PHONY: op16_customimage
op16_customimage:
	echo build op16 customimage for $(MTK_TARGET_PROJECT)
	perl vendor/mediatek/proprietary/operator/common/build/CIPbuild.pl -ini=vendor/mediatek/proprietary/operator/common/build/$(MTK_TARGET_PROJECT).ini -p=$(MTK_TARGET_PROJECT) -op=OP16 -pf=$(TARGET_BOARD_PLATFORM)

.PHONY: op120_customimage
op120_customimage:
	echo build op120 customimage for $(MTK_TARGET_PROJECT)
	perl vendor/mediatek/proprietary/operator/common/build/CIPbuild.pl -ini=vendor/mediatek/proprietary/operator/common/build/$(MTK_TARGET_PROJECT).ini -p=$(MTK_TARGET_PROJECT) -op=OP120 -pf=$(TARGET_BOARD_PLATFORM)

PHONY: op126_customimage
op126_customimage:
	echo build op126 customimage for $(MTK_TARGET_PROJECT)
	perl vendor/mediatek/proprietary/operator/common/build/CIPbuild.pl -ini=vendor/mediatek/proprietary/operator/common/build/$(MTK_TARGET_PROJECT).ini -p=$(MTK_TARGET_PROJECT) -op=OP126 -pf=$(TARGET_BOARD_PLATFORM)

PHONY: op112_customimage
op112_customimage:
	echo build op112 customimage for $(MTK_TARGET_PROJECT)
	perl vendor/mediatek/proprietary/operator/common/build/CIPbuild.pl -ini=vendor/mediatek/proprietary/operator/common/build/$(MTK_TARGET_PROJECT).ini -p=$(MTK_TARGET_PROJECT) -op=OP112 -pf=$(TARGET_BOARD_PLATFORM)

else

PHONY: la_customimage
la_customimage:
	echo build la customimage for $(MTK_TARGET_PROJECT)
	perl vendor/mediatek/proprietary/operator/common/build/CIPbuild.pl -ini=vendor/mediatek/proprietary/operator/common/build/$(MTK_TARGET_PROJECT).ini -p=$(MTK_TARGET_PROJECT) -cxp=$(MTK_CARRIEREXPRESS_PACK) -rp=la

PHONY: na_customimage
na_customimage:
	echo build na customimage for $(MTK_TARGET_PROJECT)
	perl vendor/mediatek/proprietary/operator/common/build/CIPbuild.pl -ini=vendor/mediatek/proprietary/operator/common/build/$(MTK_TARGET_PROJECT).ini -p=$(MTK_TARGET_PROJECT) -cxp=$(MTK_CARRIEREXPRESS_PACK) -rp=na

PHONY: eu_customimage
eu_customimage:
	echo build eu customimage for $(MTK_TARGET_PROJECT)
	perl vendor/mediatek/proprietary/operator/common/build/CIPbuild.pl -ini=vendor/mediatek/proprietary/operator/common/build/$(MTK_TARGET_PROJECT).ini -p=$(MTK_TARGET_PROJECT) -cxp=$(MTK_CARRIEREXPRESS_PACK) -rp=eu

PHONY: ind_customimage
ind_customimage:
	echo build ind customimage for $(MTK_TARGET_PROJECT)
	perl vendor/mediatek/proprietary/operator/common/build/CIPbuild.pl -ini=vendor/mediatek/proprietary/operator/common/build/$(MTK_TARGET_PROJECT).ini -p=$(MTK_TARGET_PROJECT) -cxp=$(MTK_CARRIEREXPRESS_PACK) -rp=ind

PHONY: jpn_customimage
jpn_customimage:
	echo build jpn customimage for $(MTK_TARGET_PROJECT)
	perl vendor/mediatek/proprietary/operator/common/build/CIPbuild.pl -ini=vendor/mediatek/proprietary/operator/common/build/$(MTK_TARGET_PROJECT).ini -p=$(MTK_TARGET_PROJECT) -cxp=$(MTK_CARRIEREXPRESS_PACK) -rp=jpn

PHONY: mea_customimage
mea_customimage:
	echo build mea customimage for $(MTK_TARGET_PROJECT)
	perl vendor/mediatek/proprietary/operator/common/build/CIPbuild.pl -ini=vendor/mediatek/proprietary/operator/common/build/$(MTK_TARGET_PROJECT).ini -p=$(MTK_TARGET_PROJECT) -cxp=$(MTK_CARRIEREXPRESS_PACK) -rp=mea

PHONY: au_customimage
au_customimage:
	echo build au customimage for $(MTK_TARGET_PROJECT)
	perl vendor/mediatek/proprietary/operator/common/build/CIPbuild.pl -ini=vendor/mediatek/proprietary/operator/common/build/$(MTK_TARGET_PROJECT).ini -p=$(MTK_TARGET_PROJECT) -cxp=$(MTK_CARRIEREXPRESS_PACK) -rp=au

PHONY: rus_customimage
rus_customimage:
	echo build rus customimage for $(MTK_TARGET_PROJECT)
	perl vendor/mediatek/proprietary/operator/common/build/CIPbuild.pl -ini=vendor/mediatek/proprietary/operator/common/build/$(MTK_TARGET_PROJECT).ini -p=$(MTK_TARGET_PROJECT) -cxp=$(MTK_CARRIEREXPRESS_PACK) -rp=rus

endif

endif

