/*
 * AVB GPT driver
 * Copyright 2018 - 2021 NXP
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

#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/fec.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/regmap.h>
#include <linux/mfd/syscon.h>

#include "gpt.h"
#include "imx-pll.h"
#include "net_port.h"
#include "avbdrv.h"


/**
 * DOC: GPT driver which handles a PLL based recovery.
 *
 */

int gpt_rec_timer_irq(struct mclock_dev *dev, void *data, unsigned int ticks)
{
	struct mclock_rec_pll *rec = container_of(dev, struct mclock_rec_pll, dev);
	struct gpt_drv *drv = container_of(rec, struct gpt_drv, rec);
	unsigned long flags;
	u32 icr_val, cnt_val;

	raw_spin_lock_irqsave(&dev->eth->lock, flags);

	/* get status  */
	icr_val = readl_relaxed(drv->baseaddr + GPT_SR);
	if (icr_val & GPT_IF(drv->cap_channel)) {

		/* get value */
		cnt_val = readl_relaxed(drv->baseaddr + GPT_ICR(drv->cap_channel));

		mclock_rec_pll_timer_irq(rec, 1, (u32)(cnt_val - drv->cnt_last), ticks);
		drv->cnt_last = cnt_val;

		/* clear status bit
		 * Do this _after_ mclock_rec_pll_timer_irq because internal resets
		 * can cause FEC ENET compare output signal toggling.
		 */
		writel_relaxed(icr_val & GPT_IF(drv->cap_channel), drv->baseaddr + GPT_SR);
	}
	else
		mclock_rec_pll_timer_irq(rec, 0, 0, ticks);

	raw_spin_unlock_irqrestore(&dev->eth->lock, flags);

	return 0;
}

int gpt_rec_start(struct mclock_dev *dev, struct mclock_start *start)
{
	struct mclock_rec_pll *rec = container_of(dev, struct mclock_rec_pll, dev);
	int rc = 0;

	/* Start ENET */
	mclock_rec_pll_start(rec, start);

	rc = mclock_drv_register_timer(dev, gpt_rec_timer_irq, 1);
	if (rc < 0) {
		mclock_rec_pll_stop(rec);
		pr_err("%s : cannot register timer\n", __func__);
		goto out;
	}

out:
	return rc;
}

static void gpt_hw_timer_start(struct hw_timer_dev *dev)
{
	struct gpt_drv *gpt = platform_get_drvdata(dev->pdev);
	u32 val, cycles_now;

	/* Enable Output Compare Channel interrupt*/
	val = readl_relaxed(gpt->baseaddr + GPT_IR);
	writel_relaxed(val | GPT_OFIE(gpt->timer_channel), gpt->baseaddr + GPT_IR);

	cycles_now = readl_relaxed(gpt->baseaddr + GPT_CNT);

	gpt->next_cycles = cycles_now + (u32) dev->period;
	writel_relaxed(gpt->next_cycles, gpt->baseaddr + GPT_OCR(gpt->timer_channel));

}

int gpt_rec_stop(struct mclock_dev *dev)
{
	struct mclock_rec_pll *rec = container_of(dev, struct mclock_rec_pll, dev);
	int rc = 0;

	mclock_drv_unregister_timer(dev);

	rc = mclock_rec_pll_stop(rec);
	if (rc < 0)
		pr_err("%s : cannot stop enet\n", __func__);

	return rc;
}

static void gpt_hw_timer_stop(struct hw_timer_dev *dev)
{
	struct gpt_drv *drv = platform_get_drvdata(dev->pdev);
	u32 val;

	val = readl_relaxed(drv->baseaddr + GPT_IR);
	writel_relaxed(val & ~GPT_OFIE(drv->timer_channel), drv->baseaddr + GPT_IR);

	writel_relaxed(GPT_OF(drv->timer_channel), drv->baseaddr + GPT_SR);
}


