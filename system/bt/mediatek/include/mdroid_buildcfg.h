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

#pragma once

/******************************************************************************
 * **
 * ** General Config
 * **
 * ******************************************************************************/

#if __STDC_VERSION__ < 199901L
#  ifndef FALSE
#    define FALSE 0
#  endif
#  ifndef TRUE
#    define TRUE (!FALSE)
#  endif
#else
#  include <stdbool.h>
#  ifndef FALSE
#    define FALSE  false
#  endif
#  ifndef TRUE
#    define TRUE   true
#  endif
#endif

#ifdef MTK_BT_COMMON
#define MTK_COMMON                     TRUE

// #define BT_TRACE_VERBOSE            TRUE
// #define SMP_DEBUG                   TRUE
// #define SDP_DEBUG_RAW               TRUE

/******************************************************************************
 * **
 * ** MTK BT feature option
 * **
 * ******************************************************************************/

/* MTK config in bt_stack.conf */
#ifndef MTK_STACK_CONFIG
#define MTK_STACK_CONFIG               TRUE
#endif

#if MTK_STACK_CONFIG == TRUE

#define MTK_STACK_CONFIG_VALUE_SEPARATOR ","
#define MTK_STACK_CONFIG_PARTIAL_NAME_VALUE_MAX 1024

#define MTK_STACK_CONFIG_FPATH_LEN 1024
#define MTK_STACK_CONFIG_NUM_OF_HEXLIST 5
#define MTK_STACK_CONFIG_NUM_OF_HEXITEMS 260
#define MTK_STACK_CONFIG_NUM_OF_HEXROWITEMS 16
#define MTK_STACK_CONFIG_NUM_OF_HEXCOLUMNITEMS 16

/* Role switch & sniff subrating blacklist */
#ifndef MTK_STACK_CONFIG_BL
#define MTK_STACK_CONFIG_BL            TRUE
#endif

/* fwlog hcl hexlist */
#ifndef MTK_STACK_CONFIG_LOG
#define MTK_STACK_CONFIG_LOG           TRUE
#endif

/* Enable BT log on userdebug/eng Load w/o BTLogger */
#ifdef MTK_BT_DEFAULT_OVERRIDE
#define MTK_STACK_CONFIG_DEFAULT_OVERRIDE FALSE
#endif

/* Blueangel to Bluedroid paired device database convert */
#ifndef MTK_STACK_BA2BD_CONVERT
#define MTK_STACK_BA2BD_CONVERT TRUE
#endif//MTK_STACK_BA2BD_CONVERT

#endif /* MTK_STACK_CONFIG == TRUE */

/******************************************************************************
 * **
 * ** BLE
 * **
 * ******************************************************************************/

/*For MTK solution, vendor specific extensions is supported by default*/
#ifndef BLE_VND_INCLUDED
#define BLE_VND_INCLUDED               TRUE
#endif

/*For MTK BT chip, set below CE length*/
#ifndef MTK_BLE_CONN_MIN_CE_LEN
#define MTK_BLE_CONN_MIN_CE_LEN    0x0060
#endif

#ifndef MTK_BLE_CONN_MAX_CE_LEN
#define MTK_BLE_CONN_MAX_CE_LEN    0x0140
#endif

/******************************************************************************
**
** GKI Buffer Pools
**
******************************************************************************/
/* Pool ID where to reassemble the SDU.
This Pool allows buffers to be used that are larger than
the L2CAP_MAX_MTU. */
#ifndef MTK_AVCT_BR_USER_RX_BUFFER_SIZE
#define MTK_AVCT_BR_USER_RX_BUFFER_SIZE     BT_DEFAULT_BUFFER_SIZE
#endif

/* Pool ID where to hold the SDU.
This Pool allows buffers to be used that are larger than
the L2CAP_MAX_MTU. */
#ifndef MTK_AVCT_BR_USER_TX_BUFFER_SIZE
#define MTK_AVCT_BR_USER_TX_BUFFER_SIZE     BT_DEFAULT_BUFFER_SIZE
#endif

