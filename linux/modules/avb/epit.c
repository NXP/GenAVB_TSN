/*
 * AVB epit driver
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
#include <linux/of_platform.h>
#include <linux/module.h>
#include <asm/io.h>
#include "epit.h"

/**
 * DOC: EPIT timer
 *
 * Epit hardware timer is used as a possible source to generate 125us periodic
 * interrupts with low overhead.
 */

static void epit_start(struct hw_timer_dev *dev)
{
	struct epit *p = platform_get_drvdata(dev->pdev);

	p->next_cycles = (u32)0xffffffff - (u32)dev->period;
	writel_relaxed(p->next_cycles, p->baseaddr + EPIT_CMPR);

	writel_relaxed(EPIT_CTRL | EPIT_CR_EN(1) | EPIT_CR_OCIEN(1), p->baseaddr + EPIT_CR);
}

static void epit_stop(struct hw_timer_dev *dev)
{
	struct epit *p = platform_get_drvdata(dev->pdev);

	writel_relaxed(EPIT_CTRL | EPIT_CR_EN(0) | EPIT_CR_OCIEN(0), p->baseaddr + EPIT_CR);
	writel_relaxed(1, p->baseaddr + EPIT_SR);
}

static void epit_set_period(struct hw_timer_dev *dev, unsigned long period_us)
{
	struct epit *p = platform_get_drvdata(dev->pdev);

	dev->period = (period_us * (dev->rate / 1000)) / 1000;

	pr_info("epit(%p) set period %lu(us), %u(cycles)\n", p, period_us, dev->period);
}

static unsigned int epit_elapsed(struct hw_timer_dev *dev, unsigned int cycles)
{
	struct epit *p = platform_get_drvdata(dev->pdev);
	u32 cycles_now;

	cycles_now = readl_relaxed(p->baseaddr + EPIT_CNR);

	return (u32)cycles - cycles_now;
}


static unsigned int epit_irq_ack(struct hw_timer_dev *dev, unsigned int *cycles, unsigned int *dcycles)
{
	struct epit *p = platform_get_drvdata(dev->pdev);
	unsigned int ticks = 0;
	u32 cycles_now, num_recovery = 0;

	cycles_now = readl_relaxed(p->baseaddr + EPIT_CNR);

	*cycles = cycles_now;

try_again:
	writel_relaxed(1, p->baseaddr + EPIT_SR);

	*dcycles = p->next_cycles - cycles_now;

	/* Check how many periods have elapsed since last time
	 * Normally a single period has elapsed, but we may be late */
	do {
		p->next_cycles -= (u32)dev->period;
		ticks++;
	} while ((int)(p->next_cycles - cycles_now) > 0);

	writel_relaxed(p->next_cycles, p->baseaddr + EPIT_CMPR);

	/* Check if the free running counter has already passed
	the newly programmed CMPR value */
	cycles_now = readl_relaxed(p->baseaddr + EPIT_CNR);

	if ((int)(cycles_now - p->next_cycles) < 0) {
		num_recovery++;

		/* hard protection against potential dead loop (> 2*125us) */
		if (num_recovery > 2)
			pr_err("epit(%p) - recovery failure\n", p);
		else
			/* try to set again the CMPR below CNR */
			goto try_again;
	}

	dev->recovery_errors += num_recovery;

	return ticks;
}

static int epit_probe(struct platform_device *pdev)
{
	struct epit *p;
	struct resource *iores;
	int rc;

	p = kzalloc(sizeof(*p), GFP_KERNEL);
	if (!p) {
		rc = -ENOMEM;
		goto err_alloc;
	}

	iores = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!iores) {
		rc = -ENODEV;
		goto err_get_resource;
	}

	p->start = iores->start;
	p->size = resource_size(iores);

	p->irq = platform_get_irq(pdev, 0);
	if (p->irq < 0) {
		rc = -ENODEV;
		goto err_get_irq;
	}

	if (!request_mem_region(p->start, p->size, pdev->name)) {
		rc = -EBUSY;
		goto err_request_mem;
	}

	p->baseaddr = ioremap(p->start, p->size);
	if (!p->baseaddr) {
		rc = -ENOMEM;
		goto err_ioremap;
	}

	p->clk = devm_clk_get(&pdev->dev, NULL);
	if (IS_ERR(p->clk)) {
		rc = PTR_ERR(p->clk);
		goto err_clk_get;
	}

	clk_prepare_enable(p->clk);

	platform_set_drvdata(pdev, p);

	writel_relaxed(EPIT_CTRL, p->baseaddr + EPIT_CR);
	writel_relaxed(1, p->baseaddr + EPIT_SR);

	p->timer_dev.rate = clk_get_rate(p->clk);
	p->timer_dev.min_delay_cycles = (HW_TIMER_MIN_DELAY_US * p->timer_dev.rate) / USEC_PER_SEC;
	p->timer_dev.irq = p->irq;
	p->timer_dev.start = epit_start;
	p->timer_dev.stop = epit_stop;
	p->timer_dev.irq_ack = epit_irq_ack;
	p->timer_dev.elapsed = epit_elapsed;
	p->timer_dev.set_period = epit_set_period;
	p->timer_dev.pdev = pdev;

	hw_timer_register_device(&p->timer_dev);

	return 0;

err_clk_get:
	iounmap(p->baseaddr);

err_ioremap:
	release_mem_region(p->start, p->size);

err_request_mem:
err_get_irq:
err_get_resource:
	kfree(p);

err_alloc:
	return rc;
}

static int epit_remove(struct platform_device *pdev)
{
	struct epit *p = platform_get_drvdata(pdev);

	hw_timer_unregister_device(&p->timer_dev);

	platform_set_drvdata(pdev, NULL);

	clk_disable_unprepare(p->clk);

	iounmap(p->baseaddr);

	release_mem_region(p->start, p->size);

	kfree(p);

	return 0;
}

static const struct of_device_id epit_dt_ids[] = {
	{ .compatible = "fsl,imx6ql-epit", },
	{ /* sentinel */ }
};

MODULE_DEVICE_TABLE(of, epit_dt_ids);

int epit_init(struct platform_driver *epit_driver)
{
	epit_driver->driver.name = "epit";
	epit_driver->driver.owner = THIS_MODULE;
	epit_driver->driver.of_match_table = epit_dt_ids;
	epit_driver->probe = epit_probe;
	epit_driver->remove = epit_remove;

	return platform_driver_register(epit_driver);
}

void epit_exit(struct platform_driver *epit_driver)
{
	platform_driver_unregister(epit_driver);
}
