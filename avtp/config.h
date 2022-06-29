/*
* Copyright 2014 Freescale Semiconductor, Inc.
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
 @brief AVTP static configuration
 @details Contains all compile time configuration options for avtp
*/

#ifndef _AVTP_CFG_H_
#define _AVTP_CFG_H_

#include "common/config.h"

#define avtp_CFG_LOG	CFG_LOG

#define CFG_AVTP_MAX_TIMERS	2	/* one per CRF stream */

#define CFG_AVTP_61883_6_MAX_CHANNELS	32
#define CFG_AVTP_AAF_PCM_MAX_CHANNELS	32
#define CFG_AVTP_AAF_PCM_MAX_SAMPLES	256  /* Matches 1 packet per interval for SR Class C at 192KHz and SR Class D at 176.4KHz */
#define CFG_AVTP_AAF_AES3_MAX_STREAMS	10
#define CFG_AVTP_AAF_AES3_MAX_FRAMES	256  /* Matches 1 packet per interval for SR Class C at 192KHz and SR Class D at 176.4KHz */

#endif /* _AVTP_CFG_H_ */