static void gpt_hw_timer_set_period(struct hw_timer_dev *dev, unsigned long period_us)
{
	struct gpt_drv *gpt = platform_get_drvdata(dev->pdev);

	dev->period = (period_us * (dev->rate / 1000)) / 1000;

	if (((dev->period * 1000) / (dev->rate / 1000)) != period_us)
		pr_warn("%s: dev(%p) warning the HW timer period is incorrect",
			__func__, dev);

	pr_info("gpt_hw (%p) set period %lu(us), %u(cycles)\n", gpt, period_us, dev->period);
}

static unsigned int gpt_hw_timer_elapsed(struct hw_timer_dev *dev, unsigned int cycles)
{
	struct gpt_drv *drv = platform_get_drvdata(dev->pdev);
	u32 cycles_now;

	cycles_now = readl_relaxed(drv->baseaddr + GPT_CNT);

	return cycles_now - (u32)cycles;
}

static unsigned int gpt_hw_timer_irq_ack(struct hw_timer_dev *dev, unsigned int *cycles, unsigned int *dcycles)
{
	struct gpt_drv *drv = platform_get_drvdata(dev->pdev);
	unsigned int ticks = 0;
	u32 cycles_now, num_recovery = 0;

	cycles_now = readl_relaxed(drv->baseaddr + GPT_CNT);

	*cycles = cycles_now;

try_again:
	writel_relaxed(GPT_OF(drv->timer_channel), drv->baseaddr + GPT_SR);

	*dcycles = cycles_now - drv->next_cycles;

	/* Check how many periods have elapsed since last time
	 * Normally a single period has elapsed, but we may be late */
	do {
		drv->next_cycles += (u32)dev->period;
		ticks++;
	} while ((int)(drv->next_cycles - cycles_now) < 0);

	/* Set next timer value */
	writel_relaxed(drv->next_cycles, drv->baseaddr + GPT_OCR(drv->timer_channel));

	/* Check if the free running counter has already passed
	the newly programmed target value */
	cycles_now = readl_relaxed(drv->baseaddr + GPT_CNT);

	if ((int)(cycles_now - drv->next_cycles) > 0) {
		num_recovery++;

		/* hard protection against potential dead loop (> 2*125us) */
		if (num_recovery > 2)
			pr_err("drv(%p) - recovery failure\n", drv);
		else
			/* try to set again the OCR below CNT */
			goto try_again;
	}

	dev->recovery_errors += num_recovery;

	return ticks;
}

int gpt_rec_reset(struct mclock_dev *dev)
{
	return 0;
}

int gpt_rec_open(struct mclock_dev *dev, int port)
{
	struct mclock_rec_pll *rec = container_of(dev, struct mclock_rec_pll, dev);
	struct gpt_drv *drv = container_of(rec, struct gpt_drv, rec);
	struct avb_drv *avb = container_of(dev->drv, struct avb_drv, mclock_drv);

	dev->eth = &avb->eth[drv->eth_port];
	if (!dev->eth)
		return -ENODEV;

	return 0;
}

static void gpt_reg_init(struct gpt_drv *drv)
{
	u32 gpt_cr;

	/* Reset GPT */
	writel_relaxed(GPT_SWR, drv->baseaddr + GPT_CR);

	/* Setup GPT config */
	gpt_cr = GPT_CLKSRC(drv->clk_src_type);

	if (drv->func_mode & GPT_MCLOCK_REC_MODE)
		gpt_cr |= GPT_IM(drv->cap_channel, GPT_IM_BOTH);

	gpt_cr |= (GPT_FRR | GPT_WAITEN | GPT_EN);

	writel_relaxed(gpt_cr, drv->baseaddr + GPT_CR);

	writel_relaxed(GPT_PRESCALER(drv->prescale - 1), drv->baseaddr + GPT_PR);

	if (drv->func_mode & GPT_HW_TIMER_MODE)
		writel_relaxed(GPT_OF(drv->timer_channel), drv->baseaddr + GPT_SR);
}

