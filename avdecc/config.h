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
 @brief AVDECC static configuration
 @details Contains all compile time configuration options for avdecc
*/

#ifndef _AVDECC_CFG_H_
#define _AVDECC_CFG_H_

#include "common/config.h"
#include "common/adp.h"

#define avdecc_CFG_LOG	CFG_LOG

#define CFG_AVDECC_MAX_TIMERS_PER_ENTITY	3

#define AVDECC_CFG_INFLIGHT_TIMER_RESOLUTION	10

#define AECP_CFG_MAX_AEM_IN_PROGRESS		(10000 / AECP_IN_PROGRESS_TIMEOUT) /* 10000 ms : Maximum IN_PROGRESS responses for AECP CMD before declaring the application unresponsive*/

#define CFG_AECP_DEFAULT_NUM_UNSOLICITED		8
#define CFG_AECP_MAX_NUM_UNSOLICITED			64
#define CFG_AECP_MIN_NUM_UNSOLICITED			1

#define CFG_ADP_DEFAULT_NUM_ENTITIES_DISCOVERY		16
#define CFG_ADP_MIN_NUM_ENTITIES_DISCOVERY		8
#define CFG_ADP_MAX_NUM_ENTITIES_DISCOVERY		128


#define CFG_AVDECC_DEFAULT_NUM_INFLIGHTS		5
#define CFG_AVDECC_MAX_NUM_INFLIGHTS			(CFG_ADP_MAX_NUM_ENTITIES_DISCOVERY)
#define CFG_AVDECC_MIN_NUM_INFLIGHTS			5

#define CFG_ADP_DEFAULT_VALID_TIME	62 //seconds
#define CFG_ADP_MIN_VALID_TIME		2 //seconds
#define CFG_ADP_MAX_VALID_TIME		62 //seconds

#define CFG_ACMP_DEFAULT_NUM_TALKER_STREAMS	8
#define CFG_ACMP_MIN_NUM_TALKER_STREAMS		1
#define CFG_ACMP_MAX_NUM_TALKER_STREAMS		32

#define CFG_ACMP_DEFAULT_NUM_LISTENER_STREAMS	8
#define CFG_ACMP_MIN_NUM_LISTENER_STREAMS	1
#define CFG_ACMP_MAX_NUM_LISTENER_STREAMS	32

#define CFG_ACMP_DEFAULT_NUM_LISTENER_PAIRS	10
#define CFG_ACMP_MIN_NUM_LISTENER_PAIRS		1
#define CFG_ACMP_MAX_NUM_LISTENER_PAIRS		512

#endif /* _AVDECC_CFG_H_ */
