/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein is
 * confidential and proprietary to MediaTek Inc. and/or its licensors. Without
 * the prior written permission of MediaTek inc. and/or its licensors, any
 * reproduction, modification, use or disclosure of MediaTek Software, and
 * information contained herein, in whole or in part, shall be strictly
 * prohibited.
 *
 * MediaTek Inc. (C) 2015. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER
 * ON AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL
 * WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR
 * NONINFRINGEMENT. NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH
 * RESPECT TO THE SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY,
 * INCORPORATED IN, OR SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES
 * TO LOOK ONLY TO SUCH THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO.
 * RECEIVER EXPRESSLY ACKNOWLEDGES THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO
 * OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES CONTAINED IN MEDIATEK
 * SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE
 * RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S
 * ENTIRE AND CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE
 * RELEASED HEREUNDER WILL BE, AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE
 * MEDIATEK SOFTWARE AT ISSUE, OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE
 * CHARGE PAID BY RECEIVER TO MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek
 * Software") have been modified by MediaTek Inc. All revisions are subject to
 * any receiver's applicable license agreements with MediaTek Inc.
 */
#if (MTK_A2DP_SRC_AAC_CODEC == TRUE)

#include "a2d_aac.h"
#include "bta_av_api.h"
#include <utils/Log.h>

/******************************************************************************
**
** Function         A2D_Bld_AACInfo
**
******************************************************************************/

UINT8 A2D_BldAACInfo(UINT8 media_type, tA2D_AAC_CIE *p_ie, UINT8 *p_result)
{
    UINT8 status = 0;
    status = A2D_SUCCESS;
    *p_result++ = BTA_AV_CO_AAC_CODEC_LEN;
    *p_result++ = media_type;
    *p_result++ = BTA_AV_CODEC_M24;
    *p_result++ = (UINT8)(p_ie->object_type);
    *p_result++ = (UINT8)((p_ie->samp_freq & 0xFF00) >> 8);
    *p_result++ = (UINT8)((p_ie->samp_freq & 0x00FF) + p_ie->channels);
    *p_result++ = (UINT8)(p_ie->bit_rate_high);
    *p_result++ = (UINT8)(p_ie->bit_rate_mid);
    *p_result++ = (UINT8)(p_ie->bit_rate_low);
    return status;
}

/******************************************************************************
**
** Function         A2D_ParsAACInfo
**
******************************************************************************/

tA2D_STATUS A2D_ParsAACInfo(tA2D_AAC_CIE *p_ie, UINT8 *p_info, BOOLEAN for_caps)
{
    // Creck: todo
    tA2D_STATUS status;
    status = A2D_SUCCESS;
    return status;
}


/*******************************************************************************
**
** Function         AAC_Encoder_Init
**
** Description      This function initialises the AAC encoder
**
** Returns          HANDLE_AACENCODER
**
*******************************************************************************/
HANDLE_AACENCODER AAC_Encoder_Init(AAC_ENC_PARAMS params)
{
    //1. Open AAC Encoder
    if (AACENC_OK != aacEncOpen(&params.aacEncoder, 0, 2))
    {
        ALOGE("aacEncOpen failed!");
    }

    //2. Set AAC Profile
    if (AACENC_OK != aacEncoder_SetParam(params.aacEncoder, AACENC_AOT, AOT_AAC_LC))
    {
        ALOGE("Set AAC Profile Failed!");
    }

    //3. Set AAC Sample Rate
    if (AACENC_OK != aacEncoder_SetParam(params.aacEncoder, AACENC_SAMPLERATE, params.u16SamplingFreq))
    {
        ALOGE("Set AAC Sample Rate Failed");
    }

    //4. Set AAC Bitrate
    if (AACENC_OK != aacEncoder_SetParam(params.aacEncoder, AACENC_BITRATE, params.u32BitRate))
    {
        ALOGE("Set AAC Bitrate Failed");
    }

    //5. Set AAC Channel Mode
    if (AACENC_OK != aacEncoder_SetParam(params.aacEncoder, AACENC_CHANNELMODE, params.u16ChannelMode))
    {
        ALOGE("Set AAC Channel Mode Failed");
    }

    //6. Set AAC Bitstream format
    if (AACENC_OK != aacEncoder_SetParam(params.aacEncoder, AACENC_TRANSMUX, TT_MP4_LATM_MCP0))
    {
        ALOGE("Set AAC Sample Rate Failed");
    }

    //7. Init AAC encoder parameter
    if (AACENC_OK != aacEncEncode(params.aacEncoder, NULL, NULL, NULL, NULL) )
    {
        ALOGE("accEncEncode init Failed");
    }
    return params.aacEncoder;
}




/*******************************************************************************
**
** Function         AAC_Encoder_Deinit
**
** Description      This function de-initialises the AAC encoder
**
** Returns          void.
**
*******************************************************************************/
void AAC_Encoder_Deinit(AAC_ENC_PARAMS params)
{
    if (AACENC_INVALID_HANDLE == (UINT32) params.aacEncoder)
    {
        ALOGE("Invalid aacEncoder handler");
        return;
    }
    else if (AACENC_OK != aacEncClose(&params.aacEncoder))
    {
        ALOGE("Close AAC Encoder Error");
    }
}
#endif //#if (MTK_A2DP_SRC_APTX_CODEC == TRUE)