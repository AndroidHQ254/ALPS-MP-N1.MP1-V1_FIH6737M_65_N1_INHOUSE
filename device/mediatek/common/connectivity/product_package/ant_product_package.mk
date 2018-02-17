# Add for ANT+
ifeq ($(strip $(MTK_ANT_SUPPORT)), yes)

  PRODUCT_PACKAGES += com.dsi.ant.antradio_library \
                      AntHalService \
                      libantradio \
                      antradio_app

  ant_patch_folder := vendor/mediatek/proprietary/hardware/connectivity/ANT/RAM
ifneq ($(filter MT6630,$(MTK_COMBO_CHIP)),)
  PRODUCT_COPY_FILES += $(ant_patch_folder)/ANT_RAM_CODE_E1.BIN:$(TARGET_COPY_OUT_VENDOR)/firmware/ANT_RAM_CODE_E1.BIN:mtk
  PRODUCT_COPY_FILES += $(ant_patch_folder)/ANT_RAM_CODE_E2.BIN:$(TARGET_COPY_OUT_VENDOR)/firmware/ANT_RAM_CODE_E2.BIN:mtk
endif
ifneq ($(filter CONSYS_6797,$(MTK_COMBO_CHIP)),)
  PRODUCT_COPY_FILES += $(ant_patch_folder)/ANT_RAM_CODE_CONN_V1.BIN:$(TARGET_COPY_OUT_VENDOR)/firmware/ANT_RAM_CODE_CONN_V1.BIN:mtk
endif

  ant_radio_folder := vendor/mediatek/proprietary/external/ant-wireless/antradio-library
  PRODUCT_COPY_FILES += $(ant_radio_folder)/com.dsi.ant.antradio_library.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/com.dsi.ant.antradio_library.xml:dynastream

endif
