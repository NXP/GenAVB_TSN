/*
 * AVB FTM driver
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

#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/fec.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/module.h>
#include "ftm.h"
#include "hw_timer.h"
#include "net_port.h"
#include "avbdrv.h"
#include "imx-pll.h"


/**
 * DOC: FTM timer
 *
 * FlexTimer hardware timer is used as a possible source to generate 125us periodic
 * interrupts with low overhead.
 */

static void ftm_timer_start(struct hw_timer_dev *dev)
{
	struct ftm *f = platform_get_drvdata(dev->pdev);
	u16 cnt_val;

	cnt_val = readl_relaxed(f->baseaddr + FTM_CNT);
	f->timer_next_val = cnt_val + (u16)dev->period;

	writel_relaxed(f->timer_next_val, f->baseaddr + FTM_CV(f->timer_channel));
	writel_relaxed(FTM_CSC_MSA | FTM_CSC_CHIE, f->baseaddr + FTM_CSC(f->timer_channel));
}

static void ftm_timer_stop(struct hw_timer_dev *dev)
{
	struct ftm *f = platform_get_drvdata(dev->pdev);

	writel_relaxed(0, f->baseaddr+ FTM_CSC(f->timer_channel));
}

static void ftm_timer_set_period(struct hw_timer_dev *dev, unsigned long period_us)
{
	struct ftm *f = platform_get_drvdata(dev->pdev);

	dev->period = (period_us * (dev->rate / 1000)) / 1000 ;

	if (((dev->period * 1000) / (dev->rate / 1000)) != period_us)
		pr_warn("%s: dev(%p) warning the HW timer period is incorrect",
			__func__, dev);

	pr_info("%s: ftm (%p) set period %lu(us), %u(cycles)\n",
		__func__, f, period_us, dev->period);
}

static unsigned int ftm_timer_elapsed(struct hw_timer_dev *dev, unsigned int cycles)
{
	struct ftm *f = platform_get_drvdata(dev->pdev);
	u16 cycles_now;

	cycles_now = readl_relaxed(f->baseaddr + FTM_CNT);

	return cycles_now - (u16)cycles;
}


static unsigned int ftm_timer_irq_ack(struct hw_timer_dev *dev, unsigned int *cycles, unsigned int *dcycles)
{
	struct ftm *f = platform_get_drvdata(dev->pdev);
	unsigned int ticks = 0;
	u32 csc_val;
	u16 cycles_now, num_recovery = 0;

	cycles_now = readl_relaxed(f->baseaddr + FTM_CNT);

	*cycles = cycles_now;

try_again:
	/* clear status bit  */
	csc_val = readl_relaxed(f->baseaddr + FTM_CSC(f->timer_channel));
	csc_val &= (~FTM_CSC_CHF);
	writel_relaxed(csc_val, f->baseaddr + FTM_CSC(f->timer_channel));

	*dcycles = cycles_now - f->timer_next_val;

	/* Check how many periods have elapsed since last time
	 * Normally a single period has elapsed, but we may be late */
	do {
		f->timer_next_val += (u16)dev->period;
		ticks++;
	} while ((int)(f->timer_next_val - cycles_now) < 0);

	/* Set next timer value */
	writel_relaxed(f->timer_next_val, f->baseaddr + FTM_CV(f->timer_channel));

	/* Check if the free running counter has already passed
	the newly programmed timer target value */
	cycles_now = readl_relaxed(f->baseaddr + FTM_CNT);

	if ((int)(cycles_now - f->timer_next_val) > 0) {
		num_recovery++;

		/* hard protection against potential dead loop (> 2*125us) */
		if (num_recovery > 2)
			pr_err("ftm(%p) - recovery failure\n", f);
		else
			goto try_again;
	}

	dev->recovery_errors += num_recovery;

	return ticks;
}

