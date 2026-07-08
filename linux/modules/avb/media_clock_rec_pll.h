/*
 * Copyright 2014-2015 Freescale Semiconductor, Inc.
 * Copyright 2018-2020, 2022-2025 NXP
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef _MEDIA_CLOCK_REC_PLL_
#define _MEDIA_CLOCK_REC_PLL_

#include "media_clock.h"
#include "pi.h"
#include "imx-pll.h"

#include "media_clock_rec_pll_common.h"

struct mclock_rec_pll_stats {
	unsigned int err_start_port_down;
	unsigned int err_stop_port_down;
};

struct mclock_rec_pll {
	struct mclock_rec_pll_common c;		/* OS-agnostic fields */
	unsigned int fec_tc_id;
	unsigned int fec_nb_meas;
	int req_ki_factor;
	int req_kp_factor;
	int next_ppb;
	struct task_struct *mcr_kthread;
	struct mclock_rec_pll_stats stats;
};

#define mclock_rec_pll_common_to_rec(common)	container_of((common), struct mclock_rec_pll, c)
#define mclock_dev_to_rec(m_dev) container_of((m_dev), struct mclock_rec_pll, c.dev)

int  mclock_rec_pll_init(struct mclock_rec_pll *rec, unsigned int eth_port);
void mclock_rec_pll_exit(struct mclock_rec_pll *rec);

void mclock_rec_pll_reset(struct mclock_rec_pll *rec);
int  mclock_rec_pll_ready(struct mclock_rec_pll *rec);
int  mclock_rec_pll_stop(struct mclock_rec_pll *rec);
int  mclock_rec_pll_start(struct mclock_rec_pll *rec, struct mclock_start *start);

#endif /* _MEDIA_CLOCK_REC_PLL_ */