static int gpt_rec_pin_config(struct device_node *np)
{

	struct device_node *gpr_np;
	struct regmap *gpr;
	u8 gpr_reg;
	u8 gpt1_capin_sel_bit;
	u32 out_val[3];
	int rc = 0;

	if (of_machine_is_compatible("fsl,imx8mp-evk")) {
		gpr_np = of_parse_phandle(np, "gpt1_capin1_sel", 0);
		if (!gpr_np) {
			rc = -EINVAL;
			pr_err("%s: cannot get gpt1 capture dt handle\n", __func__);
			goto out;
		}

		rc = of_property_read_u32_array(np, "gpt1_capin1_sel", out_val,
						 ARRAY_SIZE(out_val));
		if (rc) {
			pr_err("%s: no gpt1 capture input select property\n", __func__);
			rc = -EINVAL;
			of_node_put(gpr_np);
			goto out;
		}

		gpr = syscon_node_to_regmap(gpr_np);
		if (IS_ERR(gpr)) {
			pr_err("%s: could not find gpr regmap\n", __func__);
			rc = -EINVAL;
			of_node_put(gpr_np);
			goto out;
		}

		of_node_put(gpr_np);

		gpr_reg = out_val[1];
		gpt1_capin_sel_bit = out_val[2];

		regmap_update_bits(gpr, gpr_reg, BIT(gpt1_capin_sel_bit), BIT(gpt1_capin_sel_bit));
	}

out:
	return rc;
}

