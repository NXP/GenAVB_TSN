/*
* Copyright 2015 Freescale Semiconductor, Inc.
* Copyright 2020 NXP
*
* NXP Confidential. This software is owned or controlled by NXP and may only
* be used strictly in accordance with the applicable license terms.  By expressly
* accepting such terms or by downloading, installing, activating and/or otherwise
* using the software, you are agreeing that you have read, and that you agree to
* comply with and are bound by, such license terms.  If you do not agree to be
* bound by the applicable license terms, then you may not retain, install, activate
* or otherwise use the software.
*/

/**
 @file
 @brief Common 61883 IIDC functions
 @details
*/

#include "common/61883_iidc.h"

const avb_u32 avtp_61883_6_sampling_freq[IEC_61883_6_FDF_SFC_MAX + 1] = {
	[IEC_61883_6_FDF_SFC_32000] = 32000,
	[IEC_61883_6_FDF_SFC_44100] = 44100,
	[IEC_61883_6_FDF_SFC_48000] = 48000,
	[IEC_61883_6_FDF_SFC_88200] = 88200,
	[IEC_61883_6_FDF_SFC_96000] = 96000,
	[IEC_61883_6_FDF_SFC_176400] = 176400,
	[IEC_61883_6_FDF_SFC_192000] = 192000,
	[IEC_61883_6_FDF_SFC_RSVD] = 0,
};
