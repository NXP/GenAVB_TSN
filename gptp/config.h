/*
* Copyright 2015 Freescale Semiconductor, Inc.
* Copyright 2020-2022 NXP
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
 @brief GPTP static configuration
 @details Contains all compile time configuration options for gptp
*/

#ifndef _GPTP_CFG_H_
#define _GPTP_CFG_H_

#include "common/config.h"

//#define CFG_LOG_OFFSET 1


#define gptp_CFG_LOG	CFG_LOG

#define CFG_GPTP_MAX_TIMERS_PER_DOMAIN_AND_PORT		12
#define CFG_GPTP_MAX_TIMERS_PER_CMLDS_AND_PORT		1

#define CFG_GPTP_DEFAULT_LOG_LEVEL "info"
#define CFG_GPTP_DEFAULT_LOG_MONOTONIC "disabled"


/*
 * gPTP profile selection
 */
#define CFG_GPTP_PROFILE_STANDARD	0 /* implements gptp per 802.1AS standard only */
#define CFG_GPTP_PROFILE_AUTOMOTIVE	1 /* e.g implements gptp per Avnu AutoCDCFunctionalSpec 1.1 */
#define CFG_GPTP_DEFAULT_PROFILE_NAME	"standard"

/*
 * Port configuration
 */
#define CFG_GPTP_MAX_NUM_PORT	CFG_MAX_NUM_PORT

/*
 * Default port role (used in automotive profile)
 */
#define CFG_GPTP_DEFAULT_PORT_ROLE	DISABLED_PORT



/*
 * Synchronization notification thresholds
 */
#define CFG_GPTP_SYNC_THRESH_HIGH	550 /* from master max offset in ns to claim not synchronized state */
#define CFG_GPTP_SYNC_THRESH_LOW	450 /* from master min offset in ns to claim synchronized state */


/*
 * Default grandmaster ID
 */
#define CFG_GPTP_DEFAULT_GM_ID	0xaf35d4feffc25000


/*
 * Default priority vector
 */

#define CFG_GPTP_DEFAULT_GM_CAPABLE		1

#define CFG_GPTP_DEFAULT_PRIORITY1		248
#define CFG_GPTP_DEFAULT_PRIORITY2		248
#define CFG_GPTP_DEFAULT_CLOCK_CLASS		248
#define CFG_GPTP_DEFAULT_CLOCK_ACCURACY		0xFF
#define CFG_GPTP_DEFAULT_CLOCK_VARIANCE		17258


/*
* Transmit interval values for pdelay request, sync and announce messages.
* Covers requirement from the following standard:
* - 802.1AS-2020 sections 11.5.2.2, 11.5.2.3 and 10.7.2.2
* - Avnu AutoCDSFunctionalSpecs_1.4 - Table 12 and Table 13
* - IEC 60802 -sections 5.7 and 5.9
*/
#define CFG_GPTP_DFLT_LOG_SYNC_INTERVAL	(-3)	//125ms
#define CFG_GPTP_MIN_LOG_SYNC_INTERVAL	(-5)	//31.25ms
#define CFG_GPTP_MAX_LOG_SYNC_INTERVAL	(0)	//1s

#define CFG_GPTP_DFLT_LOG_PDELAY_REQ_INTERVAL	(0)	//1s
#define CFG_GPTP_MIN_LOG_PDELAY_REQ_INTERVAL	(0)	//1s
#define CFG_GPTP_MAX_LOG_PDELAY_REQ_INTERVAL	(3)	//8s

#define CFG_GPTP_DFLT_LOG_ANNOUNCE_INTERVAL	(0)	//1s
#define CFG_GPTP_MIN_LOG_ANNOUNCE_INTERVAL	(0)	//1s
#define CFG_GPTP_MAX_LOG_ANNOUNCE_INTERVAL	(3)	//8s

#define CFG_GPTP_MIN_LOG_INTERVAL (-5)
#define CFG_GPTP_MAX_LOG_INTERVAL (22)


/*
* Neighbor propagation delay threshold
*/
#define CFG_GPTP_NEIGH_THRESH_DEFAULT		(800)
#define CFG_GPTP_NEIGH_THRESH_MIN_DEFAULT	(0)
#define CFG_GPTP_NEIGH_THRESH_MAX_DEFAULT	(10000000)


/* Reverse sync feature default values */
#define CFG_GPTP_RSYNC_INTERVAL_DEFAULT		(112)
#define CFG_GPTP_RSYNC_INTERVAL_MIN_DEFAULT	(32)
#define CFG_GPTP_RSYNC_INTERVAL_MAX_DEFAULT	(10000)

#define CFG_GPTP_RSYNC_ENABLE_DEFAULT		(0)
#define CFG_GPTP_RSYNC_ENABLE_MIN_DEFAULT	(0)
#define CFG_GPTP_RSYNC_ENABLE_MAX_DEFAULT	(1)

#define CFG_GPTP_RSYNC_LOG_DEFAULT	(0)
#define CFG_GPTP_RSYNC_LOG_MIN_DEFAULT	(0)
#define CFG_GPTP_RSYNC_LOG_MAX_DEFAULT	(1)

/* static pdelay default settings */
#define CFG_GPTP_DEFAULT_PDELAY_MODE_STRING	"static"
#define CFG_GPTP_DEFAULT_PDELAY_VALUE	(250)
#define CFG_GPTP_DEFAULT_PDELAY_VALUE_MIN	(0)
#define CFG_GPTP_DEFAULT_PDELAY_VALUE_MAX	(10000)
#define CFG_GPTP_DEFAULT_PDELAY_SENSITIVITY	(1)
#define CFG_GPTP_DEFAULT_PDELAY_SENSITIVITY_MIN	(0)
#define CFG_GPTP_DEFAULT_PDELAY_SENSITIVITY_MAX	(1000)

#define CFG_GPTP_PDELAY_MODE_STATIC	(0)
#define CFG_GPTP_PDELAY_MODE_STANDARD	(1)
#define CFG_GPTP_PDELAY_MODE_SILENT	(2)

/* statistics interval default settings */
#define CFG_GPTP_STATS_INTERVAL_DEFAULT 	(10)
#define CFG_GPTP_STATS_INTERVAL_MIN_DEFAULT (0)
#define CFG_GPTP_STATS_INTERVAL_MAX_DEFAULT (255)

#define CFG_GPTP_DEFAULT_PTP_ENABLED 	(1)
#define CFG_GPTP_DEFAULT_PTP_ENABLED_MIN (0)
#define CFG_GPTP_DEFAULT_PTP_ENABLED_MAX (1)

#define CFG_GPTP_DEFAULT_FORCE_2011_STRING	"no"

#define CFG_GPTP_DEFAULT_RX_DELAY_COMP 	(0)
#define CFG_GPTP_DEFAULT_TX_DELAY_COMP 	(0)
#define CFG_GPTP_DEFAULT_DELAY_COMP_MIN (-1000000)
#define CFG_GPTP_DEFAULT_DELAY_COMP_MAX (1000000)

/* allowed lost responses default settings as per 802.1AS-2020/2011 section 11.5.3 */
#define CFG_GPTP_DFLT_ALLOWED_LOST_RESP_2020	(9)
#define CFG_GPTP_DFLT_ALLOWED_LOST_RESP_2011	(3)
#define CFG_GPTP_MIN_ALLOWED_LOST_RESP	(1)
#define CFG_GPTP_MAX_ALLOWED_LOST_RESP	(255)

#endif /* _GPTP_CFG_H_ */
