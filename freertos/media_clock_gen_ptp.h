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
 @brief AVB media clock generation driver
*/

#ifndef _MEDIA_CLOCK_GEN_PTP_
#define _MEDIA_CLOCK_GEN_PTP_

#include "media_clock.h"

#define MCLOCK_DOMAIN_PTP_RANGE		1

struct mclock_gen_ptp_stats {
	unsigned int reset;
	unsigned int start;
	unsigned int stop;
	unsigned int ts_period;
	unsigned int ptp_now;
	unsigned int err_drift;
	unsigned int err_jump;
};

struct mclock_gen_ptp {
	struct mclock_dev dev;
	unsigned int clk_ptp;
	uint32_t ts_next;
	uint32_t ts_period;
	uint32_t ts_period_rem;
	uint32_t ts_corr;
	uint32_t ts_period_n;
	struct mclock_gen_ptp_stats stats;
};

int mclock_gen_ptp_init(struct mclock_gen_ptp *ptp);
void mclock_gen_ptp_exit(struct mclock_gen_ptp *ptp);


#define MCLOCK_GEN_NUM_TS		256	// Needs to be power of 2.
#define MCLOCK_GEN_BUF_SIZE		(MCLOCK_GEN_NUM_TS * sizeof(unsigned int))
#define MCLOCK_GEN_MMAP_SIZE		(MCLOCK_GEN_BUF_SIZE + 3 * sizeof(unsigned int)) // w_idx, ptp and count
#define MCLOCK_GEN_TS_FREQ_INIT		300

#endif /* _MEDIA_CLOCK_GEN_PTP_ */