static void ftm_timer_init(struct ftm *f)
{
	u32 sc_val;

	/* FTM global config */
	/* Set prescaler and external config */
	sc_val = (FTM_SC_PS_MASK & ftm_prescale_to_regval(f->prescale)) | FTM_SC_CLK(FTM_SC_CLK_EXT);
	writel_relaxed(sc_val, f->baseaddr + FTM_SC);

	/* Modulo config */
	/* 16 bits */
	writel_relaxed(0xFFFF, f->baseaddr + FTM_MOD);
}

int ftm_rec_timer_irq(struct mclock_dev *dev, void *data, unsigned int ticks)
{
	struct mclock_rec_pll *rec = container_of(dev, struct mclock_rec_pll, dev);
	struct ftm *f = container_of(rec, struct ftm, rec);
	u32 csc_val;
	u16 cnt_val;

	raw_read_lock(&dev->eth->lock);

	/* get status  */
	csc_val = readl_relaxed(f->baseaddr + FTM_CSC(f->rec_channel));
	if (csc_val & FTM_CSC_CHF) {

		/* get value */
		cnt_val = readl_relaxed(f->baseaddr + FTM_CV(f->rec_channel));

		mclock_rec_pll_timer_irq(rec, 1, (u16)(cnt_val - f->cnt_last), ticks);
		f->cnt_last = cnt_val;

		/* clear status bit
		 * Do this _after_ mclock_rec_pll_timer_irq because internal resets
		 * can cause FEC ENET compare output signal toggling.
		 */
		csc_val &= ~FTM_CSC_CHF;
		writel_relaxed(csc_val, f->baseaddr + FTM_CSC(f->rec_channel));
	}
	else
		mclock_rec_pll_timer_irq(rec, 0, 0, ticks);

	raw_read_unlock(&dev->eth->lock);

	return 0;
}

int ftm_rec_start(struct mclock_dev *dev, struct mclock_start *start)
{
	struct mclock_rec_pll *rec = container_of(dev, struct mclock_rec_pll, dev);
	struct ftm *f = container_of(rec, struct ftm, rec);
	int rc = 0;

	/* Start ENET */
	mclock_rec_pll_start(rec, start);

	/* Main FTM recovery handler is registered every ms */
	rc = mclock_drv_register_timer(dev, ftm_rec_timer_irq, 1);
	if (rc < 0) {
		mclock_rec_pll_stop(rec);
		pr_err("%s : cannot register timer\n", __func__);
		goto out;
	}

	/* Enable FTM recovery channel and trigger on both edges, no interrupt */
	writel_relaxed(FTM_CSC_ELSA | FTM_CSC_ELSB, f->baseaddr + FTM_CSC(f->rec_channel));

out:
	return rc;
}

int ftm_rec_stop(struct mclock_dev *dev)
{
	struct mclock_rec_pll *rec = container_of(dev, struct mclock_rec_pll, dev);
	struct ftm *f = container_of(rec, struct ftm, rec);
	int rc = 0;

	mclock_drv_unregister_timer(dev);

	rc = mclock_rec_pll_stop(rec);
	if (rc < 0)
		pr_err("%s : cannot stop enet\n", __func__);

	writel_relaxed(0, f->baseaddr + FTM_CSC(f->rec_channel));

	pr_info("%s: dev(%p)\n", __func__, dev);

	return rc;
}

int ftm_rec_open(struct mclock_dev *dev, int port)
{
	struct mclock_rec_pll *rec = container_of(dev, struct mclock_rec_pll, dev);
	struct ftm *f = container_of(rec, struct ftm, rec);
	struct avb_drv *avb = container_of(dev->drv, struct avb_drv, mclock_drv);

	dev->eth = &avb->eth[f->eth_port];
	if (!dev->eth)
		return -ENODEV;

	return 0;
}