/*
GKI Buffer Pool ID used to hold MPS segments during SDU reassembly
*/
#ifndef MTK_AVCT_BR_FCR_RX_BUFFER_SIZE
#define MTK_AVCT_BR_FCR_RX_BUFFER_SIZE      BT_DEFAULT_BUFFER_SIZE
#endif

/*
GKI Buffer Pool ID used to hold MPS segments used in (re)transmissions.
L2CAP_DEFAULT_ERM_POOL_ID is specified to use the HCI ACL data pool.
Note:  This pool needs to have enough buffers to hold two times the window size negotiated
in the tL2CAP_FCR_OPTIONS (2 * tx_win_size)  to allow for retransmissions.
The size of each buffer must be able to hold the maximum MPS segment size passed in
tL2CAP_FCR_OPTIONS plus BT_HDR (8) + HCI preamble (4) + L2CAP_MIN_OFFSET (11 - as of BT 2.1 + EDR Spec).
*/
#ifndef MTK_AVCT_BR_FCR_TX_BUFFER_SIZE
#define MTK_AVCT_BR_FCR_TX_BUFFER_SIZE      BT_DEFAULT_BUFFER_SIZE
#endif

/******************************************************************************
 * **
 * ** Advanced Audio Distribution Profile (A2DP)
 * **
 * ******************************************************************************/

/* A2DP advanced codec support */

/* support A2DP SRC AAC codec */
/* Please enable this feature by changing option value defined here*/
#ifndef MTK_A2DP_SRC_AAC_CODEC
#define MTK_A2DP_SRC_AAC_CODEC  TRUE
#endif

/* support A2DP SRC APTX codec */
/* Please enable this feature by adding feature option in ProjectConfig.mk*/
#ifdef MTK_BT_A2DP_SRC_APTX_CODEC
#define MTK_A2DP_SRC_APTX_CODEC  TRUE
#endif

/* Content protection: SCMS_T switch */
#ifndef BTA_AV_CO_CP_SCMS_T
#define BTA_AV_CO_CP_SCMS_T  TRUE
#endif

/* support a2dp audio dump debug feature */
#ifndef MTK_A2DP_PCM_DUMP
#define MTK_A2DP_PCM_DUMP TRUE
#endif

/* BTIF_MEDIA_FR_PER_TICKS_44_1_APTX: considering 1 frame as 16 aptX samples (4 bytes per sample)
  1 sample duration(us) = 4 * 1000000/44100
  13.75 frames/tick @ 20 ms tick (1 out of 4 frames sends one less) */
#define BTIF_MEDIA_FR_PER_TICKS_44_1_APTX (14)

/* Parameters relate to AAC codec */
#define MTK_A2DP_AAC_DEFAULT_BIT_RATE 200000
#define MTK_A2DP_BTIF_MEDIA_FR_PER_TICKS_AAC MTK_A2DP_BTIF_MEDIA_TIME_EXT
#define MTK_A2DP_AAC_ENC_INPUT_BUF_SIZE (4*1024)
#define MTK_A2DP_AAC_ENC_OUTPUT_BUF_SIZE (4*1024)
#define MTK_A2DP_AAC_READ_BUF_SIZE (BTIF_MEDIA_TIME_TICK * 44100 / 1000 * 2 * 2)
#define MTK_A2DP_AAC_LIMIT_MTU_SIZE (800)

