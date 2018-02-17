#define LOG_TAG "hci_fw_core_dump_ctrl"

#include "osi/include/log.h"
#include "hci_fw_core_dump_ctrl.h"

#if (MTK_SUPPORT_FW_CORE_DUMP == TRUE)
typedef enum {
  ERRCODE_CHIP_RESET_IS_DOING = 88,
  ERRCODE_CHIP_RESET_IS_DONE = 99
} chiprst_errcode_t;

static volatile bool b_detect_chiprst_in_write = false;
static bool b_trig_coredump = false;
static const vendor_t *m_vendor = NULL;

bool is_chip_doing_reset(void) {
  bool b_in_reset = b_detect_chiprst_in_write || b_trig_coredump;
  LOG_INFO(LOG_TAG, "%s %d", __func__, b_in_reset);
  return b_in_reset;
}

bool handleFirmwareReset(int error_code) {
  switch(error_code) {
  case ERRCODE_CHIP_RESET_IS_DOING: {
    LOG_ERROR(LOG_TAG, "detect bluetooth firmware is doing chip-reset.");
    if (!b_detect_chiprst_in_write) {
        b_detect_chiprst_in_write = true;
    }
    // sleep 10ms to avoid busy loop
    usleep(10000);
    return true;
  }
  case ERRCODE_CHIP_RESET_IS_DONE: {
    LOG_ERROR(LOG_TAG, "%s restarting the bluetooth process.", __func__);
    usleep(10000);
    kill(getpid(), SIGKILL);
    return true;
  }
  }
  return false;
}

/* When fw coredump is triggered, stack doesn't need to kill bt process because that
 * "fw coredump" of vendor library includes coredump and whole-chip-reset,
 * the feature of "MTK_HCITRANS_DETECT_CHIP_RESET" would take care of detecting
 * the end of whole-chip-reset and then kill bt process
 */
bool triggerFirmwareAssert() {
  LOG_INFO(LOG_TAG, "%s Stack triggers firmware coredump!", __func__);
  if (m_vendor == NULL) {
    LOG_INFO(LOG_TAG, "%s Act FW Coredump Fails! Vender not set.", __func__);
    return false;
  }
  int stack_trigger_reason = 31;
  if (!m_vendor->send_command(VENDOR_SET_FW_ASSERT, &stack_trigger_reason)) {
    LOG_INFO(LOG_TAG, "%s Act FW Coredump Success!", __func__);
    b_trig_coredump = true;
    return true;
  }
  LOG_WARN(LOG_TAG, "%s Act FW Coredump Fails!", __func__);
  return false;
}

void setFimrwareCoreDumpVendor(const vendor_t * vendor) {
  m_vendor = vendor;
}
#endif
