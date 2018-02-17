ifeq ($(MTK_INTERNAL),yes)
  MTK_REMAKE_FLAG ?= yes
endif
ifeq ($(strip $(MTK_REMAKE_FLAG)),yes)
# only work in full build
ifeq ($(ONE_SHOT_MAKEFILE)$(dont_bother),)

MTK_REAMKE_MODULE_DEPENDENCIES_OUT := $(PRODUCT_OUT)/obj/REMAKE_DEP
MTK_REMAKE_PREVIOUS_PROJECT_CONFIG_FILE := $(MTK_REAMKE_MODULE_DEPENDENCIES_OUT)/previous_ProjectConfig.txt

$(MTK_REMAKE_PREVIOUS_PROJECT_CONFIG_FILE): PRIVATE_ALL_MODULES := $(ALL_MODULES)
$(MTK_REMAKE_PREVIOUS_PROJECT_CONFIG_FILE): PRIVATE_ALL_DEFAULT_INSTALLED_MODULES := $(patsubst $(OUT_DIR)/%,$$(OUT_DIR)/%,$(patsubst $(PRODUCT_OUT)/%,$$(PRODUCT_OUT)/%,$(ALL_DEFAULT_INSTALLED_MODULES)))
$(MTK_REMAKE_PREVIOUS_PROJECT_CONFIG_FILE):
	@echo Generating $@
	$(hide) mkdir -p $(dir $@)
	$(hide) echo "PREVIOUS_ALL_MODULES=$(PRIVATE_ALL_MODULES)" >$@
	$(hide) echo 'PREVIOUS_ALL_DEFAULT_INSTALLED_MODULES=$(PRIVATE_ALL_DEFAULT_INSTALLED_MODULES)' >>$@

files: $(MTK_REMAKE_PREVIOUS_PROJECT_CONFIG_FILE)

-include $(MTK_REMAKE_PREVIOUS_PROJECT_CONFIG_FILE)
MTK_REMAKE_DELETED_INSTALLED_MODULES := $(filter-out $(ALL_DEFAULT_INSTALLED_MODULES),$(PREVIOUS_ALL_DEFAULT_INSTALLED_MODULES))
MTK_REMAKE_DELETED_ALL_MODULES := $(filter-out $(ALL_MODULES),$(PREVIOUS_ALL_MODULES))

# installclean
ifneq ($(strip $(MTK_REMAKE_DELETED_INSTALLED_MODULES)),)
ifeq ($(filter-out $(INTERNAL_MODIFIER_TARGETS) dist,$(MAKECMDGOALS)),)
  intermediates_clean_files := \
    $(strip $(wildcard $(filter $(OUT_DIR)/%,$(MTK_REMAKE_DELETED_INSTALLED_MODULES))))
  $(info *** rm -rf $(intermediates_clean_files))
  $(shell rm -rf $(intermediates_clean_files))
endif
endif#MTK_REMAKE_DELETED_INSTALLED_MODULES
ifneq ($(strip $(MTK_REMAKE_DELETED_ALL_MODULES)),)
  intermediates_clean_files := \
    $(strip $(wildcard $(foreach m,$(MTK_REMAKE_DELETED_ALL_MODULES),\
      $(foreach c,EXECUTABLES SHARED_LIBRARIES STATIC_LIBRARIES APPS JAVA_LIBRARIES ETC FAKE NOTICE_FILES PACKAGING,\
        $(call intermediates-dir-for,$(c),$(m))\
        $(call intermediates-dir-for,$(c),$(m),,,2ND_)\
        $(call intermediates-dir-for,$(c),$(m),HOST)\
        $(call intermediates-dir-for,$(c),$(m),HOST,,2ND_)\
        $(call intermediates-dir-for,$(c),$(m),,COMMON)\
        $(call intermediates-dir-for,$(c),$(m),HOST,COMMON)\
      )\
    )))
  $(info *** rm -rf $(intermediates_clean_files))
  $(shell rm -rf $(intermediates_clean_files))
endif#MTK_REMAKE_DELETED_ALL_MODULES

endif#ONE_SHOT_MAKEFILE
endif#MTK_REMAKE_FLAG