static int gpt_probe(struct platform_device *pdev)
{
	struct gpt_drv *drv;
	struct resource *iores;
	struct mclock_rec_pll *rec = NULL;
	struct mclock_dev *clock_dev = NULL;
	struct hw_timer_dev *timer_dev = NULL;
	struct imx_pll *pll = NULL;
	struct clk *external_clk = NULL;
	int rc = 0;

	drv = devm_kzalloc(&pdev->dev, sizeof(*drv), GFP_KERNEL);
	if (!drv) {
		rc = -ENOMEM;
		goto err;
	}

	if (of_find_property(pdev->dev.of_node, "rec-channel", NULL))  {
		rec = &drv->rec;
		clock_dev = &rec->dev;
		pll = &rec->pll;
		drv->func_mode |= GPT_MCLOCK_REC_MODE;
	}

	if (of_find_property(pdev->dev.of_node, "timer-channel", NULL))  {
		timer_dev = &drv->timer_dev;
		drv->func_mode |= GPT_HW_TIMER_MODE;
	}

	if (!(timer_dev || clock_dev)) {
		rc = -EINVAL;
		pr_err("%s: missing properties in device tree node\n", __func__);
		goto err;
	}


	iores = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!iores) {
		rc = -ENODEV;
		goto err;
	}

	drv->start = iores->start;
	drv->size = resource_size(iores);

	drv->irq = platform_get_irq(pdev, 0);
	if (drv->irq < 0) {
		rc = -ENODEV;
		goto err;
	}

	if (!devm_request_mem_region(&pdev->dev, drv->start, drv->size, pdev->name)) {
		rc = -EBUSY;
		goto err;
	}

	drv->baseaddr = devm_ioremap(&pdev->dev, drv->start, drv->size);
	if (!drv->baseaddr) {
		rc = -ENOMEM;
		goto err;
	}

	/* By default choose the peripheral clock as counter clock*/
	drv->clk_gpt = devm_clk_get(&pdev->dev, "per"); //counter clock
	if (IS_ERR(drv->clk_gpt)) {
		rc = PTR_ERR(drv->clk_gpt);
		goto err;
	}
	drv->clk_src_type = GPT_CLKSRC_IPG_CLK;

	/* The presence of clk_in means that an external clock will be used for counter*/
	external_clk = devm_clk_get(&pdev->dev, "clk_in");
	if (!IS_ERR(external_clk)) {
		drv->clk_gpt = external_clk;
		drv->clk_src_type = GPT_CLKSRC_EXT_CLK;
	}

	drv->clk_ipg = devm_clk_get(&pdev->dev, "ipg"); //gate
	if (IS_ERR(drv->clk_ipg)) {
		rc = PTR_ERR(drv->clk_ipg);
		goto err;
	}

	platform_set_drvdata(pdev, drv);

	clk_prepare_enable(drv->clk_gpt);
	clk_prepare_enable(drv->clk_ipg);

	if (of_property_read_u32(pdev->dev.of_node, "prescale", &drv->prescale)) {
		rc = -EINVAL;
		pr_err("%s: cannot get prescale \n", __func__);
		goto err_clk;
	}

	if (clock_dev) {

		if (of_property_read_u32_index(pdev->dev.of_node, "rec-channel", 0, &drv->cap_channel)) {
			rc = -EINVAL;
			pr_err("%s: cannot get GPT capture channel id\n", __func__);
			goto err_clk;
		}

		if ((drv->cap_channel > GPT_MAX_CAPTURE_CH_ID) || (drv->cap_channel < GPT_MIN_CAPTURE_CH_ID)) {
			rc = -EINVAL;
			pr_err("%s: GPT capture channel id (%u) out-of-range [%d,%d]\n", __func__, drv->cap_channel, GPT_MIN_CAPTURE_CH_ID, GPT_MAX_CAPTURE_CH_ID);
			goto err_clk;
		}

		if (of_property_read_u32_index(pdev->dev.of_node, "rec-channel", 1, &drv->eth_port)) {
			rc = -EINVAL;
			pr_err("%s: cannot get ethernet port\n", __func__);
			goto err_clk;
		}

		if (drv->eth_port >= CFG_PORTS) {
			rc = -EINVAL;
			pr_err("%s: The Eth port %u is out-of-range ( > %d)\n", __func__, drv->eth_port, CFG_PORTS - 1);
			goto err_clk;
		}

		if (of_property_read_u32_index(pdev->dev.of_node, "rec-channel", 2, &drv->rec.fec_tc_id)) {
			rc = -EINVAL;
			pr_err("%s: cannot get FEC tc id\n", __func__);
			goto err_clk;
		}

		if (of_property_read_u32(pdev->dev.of_node, "domain", &clock_dev->domain)) {
			rc = -EINVAL;
			pr_err("%s: cannot get domain\n", __func__);
			goto err_clk;
		}

		if (imx_pll_init(&drv->rec.pll, &pdev->dev) < 0) {
			rc = -EINVAL;
			pr_err("%s: cannot init the audio PLL\n", __func__);
			goto err_clk;
		}

		rec->pll_ref_freq = clk_get_rate(drv->clk_gpt) / drv->prescale;
		rec->max_adjust = GPT_REC_MAX_ADJUST * IMX_PLL_ADJUST_FACTOR;
		pi_init(&rec->pi, GPT_REC_I_FACTOR, GPT_REC_P_FACTOR);
		rec->factor = GPT_REC_FACTOR * IMX_PLL_ADJUST_FACTOR;

		//FIXME register 2 devices, a REC and a GEN
		clock_dev->type = REC;
		clock_dev->ts_src = TS_INTERNAL;
		clock_dev->start = gpt_rec_start;
		clock_dev->stop = gpt_rec_stop;
		clock_dev->reset = gpt_rec_reset;
		clock_dev->clean = mclock_rec_pll_clean_get;
		clock_dev->open = gpt_rec_open;
		clock_dev->release = NULL;
		clock_dev->config = mclock_rec_pll_config;
	}

	if (timer_dev) {

		if (of_property_read_u32(pdev->dev.of_node, "timer-channel", &drv->timer_channel)) {
			rc = -EINVAL;
			pr_err("%s: cannot get GPT output channel for HW timer \n", __func__);
			goto err_clk;
		}

		if ((drv->timer_channel > GPT_MAX_OUTPUT_CH_ID) || (drv->timer_channel < GPT_MIN_OUTPUT_CH_ID)) {
			rc = -EINVAL;
			pr_err("%s: GPT output compare  channel id (%u) out-of-range [%d,%d]\n", __func__, drv->timer_channel, GPT_MIN_OUTPUT_CH_ID, GPT_MAX_OUTPUT_CH_ID);
			goto err_clk;
		}

		timer_dev->rate = clk_get_rate(drv->clk_gpt) / drv->prescale;
		timer_dev->min_delay_cycles = ((HW_TIMER_MIN_DELAY_US * timer_dev->rate) / USEC_PER_SEC);
		timer_dev->irq = drv->irq;
		timer_dev->start = gpt_hw_timer_start;
		timer_dev->stop = gpt_hw_timer_stop;
		timer_dev->irq_ack = gpt_hw_timer_irq_ack;
		timer_dev->elapsed = gpt_hw_timer_elapsed;
		timer_dev->set_period = gpt_hw_timer_set_period;
		timer_dev->pdev = pdev;

	}

	if (clock_dev) {
		rc = gpt_rec_pin_config(pdev->dev.of_node);
		if (rc)
			goto err_clk;
	}

	/* Do init config before registering/starting any device*/
	gpt_reg_init(drv);

	if (clock_dev) {
		rc = mclock_rec_pll_init(rec);
		if (rc < 0) {
			pr_err("%s: mclock_rec_pll_init error: %d\n", __func__, rc);
			goto err_rec_pll;
		}

		pr_info("%s: rec device(%p), domain: %d, sampling period: %d us, clk input: %ld Hz, pll frequency %lu pll parent (osc) frequency %lu \n",
				__func__, clock_dev, clock_dev->domain, drv->rec.fec_period.i / 1000, clk_get_rate(drv->clk_gpt),
				drv->rec.pll.current_rate, drv->rec.pll.parent_rate);
	}

	if (timer_dev) {

		hw_timer_register_device(timer_dev);

		pr_info("%s : GPT registered as HW timer (%p) device input clk = (%lu Hz) timer-channel = (%u)  prescale = (%d) \n",
			__func__, timer_dev, clk_get_rate(drv->clk_gpt), drv->timer_channel, drv->prescale);
	}


	return 0;

