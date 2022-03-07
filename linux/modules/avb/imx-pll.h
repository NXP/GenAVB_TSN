/*
 * IMX Clock PLL

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

#ifndef _IMX_CLK_PLL_H_
#define _IMX_CLK_PLL_H_

#include <linux/clk.h>
#include <linux/device.h>
#include <linux/of.h>
#include <linux/clk/imx-pll.h>

/* The PI controller was tuned (proportional gain and integral gain) to work for a control
 * output as a numerator variation in the PLL output equation.
 * Moving to a control output of a ppb varitaion we need to adjust these coefficients:
 * 1 numerator variation was giving a 24Hz variation in the PLL output frequency (786432000 Hz) which
 * is around 30 ppb*/
#define IMX_PLL_ADJUST_FACTOR	30

struct imx_pll {
	unsigned long parent_rate; /*OSC Reference clock rate*/
	unsigned long current_rate;
	struct clk *clk_audio_pll;
	struct device *timer_dev;
	struct clk_imx_pll *clk_pll;
};

int imx_pll_init(struct imx_pll *pll, struct device *dev);
int imx_pll_adjust(struct imx_pll *pll, int *ppb);
unsigned long imx_pll_get_rate(struct imx_pll *pll);
void imx_pll_deinit(struct imx_pll *pll);

#endif /* _IMX_CLK_PLL_H_ */
