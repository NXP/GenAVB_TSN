/*
 * Copyright 2018-2021, 2023-2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief AVB media clock recovery driver
*/
#ifndef _MEDIA_CLOCK_REC_PLL_
#define _MEDIA_CLOCK_REC_PLL_

#include "media_clock.h"
#include "pi.h"
#include "common/types.h"
#include "rational.h"
#include "imx-pll.h"
#include "gptp_dev.h"

#include "common/os/media_clock_rec_pll_common.h"

struct mclock_rec_pll_stats {
	unsigned int irq_count;
	unsigned int irq_count_fec_event;
	unsigned int fec_reloaded;
	uint32_t pll_numerator;
};

struct mclock_rec_pll {
	struct mclock_rec_pll_common c;		/* OS-agnostic fields */
	struct gptp_dev *gptp_event_dev;
	struct mclock_rec_pll_stats stats;
};

#define mclock_rec_pll_common_to_rec(common)	container_of((common), struct mclock_rec_pll, c)
#define mclock_dev_to_rec(m_dev) container_of((m_dev), struct mclock_rec_pll, c.dev)

int  mclock_rec_pll_init(struct mclock_rec_pll *rec);
void mclock_rec_pll_exit(struct mclock_rec_pll *rec);
void mclock_rec_pll_reset(struct mclock_rec_pll *rec);
int  mclock_rec_pll_stop(struct mclock_rec_pll *rec);
int  mclock_rec_pll_start(struct mclock_rec_pll *rec, struct mclock_start *start);

#endif /* _MEDIA_CLOCK_REC_PLL_ */
