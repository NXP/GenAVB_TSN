/*
* Copyright 2019-2020 NXP
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
 @brief GPTP stack component entry points
 @details Handles all GPTP
*/


#ifndef _GPTP_ENTRY_H_
#define _GPTP_ENTRY_H_

#include "os/sys_types.h"
#include "genavb/init.h"
#include "common/ptp.h"

/**
 * Sync info states
 */
typedef enum {
	SYNC_STATE_UNDEFINED = 0,
	SYNC_STATE_NOT_SYNCHRONIZED,
	SYNC_STATE_SYNCHRONIZED,
} ptp_port_sync_state_t;

#define PTP_SYNC_STATE(s) ((s == SYNC_STATE_SYNCHRONIZED)? "SYNCHRONIZED" :"NOT SYNCHRONIZED")

/**
 * Sync info message definition
 */
struct gptp_sync_info {
	u16 port_id;
	ptp_port_sync_state_t state;
	u64 sync_time_ms;
	u8 domain;
};

/**
 * Pdelay message definition
 */
struct gptp_pdelay_info {
	u16 port_id;
	ptp_double pdelay;
};

/**
 * GM Id message definition
 */
struct gptp_gm_info {
	struct ptp_priority_vector vector;
	u8 is_grandmaster;
	u8 domain;
};

void *gptp_init(struct fgptp_config *cfg, unsigned long priv);
int gptp_exit(void *gptp_ctx);
void gptp_stats_dump(void *gptp_ctx);

#endif /* _GPTP_ENTRY_H_ */