err_rec_pll:
	imx_pll_deinit(&drv->rec.pll);
err_clk:
	clk_disable_unprepare(drv->clk_gpt);
	clk_disable_unprepare(drv->clk_ipg);
err:
	return rc;
}
static int gpt_remove(struct platform_device *pdev)
{
	struct gpt_drv *drv = platform_get_drvdata(pdev);
	u32 val;

	/*Disable GPT*/
	val = readl_relaxed(drv->baseaddr + GPT_CR);
	writel_relaxed(val & ~GPT_EN, drv->baseaddr + GPT_CR);

	if (drv->func_mode & GPT_MCLOCK_REC_MODE) {
		mclock_rec_pll_exit(&drv->rec);
	}

	if (drv->func_mode & GPT_HW_TIMER_MODE)
		hw_timer_unregister_device(&drv->timer_dev);

	platform_set_drvdata(pdev, NULL);

	if (drv->func_mode & GPT_MCLOCK_REC_MODE)
		imx_pll_deinit(&drv->rec.pll);

	clk_disable_unprepare(drv->clk_gpt);
	clk_disable_unprepare(drv->clk_ipg);

	return 0;
}

#define GPT_AVB_DEVICE_COMPATIBLE	"fsl,avb-gpt"

static const struct of_device_id gpt_dt_ids[] = {
	{ .compatible = GPT_AVB_DEVICE_COMPATIBLE, },
	{ /* sentinel */ }
};

MODULE_DEVICE_TABLE(of, gpt_dt_ids);

bool is_gpt_hw_timer_available(void)
{
	struct device_node *node;
	bool ret = false;

	node = of_find_compatible_node(NULL, NULL, GPT_AVB_DEVICE_COMPATIBLE);
	if (!node)
		goto exit;

	if (of_find_property(node, "timer-channel", NULL))
		ret = true;

exit:
	return ret;
}

int gpt_init(struct platform_driver *gpt_driver)
{
	gpt_driver->driver.name = "gpt";
	gpt_driver->driver.owner = THIS_MODULE;
	gpt_driver->driver.of_match_table = gpt_dt_ids;
	gpt_driver->probe = gpt_probe;
	gpt_driver->remove = gpt_remove;

	return platform_driver_register(gpt_driver);
}

void gpt_exit(struct platform_driver *gpt_driver)
{
	platform_driver_unregister(gpt_driver);
}

