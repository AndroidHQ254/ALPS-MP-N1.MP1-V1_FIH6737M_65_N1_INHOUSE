/* Copyright Statement:
 * *
 * * This software/firmware and related documentation ("MediaTek Software") are
 * * protected under relevant copyright laws. The information contained herein
 * * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * * Without the prior written permission of MediaTek inc. and/or its licensors,
 * * any reproduction, modification, use or disclosure of MediaTek Software,
 * * and information contained herein, in whole or in part, shall be strictly prohibited.
 * *
 * * MediaTek Inc. (C) 2010. All rights reserved.
 * *
 * * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER ON
 * * AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
 * * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 * * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 * * NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 * * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 * * SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
 * * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
 * * THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
 * * CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
 * * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
 * * CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
 * * AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
 * * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * * MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 * *
 * * The following software/firmware and/or related documentation ("MediaTek Software")
 * * have been modified by MediaTek Inc. All revisions are subject to any receiver's
 * * applicable license agreements with MediaTek Inc.
 * */


/******************************************************************************
 *
 *  This file contains functions for the MTK defined interop function
 *
 ******************************************************************************/
#define LOG_TAG "bt_device_interop_mtk"

#include <stdlib.h>
#include <string.h>
#include <stddef.h>

#if MTK_STACK_CONFIG_BL == TRUE
#include "bdaddr.h"
#include "bt_types.h"
#include "osi/include/log.h"
#include "btif/include/btif_config.h"
#include "btm_api.h"
#include "interop_mtk.h"

bt_status_t btmtk_get_ble_device_name(const bt_bdaddr_t *remote_bd_addr, BD_NAME bd_name)
{
    bdstr_t bdstr;
    int length = BD_NAME_LEN;
    bdaddr_to_string(remote_bd_addr, bdstr, sizeof(bdstr));
    int ret = btif_config_get_str(bdstr, "Name", (char*)bd_name, &length);
    return ret ? BT_STATUS_SUCCESS : BT_STATUS_FAIL;
}

bool interop_mtk_match_name(const interop_feature_t feature, const bt_bdaddr_t* addr)
{
    char *dev_name_str;
    BD_NAME remote_name = {0};
    BD_ADDR bd_addr;

    bdcpy(bd_addr, addr->address);
    dev_name_str = BTM_SecReadDevName(bd_addr);

    //need to load ble device name from config
    if (dev_name_str == NULL || dev_name_str[0] == '\0')
    {
        dev_name_str = (char *)remote_name;
        btmtk_get_ble_device_name(addr, remote_name);
    }

    if (dev_name_str != NULL && dev_name_str[0] != '\0')
    {
        char bdstr[20] = {0};
        LOG_DEBUG(LOG_TAG, "%s match device %s(%s) for interop workaround.", __func__,
            dev_name_str, bdaddr_to_string(addr, bdstr, sizeof(bdstr)));

        return interop_match_name(feature, (const char *)dev_name_str);
    }
    return false;
}

bool interop_mtk_match_addr_name(const interop_feature_t feature, const bt_bdaddr_t* addr)
{
    if(interop_match_addr(feature, addr))
        return true;

    return interop_mtk_match_name(feature, addr);
}
#endif
