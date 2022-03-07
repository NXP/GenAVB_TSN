/*
 * AVB media clock ptp generation
 * Copyright 2014-2015 Freescale Semiconductor, Inc.
 * Copyright 2018 NXP
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef _MEDIA_CLOCK_GEN_PTP_
#define _MEDIA_CLOCK_GEN_PTP_

#include "media_clock.h"

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
	u32 ts_next;
	u32 ts_period;
	u32 ts_period_rem;
	u32 ts_corr;
	u32 ts_period_n;
	struct mclock_gen_ptp_stats stats;
	struct dentry *dentry;
};

int mclock_gen_ptp_init(struct mclock_gen_ptp *ptp);
void mclock_gen_ptp_exit(struct mclock_gen_ptp *ptp);

#define MCLOCK_GEN_NUM_TS		256	//Needs to be power of 2.
#define MCLOCK_GEN_BUF_SIZE		(MCLOCK_GEN_NUM_TS * sizeof(unsigned int))
#define MCLOCK_GEN_MMAP_SIZE		(MCLOCK_GEN_BUF_SIZE + 3 * sizeof(unsigned int)) //w_idx, ptp and count
#define MCLOCK_GEN_DMA_SIZE		(MCLOCK_GEN_MMAP_SIZE + sizeof(unsigned int)) //One more to hold tmode value
#define MCLOCK_GEN_DMA_TS_FREQ_INIT	600
#define MCLOCK_GEN_TS_FREQ_INIT		300

#endif /* _MEDIA_CLOCK_GEN_PTP_ */

