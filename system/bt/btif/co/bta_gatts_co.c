/******************************************************************************
 *
 *  Copyright (C) 2009-2013 Broadcom Corporation
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at:
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************/

#include "bta_api.h"

#if( defined BLE_INCLUDED ) && (BLE_INCLUDED == TRUE)
#if( defined BTA_GATT_INCLUDED ) && (BTA_GATT_INCLUDED == TRUE)

#include <stdlib.h>
#include <string.h>
#include "bt_common.h"
#include "bta_gatts_co.h"
#include "btif_util.h"

#if MTK_COMMON == TRUE
#include "btcore/include/bdaddr.h"
#include "btif_config.h"
#endif
/*****************************************************************************
**  Local type definitions
*****************************************************************************/

#define BTIF_GATTS_MAX_SRV_CHG_CLT_SIZE 50

typedef struct
{
    BOOLEAN             enable;
    UINT8               num_clients;
    tBTA_GATTS_SRV_CHG  srv_chg[BTIF_GATTS_MAX_SRV_CHG_CLT_SIZE];
} __attribute__((packed)) btif_gatts_srv_chg_cb_t;

/*****************************************************************************
**  Static variables
*****************************************************************************/

static btif_gatts_srv_chg_cb_t btif_gatts_srv_chg_cb;

/*****************************************************************************
**  Static functions
*****************************************************************************/

static void btif_gatts_check_init(void)
{
    btif_gatts_srv_chg_cb_t *p_cb= &btif_gatts_srv_chg_cb;

    if (!p_cb->enable)
    {
       memset(p_cb, 0, sizeof(btif_gatts_srv_chg_cb_t));
       p_cb->enable = TRUE;
    }
}

/*****************************************************************************
**  Externally called functions
*****************************************************************************/

void btif_gatts_add_bonded_dev_from_nv(BD_ADDR bda)
{
    btif_gatts_srv_chg_cb_t *p_cb= &btif_gatts_srv_chg_cb;
    BOOLEAN                 found = FALSE;
    UINT8                   i;

    btif_gatts_check_init();

#if MTK_COMMON == TRUE
    //Add by MTK: Only load clients configured indication
    bdstr_t bdstr;
    sprintf(bdstr, "%02x:%02x:%02x:%02x:%02x:%02x",
    bda[0], bda[1], bda[2], bda[3], bda[4], bda[5]);

    int clt_cfg;
    if(!btif_config_get_int(bdstr, "SrvChgInd", &clt_cfg) || !clt_cfg)
        return;
#endif

    for (i=0; i != p_cb->num_clients; ++i)
    {
        if (!memcmp(p_cb->srv_chg[i].bda,  bda, sizeof(BD_ADDR)))
        {
            found = TRUE;
            break;
        }
    }

    if (!found)
    {
        if (p_cb->num_clients < BTIF_GATTS_MAX_SRV_CHG_CLT_SIZE)
        {
            bdcpy(p_cb->srv_chg[p_cb->num_clients].bda, bda);
            p_cb->srv_chg[p_cb->num_clients].srv_changed = FALSE;
            p_cb->num_clients++;
        }
    }
}

/*****************************************************************************
**  Call-out functions
*****************************************************************************/

/*******************************************************************************
**
** Function         bta_gatts_co_update_handle_range
**
** Description      This callout function is executed by GATTS when a GATT server
**                  handle range ios to be added or removed.
**
** Parameter        is_add: true is to add a handle range; otherwise is to delete.
**                  p_hndl_range: handle range.
**
** Returns          void.
**
*******************************************************************************/
void bta_gatts_co_update_handle_range(BOOLEAN is_add, tBTA_GATTS_HNDL_RANGE *p_hndl_range)
{
    UNUSED(is_add);
    UNUSED(p_hndl_range);
}

/*******************************************************************************
**
** Function         bta_gatts_co_srv_chg
**
** Description      This call-out is to read/write/remove service change related
**                  informaiton. The request consists of the cmd and p_req and the
**                  response is returned in p_rsp
**
** Parameter        cmd - request command
**                  p_req - request paramters
**                  p_rsp - response data for the request
**
** Returns          TRUE - if the request is processed successfully and
**                         the response is returned in p_rsp.
**                  FASLE - if the request can not be processed
**
*******************************************************************************/
BOOLEAN bta_gatts_co_srv_chg(tBTA_GATTS_SRV_CHG_CMD cmd,
                             tBTA_GATTS_SRV_CHG_REQ *p_req,
                             tBTA_GATTS_SRV_CHG_RSP *p_rsp)
{
#if MTK_COMMON == FALSE
    UNUSED(cmd);
    UNUSED(p_req);
    UNUSED(p_rsp);
#else
    UNUSED(p_rsp);

    //Add by MTK: Loading bonded clients configured indication
    switch(cmd)
    {
        case GATTS_SRV_CHG_CMD_READ_NUM_CLENTS:
            p_rsp->num_clients = btif_gatts_srv_chg_cb.num_clients;
            return TRUE;
        case GATTS_SRV_CHG_CMD_READ_CLENT:
            if(p_req->client_read_index > btif_gatts_srv_chg_cb.num_clients)
                return FALSE;
            memcpy(&p_rsp->srv_chg, &btif_gatts_srv_chg_cb.srv_chg[p_req->client_read_index-1], sizeof(tGATTS_SRV_CHG));
            return TRUE;
    }
#endif
    return FALSE;
}

/*******************************************************************************
**
** Function         bta_gatts_co_load_handle_range
**
** Description      This callout function is executed by GATTS when a GATT server
**                  handle range is requested to be loaded from NV.
**
** Parameter
**
** Returns          void.
**
*******************************************************************************/
BOOLEAN bta_gatts_co_load_handle_range(UINT8 index,
                                       tBTA_GATTS_HNDL_RANGE *p_handle_range)
{
    UNUSED(index);
    UNUSED(p_handle_range);

    return FALSE;
}
#endif
#endif
