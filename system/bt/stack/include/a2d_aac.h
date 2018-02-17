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
 *  This is the interface to utility functions for dealing with AAC data
 *  frames and codec capabilities.
 *
 ******************************************************************************/
#ifndef A2D_AAC_H
#define A2D_AAC_H
#include "utl.h"
#include "aacenc_lib.h"
#include "mdroid_buildcfg.h"

/*****************************************************************************
**  macros
*****************************************************************************/
typedef short SINT16;
typedef long SINT32;


/*****************************************************************************
**  constants
*****************************************************************************/
#define BTA_AV_CO_AAC_CODEC_LEN (8)

#define AAC_MPEG2_LC (0x80)
#define AAC_MPEG4_LC (0x40)
#define AAC_MPEG4_LTP (0x20)
#define AAC_MPEG4_SCA (0x10)

#define AAC_SAMPLE_FREQ_96k (0x1000)
#define AAC_SAMPLE_FREQ_88k (0x2000)
#define AAC_SAMPLE_FREQ_64k (0x4000)
#define AAC_SAMPLE_FREQ_48k (0x8000)
#define AAC_SAMPLE_FREQ_44k (0x0100)
#define AAC_SAMPLE_FREQ_32k (0x0200)
#define AAC_SAMPLE_FREQ_24k (0x0400)
#define AAC_SAMPLE_FREQ_22k (0x0800)
#define AAC_SAMPLE_FREQ_16k (0x0010)
#define AAC_SAMPLE_FREQ_12k (0x0020)
#define AAC_SAMPLE_FREQ_11k (0x0040)
#define AAC_SAMPLE_FREQ_8k   (0x0080)

#define AAC_CHANNEL_1 (0x08)
#define AAC_CHANNEL_2 (0x04)

#define AAC_DEFAULT_BITRATE_HIGH (0x86) // VBR and bitrate
#define AAC_DEFAULT_BITRATE_MID (0)  // bitrate
#define AAC_DEFAULT_BITRATE_LOW (0) // bitrate



typedef struct
{
    UINT8 object_type;
    UINT16 samp_freq;            /* Sampling Frequency Octet1 & Octet2*/
    UINT8 channels;
    UINT32 bit_rate_high;
    UINT32 bit_rate_mid;
    UINT32 bit_rate_low;
} tA2D_AAC_CIE;


typedef struct
{
    UINT16 u16SamplingFreq;        /* 16k, 32k, 44.1k or 48k*/
    UINT16 u16ChannelMode;        /* mono, dual, streo or joint streo*/
    UINT32 u32BitRate;
    UINT16 *ps16NextPcmBuffer;
    UINT16 as16PcmBuffer[MTK_A2DP_AAC_ENC_INPUT_BUF_SIZE];

    UINT8  *pu8Packet;
    UINT8  *pu8NextPacket;
    UINT16 u16PacketLength;
    HANDLE_AACENCODER aacEncoder;
} AAC_ENC_PARAMS;


UINT8 A2D_BldAACInfo(UINT8 media_type, tA2D_AAC_CIE *p_ie, UINT8 *p_result);
UINT8 A2D_ParsAACInfo(tA2D_AAC_CIE *p_ie, UINT8 *p_info, BOOLEAN for_caps);
HANDLE_AACENCODER AAC_Encoder_Init(AAC_ENC_PARAMS params);
void AAC_Encoder_Deinit(AAC_ENC_PARAMS params);

UINT8 bta_av_get_current_codec();
UINT8* bta_av_get_current_codecInfo();
BOOLEAN bta_av_co_audio_get_aac_aptx_config(UINT8 *p_config, UINT16 *p_minmtu, UINT8 type);

#endif /* A2D_AAC_H */