/*For Low power bug fix*/
#define BTIF_MEDIA_NUM_TICK 1
#define MTK_A2DP_BTIF_MEDIA_TIME_EXT (3)
#define BTIF_MEDIA_TIME_TICK    (20 * BTIF_MEDIA_NUM_TICK * MTK_A2DP_BTIF_MEDIA_TIME_EXT)
#define MAX_PCM_FRAME_NUM_PER_TICK    14 * MTK_A2DP_BTIF_MEDIA_TIME_EXT
#define MAX_PCM_ITER_NUM_PER_TICK    3 * MTK_A2DP_BTIF_MEDIA_TIME_EXT
#define MAX_OUTPUT_A2DP_FRAME_QUEUE_SZ (MAX_PCM_FRAME_NUM_PER_TICK * 2)
#define A2DP_DATA_READ_POLL_MS    (BTIF_MEDIA_TIME_TICK / 2 / MTK_A2DP_BTIF_MEDIA_TIME_EXT)

/******************************************************************************
 * **
 * ** Audio/Video Remote Control Profile (AVRCP)
 * **
 * ******************************************************************************/
#ifdef MTK_BT_AVRCP_TG_16

#define MTK_AVRCP_TG_15 TRUE
#define MTK_AVRCP_TG_16 TRUE

#define MTK_BT_PSM_COVER_ART 0x1077
#endif


#ifdef MTK_BT_AVRCP_TG_15

/* support AVRCP 1.5 version target role
Following features are included
1. Media player selection is mandandory (if support catogary 1)
2. Get Folder Item - media player list (if support catogary 1)
3. SDP record for AVRCP 1.5 */
#ifndef MTK_AVRCP_TG_15
#define MTK_AVRCP_TG_15 TRUE
#endif

#define MTK_AVRCP_VERSION_SDP_POSITION 7
#define MTK_AVRCP_13_VERSION 0x03
#define MTK_AVRCP_14_VERSION 0x04

#endif /* ifdef MTK_BT_AVRCP_TG_15 */

/******************************************************************************
 * **
 * ** Audio/Video Control Transport Protocol (AVCTP)
 * **
 * ******************************************************************************/

#if MTK_AVRCP_TG_15 == TRUE

/* enable AOSP avct browse included */
#define AVCT_BROWSE_INCLUDED        TRUE

#define MTK_AVCT_BROWSE_INCLUDED TRUE

#endif /* ifdef MTK_BT_AVRCP_TG_15 */

/* AVCTP Browsing channel FCR Option:
Size of the transmission window when using enhanced retransmission mode. Not used
in basic and streaming modes. Range: 1 - 63
*/
#ifndef MTK_AVCT_BR_FCR_OPT_TX_WINDOW_SIZE
#define MTK_AVCT_BR_FCR_OPT_TX_WINDOW_SIZE      10
#endif

/* AVCTP Browsing channel FCR Option:
Number of transmission attempts for a single I-Frame before taking
Down the connection. Used In ERTM mode only. Value is Ignored in basic and
Streaming modes.
Range: 0, 1-0xFF
0 - infinite retransmissions
1 - single transmission
*/
#ifndef MTK_AVCT_BR_FCR_OPT_MAX_TX_B4_DISCNT
#define MTK_AVCT_BR_FCR_OPT_MAX_TX_B4_DISCNT    20
#endif

/* AVCTP Browsing channel FCR Option: Retransmission Timeout
The AVRCP specification set a value in the range of 300 - 2000 ms
Timeout (in msecs) to detect Lost I-Frames. Only used in Enhanced retransmission mode.
Range: Minimum 2000 (2 secs) when supporting PBF.
*/
#ifndef MTK_AVCT_BR_FCR_OPT_RETX_TOUT
#define MTK_AVCT_BR_FCR_OPT_RETX_TOUT           2000
#endif

/* AVCTP Browsing channel FCR Option: Monitor Timeout
The AVRCP specification set a value in the range of 300 - 2000 ms
Timeout (in msecs) to detect Lost S-Frames. Only used in Enhanced retransmission mode.
Range: Minimum 12000 (12 secs) when supporting PBF.
*/
#ifndef MTK_AVCT_BR_FCR_OPT_MONITOR_TOUT
#define MTK_AVCT_BR_FCR_OPT_MONITOR_TOUT        12000
#endif

