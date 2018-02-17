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
 *  This is the interface to utility functions for dealing with APTX data
 *  frames and codec capabilities.
 *
 ******************************************************************************/

#ifndef A2D_APTX_H
#define A2D_APTX_H

/*****************************************************************************
**  Type Definitions
*****************************************************************************/
/* data type for the aptX Codec Information Element*/
#define NON_A2DP_MEDIA_CT 0xff
#define APTX_VENDOR_ID    0
#define APTX_CODEC_ID     1
#define APTX_SAMPLE_RATE  2
#define APTX_CHANNEL      3
#define APTX_FUTURE1      4
#define APTX_FUTURE2      5

typedef unsigned char UINT8;
typedef unsigned short UINT16;
//typedef unsigned long  UINT32;
typedef short SINT16;
typedef long SINT32;
typedef UINT16 CsrCodecType;

/* aptX codec specific settings*/
#define BTA_AV_CO_APTX_CODEC_LEN 9

#define CSR_APTX_VENDOR_ID (0x0000004F)
#define CSR_APTX_CODEC_ID_BLUETOOTH ((CsrCodecType) 0x0001)
#define CSR_APTX_SAMPLERATE_44100 (0x20)
#define CSR_APTX_SAMPLERATE_48000 (0x10)
#define CSR_APTX_CHANNELS_STEREO (0x02)
#define CSR_APTX_CHANNELS_MONO (0x01)
#define CSR_APTX_FUTURE_1 (0x00)
#define CSR_APTX_FUTURE_2 (0x00)
#define CSR_APTX_OTHER_FEATURES_NONE (0x00000000)
#define CSR_AV_APTX_AUDIO (0x00)
#define CSR_APTX_CHANNEL (0x0001)
#define CSR_APTX_SAMPLERATE (0x22)

typedef struct
{
    UINT32 vendorId;
    UINT16 codecId;            /* Codec ID for aptX */
    UINT8     sampleRate;   /* Sampling Frequency */
    UINT8   channelMode;    /* STEREO/DUAL/MONO */
    UINT8   future1;
    UINT8   future2;
} tA2D_APTX_CIE;


typedef struct
{
    SINT16 s16SamplingFreq;        /* 16k, 32k, 44.1k or 48k*/
    SINT16 s16ChannelMode;        /* mono, dual, streo or joint streo*/
    UINT16 u16BitRate;
    UINT16 *ps16NextPcmBuffer;
    UINT16 as16PcmBuffer[256];
    UINT8  *pu8Packet;
    UINT8  *pu8NextPacket;
    UINT16 u16PacketLength;
    void* aptxEncoder;
} APTX_ENC_PARAMS;

UINT8 A2D_BldAptxInfo(UINT8 media_type, tA2D_APTX_CIE *p_ie, UINT8 *p_result);
UINT8 A2D_ParsAptxInfo(tA2D_APTX_CIE *p_ie, UINT8 *p_info, BOOLEAN for_caps);
void APTX_Encoder_Init(APTX_ENC_PARAMS);
UINT8 bta_av_aptx_cfg_in_cap(UINT8 *p_cfg, tA2D_APTX_CIE *p_cap);
UINT8 bta_av_get_current_codec();
UINT8* bta_av_get_current_codecInfo();
BOOLEAN bta_av_co_audio_get_aac_aptx_config(UINT8 *p_config, UINT16 *p_minmtu, UINT8 type);



#endif  // A2D_APTX_H