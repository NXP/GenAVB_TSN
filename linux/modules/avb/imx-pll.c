/*
 * IMX Clock pll common functions

 * Copyright 2018 NXP.
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

#include "imx-pll.h"
#include <linux/io.h>
#include <linux/clk-provider.h>

/**
 * imx_pll_init() - Initializes the imx_pll struct through the driver device node
 * @p - pointer to imx_pll structure
 * @dev - pointer to the timer device struct
 *
 * Returns 0 on success or negative error.
 */

int imx_pll_init(struct imx_pll *pll, struct device *dev)
{
	int rc = 0;

	pll->timer_dev = dev;

	pll->clk_audio_pll = devm_clk_get(dev, "audio_pll"); //audio pll
	if (IS_ERR(pll->clk_audio_pll)) {
		rc = PTR_ERR(pll->clk_audio_pll);
		goto err;
	}

	clk_prepare_enable(pll->clk_audio_pll);

	pll->clk_pll = clk_imx_pll_get_by_name(__clk_get_name(pll->clk_audio_pll));
	if (!pll->clk_pll) {
		rc = -EINVAL;
		pr_err("%s: Can not get the imx pll for %s \n", __func__, __clk_get_name(pll->clk_audio_pll));
		goto err_clk;
	}

	pll->parent_rate = clk_get_rate(clk_get_parent(pll->clk_audio_pll));
	if (!pll->parent_rate) {
		rc = -EINVAL;
		pr_err("%s: Can not get the audio pll parent's rate\n", __func__);
		goto err_clk;
	}

	pll->current_rate = clk_imx_pll_get_rate(pll->clk_pll, pll->parent_rate);
	if (!pll->current_rate) {
		rc = -EINVAL;
		pr_err("%s: Can not get the audio pll rate \n", __func__);
		goto err_clk;
	}

	return 0;
err_clk:
	clk_disable_unprepare(pll->clk_audio_pll);
err:
	return rc;
}

int imx_pll_adjust(struct imx_pll *pll, int *ppb)
{
	return clk_imx_pll_adjust(pll->clk_pll, ppb);
}

unsigned long imx_pll_get_rate(struct imx_pll *pll)
{
	return clk_imx_pll_get_rate(pll->clk_pll, pll->parent_rate);
}

void imx_pll_deinit(struct imx_pll *pll)
{
	int rc;
	int ppb = 0;

	/* Reset pll adjustment*/
	rc = clk_imx_pll_adjust(pll->clk_pll, &ppb);
	if (rc < 0)
		pr_err("%s: Can not reset audio pll adjustment \n", __func__);

	clk_disable_unprepare(pll->clk_audio_pll);
}
