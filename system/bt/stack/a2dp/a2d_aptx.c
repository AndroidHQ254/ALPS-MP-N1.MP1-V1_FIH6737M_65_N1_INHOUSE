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


/******************************************************************************
 *
 *  Utility functions to help build and parse the aptX Codec Information
 *  Element and Media Payload.
 *
 ******************************************************************************/
#include "bt_target.h"

#include <string.h>
#include "a2d_api.h"
#include "a2d_aptx.h"
#include "aptXbtenc.h"
#include <utils/Log.h>
#include <stdlib.h>
typedef UINT16  CsrCodecType;
#define CSR_APTX_CODEC_ID_BLUETOOTH   ((CsrCodecType) 0x0001)

#define CSR_CHANNELS_STEREO                                              (0x02)

/******************************************************************************
**
** Function         A2D_Bld_CSR_APTXInfo
**
******************************************************************************/

UINT8 A2D_BldAptxInfo(UINT8 media_type, tA2D_APTX_CIE *p_ie, UINT8 *p_result)
{
    UINT8 status = 0;
    status = A2D_SUCCESS;
    *p_result++ = BTA_AV_CO_APTX_CODEC_LEN;
    *p_result++ = media_type;
    *p_result++ = NON_A2DP_MEDIA_CT;
    *p_result++ = (UINT8)(p_ie->vendorId & 0x000000FF);
    *p_result++ = (UINT8)(p_ie->vendorId & 0x0000FF00) >> 8;
    *p_result++ = (UINT8)(p_ie->vendorId & 0x00FF0000) >> 16;
    *p_result++ = (UINT8)(p_ie->vendorId & 0xFF000000) >> 24;
    *p_result++ = (UINT8)(p_ie->codecId & 0x00FF);
    *p_result++ = (UINT8)(p_ie->codecId & 0xFF00) >> 8;
    *p_result++ = p_ie->sampleRate | p_ie->channelMode;
    return status;
}

/******************************************************************************
**
** Function         A2D_ParsAptxInfo
**
******************************************************************************/
tA2D_STATUS A2D_ParsAptxInfo(tA2D_APTX_CIE *p_ie, UINT8 *p_info, BOOLEAN for_caps)
{
    tA2D_STATUS status;
    UINT8   losc;
    UINT8   mt;

    if (p_ie == NULL || p_info == NULL)
        status = A2D_INVALID_PARAMS;
    else
    {
        losc    = *p_info++;
        mt      = *p_info++;
        ALOGD("losc %d, mt %02x", losc, mt);
        /* If the function is called for the wrong Media Type or Media Codec Type */
        if (losc != BTA_AV_CO_APTX_CODEC_LEN || *p_info != NON_A2DP_MEDIA_CT)
        {
            ALOGE("A2D_ParsAptxInfo: wrong media type %02x", *p_info);
            status = A2D_WRONG_CODEC;
        }
        else
        {
            p_info++;
            p_ie->vendorId = (*p_info & 0x000000FF) |
                             (*(p_info + 1) << 8    & 0x0000FF00) |
                             (*(p_info + 2) << 16  & 0x00FF0000) |
                             (*(p_info + 3) << 24  & 0xFF000000);
            p_info = p_info + 4;
            p_ie->codecId = (*p_info & 0x00FF) | (*(p_info + 1) << 8 & 0xFF00);
            p_info = p_info + 2;
            p_ie->channelMode = *p_info & 0x0F;
            p_ie->sampleRate = *p_info & 0xF0;

            status = A2D_SUCCESS;

            if (for_caps == FALSE)
            {
                if (A2D_BitsSet(p_ie->sampleRate) != A2D_SET_ONE_BIT)
                    status = A2D_BAD_SAMP_FREQ;
                if (A2D_BitsSet(p_ie->channelMode) != A2D_SET_ONE_BIT)
                    status = A2D_BAD_CH_MODE;
            }
        }
    }
    return status;
}

/*******************************************************************************
**
** Function         APTX_Encoder_Init
**
** Description      This function initialises the aptX encoder
**
** Returns          void.
**
*******************************************************************************/
void APTX_Encoder_Init(APTX_ENC_PARAMS params)
{
    ALOGD("APTX_Encoder_Init");
    /*void * aptxCodec;
    long long frame_duration_usec;
    int media_packet_header_size;
    UINT16 frameSize;
    int size;*/

    params.aptxEncoder = malloc((size_t)SizeofAptxbtenc());
    if (params.aptxEncoder)
        aptxbtenc_init(params.aptxEncoder, 0);
    else
        ALOGE("APTX_Encoder_Init: failed to create encoder");
}

/*******************************************************************************
**
** Function         bta_av_aptx_cfg_in_cap
**
** Description      This function checks whether an aptX codec configuration
**                  is allowable for the given codec capabilities.
**
** Returns          0 if ok, nonzero if error.
**
*******************************************************************************/
UINT8 bta_av_aptx_cfg_in_cap(UINT8 *p_cfg, tA2D_APTX_CIE *p_cap)
{
    UINT8           status = 0;
    tA2D_APTX_CIE    cfg_cie;

    /* parse configuration */
    if ((status = A2D_ParsAptxInfo(&cfg_cie, p_cfg, FALSE)) != 0)
    {
        ALOGE("bta_av_aptx_cfg_in_cap, aptx parse failed");
        return status;
    }

    /* verify that each parameter is in range */

    /* sampling frequency */
    if ((cfg_cie.sampleRate & p_cap->sampleRate) == 0)
    {
        status = A2D_NS_SAMP_FREQ;
    }
    /* channel mode */
    else if ((cfg_cie.channelMode & p_cap->channelMode) == 0)
    {
        status = A2D_NS_CH_MODE;
    }
    return status;
}