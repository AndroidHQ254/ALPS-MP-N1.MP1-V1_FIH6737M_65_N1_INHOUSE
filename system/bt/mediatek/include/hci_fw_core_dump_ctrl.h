#pragma once
#include "hci/include/vendor.h"
#include "mdroid_buildcfg.h"

#if (MTK_SUPPORT_FW_CORE_DUMP == TRUE)

bool is_chip_doing_reset();

bool handleFirmwareReset(int error_code);

bool triggerFirmwareAssert();

void setFimrwareCoreDumpVendor(const vendor_t * vendor);

#endif
