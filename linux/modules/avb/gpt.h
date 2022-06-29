/*
 * AVB GPT driver
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
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

#ifndef _GPT_H_
#define _GPT_H_

#ifdef __KERNEL__

#include <linux/time.h>
#include "hw_timer.h"
#include "media_clock_rec_pll.h"


/* GPT registers offsets */
#define GPT_CR			0x00
#define GPT_PR			0x04
#define GPT_SR			0x08
#define GPT_IR			0x0C
#define GPT_OCR(ch)		(0x10 + (((ch) - 1) * 4))
#define GPT_ICR(ch)		(0x1C + (((ch) - 1) * 4))
#define GPT_CNT			0x24

/* GPT_CR */
#define GPT_EN			BIT(0)
#define GPT_WAITEN		BIT(3)
#define GPT_CLKSRC(val)		(((val) & 0x7) << 6)
#define GPT_FRR			BIT(9)
#define GPT_SWR			BIT(15)
#define GPT_IM(ch, val)		(((val) & 0x3) << (16 + 2 * ((ch) - 1)))
#define GPT_IM_DIS		0x0
#define GPT_IM_RISING		0x1
#define GPT_IM_FALLING		0x2
#define GPT_IM_BOTH		0x3

/*GPT clock source types*/
#define GPT_CLKSRC_NOCLK		0x0	/* No clock */
#define GPT_CLKSRC_IPG_CLK		0x1	/* Peripheral clock */
#define GPT_CLKSRC_IPG_CLK_HIGH_FREQ 	0x2	/* High frequency reference clock */
#define GPT_CLKSRC_EXT_CLK		0x3	/* External clock */
#define GPT_CLKSRC_IPG_CLK_32K		0x4	/* Low frequency reference clock */
#define GPT_CLKSRC_IPG_CLK_24M		0x5	/* Crystal Oscillator as reference clock */

/* GPT_PR */
#define GPT_PRESCALER(val)	((val) & 0xFF)

/* GPT_SR */
#define GPT_OF(ch)		BIT((ch) - 1)
#define GPT_IF(ch)		BIT((ch) + 2)
#define GPT_ROV			BIT(5)

/* GPT_IR */
#define GPT_OFIE(ch)		BIT((ch) - 1)
#define GPT_IFIE(ch)		BIT((ch) + 2)
#define GPT_ROVIE		BIT(5)

#define GPT_REC_TIMER_PERIOD_MS 1
#define GPT_REC_TIMER_PERIOD_NS	(GPT_REC_TIMER_PERIOD_MS * NSEC_PER_MSEC)

#define GPT_REC_FACTOR		16
#define GPT_REC_P_FACTOR	1
#define GPT_REC_I_FACTOR	3
#define GPT_REC_MAX_ADJUST	100
#define GPT_REC_NB_START	10

#define GPT_HW_TIMER_MODE	(1 << 0)
#define GPT_MCLOCK_REC_MODE	(1 << 1)

#define GPT_MAX_CAPTURE_CH_ID	2
#define GPT_MIN_CAPTURE_CH_ID	1

#define GPT_MAX_OUTPUT_CH_ID	3
#define GPT_MIN_OUTPUT_CH_ID	1

struct gpt_drv {
	void *baseaddr;
	int irq;

	resource_size_t start;
	resource_size_t size;

	struct clk *clk_gpt;
	struct clk *clk_ipg;
	unsigned int clk_src_type;

	int prescale;

	int func_mode;
	struct hw_timer_dev timer_dev;
	u32 next_cycles;

	struct mclock_rec_pll rec;
	unsigned int cap_channel;
	unsigned int timer_channel;
	unsigned int eth_port;
	u32 cnt_last;
};

bool is_gpt_hw_timer_available(void);
int gpt_init(struct platform_driver *);
void gpt_exit(struct platform_driver *);

#endif /* __KERNEL__ */

#endif /* _GPT_H_ */