static int ftm_probe(struct platform_device *pdev)
{
	struct ftm *f;
	struct resource *iores;
	struct hw_timer_dev *timer_dev = NULL;
	struct mclock_dev *clock_dev = NULL;
	int rc = 0;

	f = kzalloc(sizeof(*f), GFP_KERNEL);
	if (!f) {
		rc = -ENOMEM;
		goto err_mem;
	}

	if (of_find_property(pdev->dev.of_node, "timer-channel", NULL))
		timer_dev = &f->timer_dev;

	if (of_find_property(pdev->dev.of_node, "rec-channel", NULL))
		clock_dev = &f->rec.dev;

	if (!(timer_dev || clock_dev)) {
		rc = -EINVAL;
		pr_err("%s: missing properties in device tree node\n", __func__);
		goto err_of;
	}

	iores = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!iores) {
		rc = -ENODEV;
		goto err_get_resource;
	}

	f->start = iores->start;
	f->size = resource_size(iores);

	f->irq = platform_get_irq(pdev, 0);
	if (f->irq < 0) {
		rc = -ENODEV;
		goto err_get_irq;
	}

	if (!request_mem_region(f->start, f->size, pdev->name)) {
		rc = -EBUSY;
		goto err_request_mem;
	}

	f->baseaddr = ioremap(f->start, f->size);
	if (!f->baseaddr) {
		rc = -ENOMEM;
		goto err_ioremap;
	}

	f->clk_ftm = devm_clk_get(&pdev->dev, "clk_in"); //counter clock
	if (IS_ERR(f->clk_ftm)) {
		rc = PTR_ERR(f->clk_ftm);
		goto err_clk_get;
	}

	f->clk_ipg = devm_clk_get(&pdev->dev, "ipg"); //gate
	if (IS_ERR(f->clk_ipg)) {
		rc = PTR_ERR(f->clk_ipg);
		goto err_clk_get;
	}

	if (of_property_read_u32(pdev->dev.of_node, "prescale", &f->prescale)) {
		rc = -EINVAL;
		pr_err("%s: cannot get prescale \n", __func__);
		goto err_prescale;
	}

	platform_set_drvdata(pdev, f);

	clk_prepare_enable(f->clk_ftm);
	clk_prepare_enable(f->clk_ipg);

	if (timer_dev) {

		if (of_property_read_u32(pdev->dev.of_node, "timer-channel", &f->timer_channel)) {
			rc = -EINVAL;
			pr_err("%s: cannot get prescale \n", __func__);
			goto err_of_timer;
		}

		f->timer_next_val = 0;
		timer_dev->rate = clk_get_rate(f->clk_ftm) / f->prescale;
		timer_dev->min_delay_cycles = (HW_TIMER_MIN_DELAY_US * timer_dev->rate) / USEC_PER_SEC;
		timer_dev->irq = f->irq;
		timer_dev->start = ftm_timer_start;
		timer_dev->stop = ftm_timer_stop;
		timer_dev->irq_ack = ftm_timer_irq_ack;
		timer_dev->elapsed = ftm_timer_elapsed;
		timer_dev->set_period = ftm_timer_set_period;
		timer_dev->pdev = pdev;
		hw_timer_register_device(timer_dev);
	}

	if (clock_dev) {
		struct mclock_rec_pll *rec = &f->rec;

		if (of_property_read_u32(pdev->dev.of_node, "domain", &clock_dev->domain)) {
			rc = -EINVAL;
			pr_err("%s: cannot get domain\n", __func__);
			goto err_of_clock;
		}

		if (of_property_read_u32_index(pdev->dev.of_node, "rec-channel", 0, &f->rec_channel)) {
			rc = -EINVAL;
			pr_err("%s: cannot get rec FTM channel id\n", __func__);
			goto err_of_clock;
		}

		if (of_property_read_u32_index(pdev->dev.of_node, "rec-channel", 1, &f->eth_port)) {
			rc = -EINVAL;
			pr_err("%s: cannot get ethernet port\n", __func__);
			goto err_of_clock;
		}

		if (of_property_read_u32_index(pdev->dev.of_node, "rec-channel", 2, &f->rec.fec_tc_id)) {
			rc = -EINVAL;
			pr_err("%s: cannot get FEC tc id\n", __func__);
			goto err_of_clock;
		}

		if (imx_pll_init(&rec->pll, &pdev->dev) < 0) {
			rc = -EINVAL;
			pr_err("%s: cannot init the audio PLL\n", __func__);
			goto err_of_clock;
		}


		rec->pll_ref_freq = clk_get_rate(f->clk_ftm) / f->prescale;
		pi_init(&rec->pi, FTM_REC_I_FACTOR, FTM_REC_P_FACTOR);
		rec->factor = FTM_REC_FACTOR * IMX_PLL_ADJUST_FACTOR;
		rec->max_adjust = FTM_REC_MAX_ADJUST * IMX_PLL_ADJUST_FACTOR;

		clock_dev->type = REC;
		clock_dev->ts_src = TS_INTERNAL;	//Default for now as external not supproted
		clock_dev->start = ftm_rec_start;
		clock_dev->stop = ftm_rec_stop;
		clock_dev->reset = NULL;
		clock_dev->clean = NULL;
		clock_dev->open = ftm_rec_open;
		clock_dev->release = NULL;
		clock_dev->config = mclock_rec_pll_config;

		rc = mclock_rec_pll_init(rec);
		if (rc < 0) {
			pr_err("%s: mclock_rec_pll_init error: %d\n", __func__, rc);
			goto err_rec_pll;
		}

		pr_info("%s: rec device(%p), domain: %d, sampling period: %d us, clk input: %ld Hz, "
			"prescale: %d, audio pll frequency %lu pll parent (osc) frequency %lu \n",
			__func__, clock_dev, clock_dev->domain, f->rec.fec_period.i / 1000, clk_get_rate(f->clk_ftm),
			f->prescale, f->rec.pll.current_rate, f->rec.pll.parent_rate);
	}

	ftm_timer_init(f);

	return 0;