/* AVCTP browsing channel FCR Option: Maximum PDU payload size.
The maximum number of payload octets that the local device can receive in a single PDU.
*/
#ifndef MTK_AVCT_BR_FCR_OPT_MPS_SIZE
#define MTK_AVCT_BR_FCR_OPT_MPS_SIZE            1000
#endif

/******************************************************************************
 * **
 * ** HANDS FREE PROFILE (HFP)
 * **
 * ******************************************************************************/

#ifdef MTK_BT_HFP_DUAL_HF
/*Dual HandsFree feature*/
#define MTK_HFP_DUAL_HF      TRUE
#endif /* #ifdef MTK_BT_HFP_DUAL_HF */

/* Wide Band Speech */
#ifndef MTK_HFP_WBS
#define MTK_HFP_WBS                 TRUE
#endif

/* In-band Ringtone */
#ifndef MTK_HFP_INBAND_RING
#define MTK_HFP_INBAND_RING         FALSE
#endif

#if MTK_HFP_WBS == TRUE
#ifndef BTM_WBS_INCLUDED
#define BTM_WBS_INCLUDED            TRUE
#endif

#ifndef BTIF_HF_WBS_PREFERRED
#define BTIF_HF_WBS_PREFERRED       TRUE
#endif
#endif /* if MTK_HFP_WBS == TRUE */

/******************************************************************************
 * **
 * ** dial-up networking (DUN)
 * **
 * ******************************************************************************/

#ifdef MTK_BT_DUN_GW_12
#define MTK_DUN_GW_12               TRUE
#endif

/******************************************************************************
 * **
 * ** Detect Chip Reset
 * **
 * ******************************************************************************/

/* Support handle trigger FW core dump and monitor chip-reset in hci layer.*/
/* Must be TRUE */
#define MTK_SUPPORT_FW_CORE_DUMP  TRUE

/******************************************************************************
 * **
 * ** New MTK Vendor Opcodes
 * **
 * ******************************************************************************/

/* Enable MTK-owned vendor opcode. */
/* Must be TRUE */
#define MTK_VENDOR_OPCODE      TRUE

/******************************************************************************
* **
 * ** MTK Power Saving Mode Ctrl by HCI Interface
* **
******************************************************************************/
/* Support PSM Mode in HCI Interface. */
#define MTK_HCI_POWERSAVING_MODE TRUE

/******************************************************************************
 * **
 * ** BT Snoop Log
 * **
 * ******************************************************************************/

/* Create a thread for accessing file i/o of btsnoop log. */
#ifndef MTK_BTSNOOPLOG_THREAD
#define MTK_BTSNOOPLOG_THREAD      TRUE
#endif

/* Default enable customize snnop log mode*/
#ifndef MTK_BTSNOOPLOG_MODE_SUPPRT
#define MTK_BTSNOOPLOG_MODE_SUPPRT TRUE
#endif

/******************************************************************************
**
** ATT/GATT Protocol/Profile Settings
**
******************************************************************************/
#ifndef BTA_DM_GATT_CLOSE_DELAY_TOUT
#define BTA_DM_GATT_CLOSE_DELAY_TOUT    1500
#endif

/******************************************************************************
**
** HID/HOGP Profile Settings
**
******************************************************************************/
#ifndef MTK_HID_IOT_MOUSE_IME
#define MTK_HID_IOT_MOUSE_IME    TRUE
#endif

#ifdef MTK_BT_KERNEL_3_18
#define MTK_HID_DRIVER_KERNEL_3_18    TRUE
#endif

#if MTK_COMMON == TRUE
#ifdef MTK_BT_KERNEL_4_4
#define MTK_HID_DRIVER_KERNEL_4_4    TRUE
#endif
#endif

/******************************************************************************
 * **
 * **
 * **
 * ******************************************************************************/

#endif /* ifdef MTK_BT_COMMON */
