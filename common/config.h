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
 @brief AVB global static configuration
 @details Contains all compile time configuration options for the entire AVB stack
*/

#ifndef _CFG_H_
#define _CFG_H_

#include "genavb/config.h"
#include "os/config.h"

#define CFG_LOG		LOG_INFO

#define common_CFG_LOG	CFG_LOG

#define AVTP_CFG_NUM_DOMAINS		4


#define CFG_AVTP_MIN_LATENCY		500000	/* minimum wakeup latency */
#define CFG_AVTP_DEFAULT_LATENCY	1000000	/* default wakeup latency */
#define CFG_AVTP_MAX_LATENCY		20000000 /* maximum wakeup latency */

#endif /* _CFG_H_ */