err_rec_pll:
	imx_pll_deinit(&f->rec.pll);
err_of_clock:
	if (timer_dev)
		hw_timer_unregister_device(timer_dev);

err_of_timer:
	clk_disable_unprepare(f->clk_ftm);
	clk_disable_unprepare(f->clk_ipg);

err_prescale:
err_clk_get:
	iounmap(f->baseaddr);

err_ioremap:
	release_mem_region(f->start, f->size);

err_request_mem:
err_get_irq:
err_get_resource:
err_of:
	kfree(f);

err_mem:
	return rc;
}

static int ftm_remove(struct platform_device *pdev)
{
	struct ftm *f = platform_get_drvdata(pdev);

	hw_timer_unregister_device(&f->timer_dev);

	mclock_rec_pll_exit(&f->rec);

	platform_set_drvdata(pdev, NULL);

	clk_disable_unprepare(f->clk_ftm);
	clk_disable_unprepare(f->clk_ipg);

	if (&f->rec.dev)
		imx_pll_deinit(&f->rec.pll);

	iounmap(f->baseaddr);

	release_mem_region(f->start, f->size);

	kfree(f);

	return 0;
}


static const struct of_device_id ftm_dt_ids[] = {
	{ .compatible = "fsl,avb-ftm", },
	{ /* sentinel */ }
};

MODULE_DEVICE_TABLE(of, ftm_dt_ids);

int ftm_init(struct platform_driver *ftm_driver)
{
	ftm_driver->driver.name = "ftm";
	ftm_driver->driver.owner = THIS_MODULE;
	ftm_driver->driver.of_match_table = ftm_dt_ids;
	ftm_driver->probe = ftm_probe;
	ftm_driver->remove = ftm_remove;

	return platform_driver_register(ftm_driver);
}

void ftm_exit(struct platform_driver *ftm_driver)
{
	platform_driver_unregister(ftm_driver);
}

