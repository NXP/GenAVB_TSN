/*
 * AVB FlexTimer driver
 * Copyright 2014-2015 Freescale Semiconductor, Inc.
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

#ifndef _FTM_H_
#define _FTM_H_

#ifdef __KERNEL__

#include <linux/time.h>
#include "hw_timer.h"
#include "media_clock.h"
#include "media_clock_rec_pll.h"


#define FTM_SC			0x00
#define FTM_SC_CLK_SHIFT	3
#define FTM_SC_CLK_MASK		(0x3 << FTM_SC_CLK_SHIFT)
#define FTM_SC_CLK(c)		((c) << FTM_SC_CLK_SHIFT)
#define FTM_SC_CLK_OFF		0
#define FTM_SC_CLK_SYS		1
#define FTM_SC_CLK_FIXED	2
#define FTM_SC_CLK_EXT		3
#define FTM_SC_PS_MASK		0x7
#define FTM_SC_TOIE		BIT(6)
#define FTM_SC_TOF		BIT(7)

#define FTM_CNT			0x04

#define FTM_MOD			0x08

#define FTM_CSC(n)		(0xC + 8 * n)
#define FTM_CSC_DMA		BIT(0)
#define FTM_CSC_ICRST		BIT(1)
#define FTM_CSC_ELSA		BIT(2)
#define FTM_CSC_ELSB		BIT(3)
#define FTM_CSC_MSA		BIT(4)
#define FTM_CSC_MSB		BIT(5)
#define FTM_CSC_CHIE		BIT(6)
#define FTM_CSC_CHF		BIT(7)

#define FTM_CV(n)		(0x10 + 8 * n)

#define FTM_CNTIN		0x4C

#define FTM_STATUS		0x50
#define FTM_STATUS_CHF(n)	BIT(n)

#define FTM_MODE		0x54
#define FTM_COMBINE		0x64


#define FTM_PS_MAX		7

#define FTM_REC_TIMER_PERIOD_MS 1
#define FTM_REC_TIMER_PERIOD_NS	(FTM_REC_TIMER_PERIOD_MS * NSEC_PER_MSEC)

#define FTM_REC_P_FACTOR	1
#define FTM_REC_I_FACTOR	1
#define FTM_REC_FACTOR		4

#define FTM_REC_MAX_ADJUST	100
#define FTM_REC_NB_START	10

struct ftm {
	void *baseaddr;
	int irq;

	resource_size_t start;
	resource_size_t size;

	struct clk *clk_ftm;
	struct clk *clk_ipg;

	struct hw_timer_dev timer_dev;
	int timer_channel;
	int prescale;
	u16 timer_next_val;

	struct mclock_rec_pll rec;
	unsigned int rec_channel;
	unsigned int eth_port;
	u16 cnt_last;
};

static inline int ftm_prescale_to_regval(int ps)
{
	int i;
	for (i = 0; (i < (sizeof(ps) * 8)) && !((1 << i) & ps); i++)
	;
	return i;
}

int ftm_init(struct platform_driver *);
void ftm_exit(struct platform_driver *);

#endif /* __KERNEL__ */

#endif /* _FTM_H_ */
