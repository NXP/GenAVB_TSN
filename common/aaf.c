/*
* Copyright 2016 Freescale Semiconductor, Inc.
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
 @brief Common AAF functions
 @details
*/

#include "common/aaf.h"

const avb_u32 avtp_aaf_sampling_freq[AAF_NSR_MAX + 1] = {
	[AAF_NSR_USER_SPECIFIED] = 48000, /* FIXME */
	[AAF_NSR_8000] = 8000,
	[AAF_NSR_16000] = 16000,
	[AAF_NSR_32000] = 32000,
	[AAF_NSR_44100] = 44100,
	[AAF_NSR_48000] = 48000,
	[AAF_NSR_88200] = 88200,
	[AAF_NSR_96000] = 96000,
	[AAF_NSR_176400] = 176400,
	[AAF_NSR_192000] = 192000,
	[AAF_NSR_24000] = 24000,
	[AAF_NSR_RESERVED1] = 0,
	[AAF_NSR_RESERVED2] = 0,
	[AAF_NSR_RESERVED3] = 0,
	[AAF_NSR_RESERVED4] = 0,
	[AAF_NSR_RESERVED5] = 0
};
