/*
 * AVB dma driver
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

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/io.h>
#include <linux/ioctl.h>
#include <linux/dmaengine.h>
#include <linux/dma-mapping.h>
#include <linux/atomic.h>

#include <asm/uaccess.h>

#include "dmadrv.h"
#include "media_clock.h"
#include "hw_timer.h"
#include "avtp.h"
#include "avbdrv.h"
#include "debugfs.h"

#define DMA_SKIP_TIMESTAMPS	2	/* The first two timestamps generated are bogus */
#define MCR_CS2000_NB_PERIOD_LOCK	200  // Maximum number of CLK_IN periods for the PLL to lock (equivalent to the nb of ts copied by the DMA)

/**
 * DOC: Media clock DMA driver
 * In order to keep a good integration with the rest of Linux kernel,
 * this driver uses the generic DMA engine layer of linux kernel. However this API
 * is used in a very specific way and has a specific handling in the actual DMA driver
 * (NXP eDMA or SDMA) which needs to support the AVB channel type.
 */


/**
 * dmadrv_dma_config() - ENET register addresses configuration
 * @dma: pointer to the media clock dma context
 *
 * This is an AVB specific dma_slave_config.
 * src_addr and dst_addr are reused to store the ENET TCCR and
 * TCSR register addresses.
 *
 * Return : 0 on success, negative otherwise
 */
static int dmadrv_dma_enet_config(struct mclock_dma *dma, dma_addr_t tccr, dma_addr_t tcsr)
{
	struct dma_slave_config cfg = {};

	cfg.src_addr = tccr;
	cfg.dst_addr = tcsr;

	if (dma->dev.type == REC)
		cfg.direction = DMA_MEM_TO_DEV;
	else if (dma->dev.type == GEN)
		cfg.direction = DMA_DEV_TO_MEM;
	else
		return -EINVAL;

	return dmaengine_slave_config(dma->chan, &cfg);
}

/**
 * dmadrv_dma_clean_get()
 * @dev: pointer to the media clock device
 *
 * Reset and return the cleaned timestamps counter with the recovery status
 *
 * Return : number of processed _timestamps_
 */

static int dmadrv_dma_clean_get(struct mclock_dev *dev, struct mclock_clean *clean)
{
	struct mclock_dma *dma = container_of(dev, struct mclock_dma, dev);
	int rc = 0;

	clean->nb_clean = atomic_xchg(&dma->nb_clean, 0);
	clean->status = atomic_read(&dma->rec_status);

	return rc;
}

/**
 * dmadrv_dma_clean() - DMA descriptors cleaning
 * @dev: pointer to the media clock device
 *
 * Triggers the cleaning of the DMA descriptors.
 * Convert the number of cleaned descriptors
 * in a number of timestamps.
 *
 * Return : number of processed _timestamps_
 */
static int dmadrv_dma_clean(struct mclock_dev *dev)
{
	struct mclock_dma *dma = container_of(dev, struct mclock_dma, dev);
	struct dma_tx_state state;
	enum dma_status status;
	dma_cookie_t cookie = 0;
	unsigned int completed;

	status = dmaengine_tx_status(dma->chan, cookie, &state);
	if (status == DMA_ERROR) {
		pr_err("%s DMA error\n", __func__);
		return -1;
	}

	/*
	 * The number of timestamps processed is half of processed BDs.
	 * However DMA could be in the middle of a transfer
	 * and we need to keep track of it.
	 */
	state.used += dma->dma_rem;
	completed = state.used / 2;
	dma->dma_rem = state.used % 2;
	atomic_add(completed, &dma->nb_clean);
	dma->nb_clean_total += completed;

	if ((dev->type == REC) && !(dev->flags & MCLOCK_FLAGS_RUNNING_LOCKED)
		&& (dma->nb_clean_total >= MCR_CS2000_NB_PERIOD_LOCK)) {
		/* Set recovery status to locked */
		atomic_set(&dma->rec_status, MCLOCK_RUNNING_LOCKED);
		dev->flags |= MCLOCK_FLAGS_RUNNING_LOCKED;
	}

	dma->stats.dma_clean += completed;

	return completed;
}

 /**
 * dmadrv_dma_prep_sg() - scatter-gather preparation
 * @dma: pointer to the media clock dma context
 *
 * This is an AVB specific scatterlist usage.
 * First element describes the array,
 * second element the TMODE value.
 * Then the DMA driver creates the real input sg that will be used
 * by the DMA engine.
 *
 * Return : 0 on success, negative otherwise
 */
static int dmadrv_dma_prep_sg(struct mclock_dma *dma)
{
	struct scatterlist sg[2];
	struct dma_async_tx_descriptor *desc;

	sg_init_table(sg, 2);
	sg_dma_address(&sg[0]) = dma->dev.sh_mem_dma;
	sg_dma_len(&sg[0]) = dma->dev.num_ts * sizeof(unsigned int);
	sg_dma_address(&sg[1]) = dma->tmode_addr_p;
	sg_dma_len(&sg[1]) = sizeof(unsigned int);

	desc =  dmaengine_prep_slave_sg(dma->chan, sg, 2, 0, 0);
	if (!desc) {
		pr_err("dmaengine_prep_slave_sg failed\n");
		return -1;
	}

	return 0;
}

static void dmadrv_set_clean_period(struct mclock_dev *dev)
{
	struct mclock_dma *dma = container_of(dev, struct mclock_dma, dev);

	if (dev->ts_period.i > (2 * dev->timer_period)) {
		dma->clean_period = dev->ts_period.i - dev->timer_period;
	}
	else
		dma->clean_period = dev->timer_period;

	/* Align drift period on clean period */
	dev->drift_period = (mclock_drift_period(dev) / dma->clean_period) * (dma->clean_period);
}


static int dmadrv_dma_timer_irq(struct mclock_dev *dev, void *data, unsigned int ticks)
{
	struct mclock_dma *dma = container_of(dev, struct mclock_dma, dev);

	if (dev->flags & MCLOCK_FLAGS_RUNNING) {
		unsigned int ptp_now = ((unsigned int *)data)[dev->eth->port];

		dev->clk_timer += ticks * dev->timer_period;

		if (dev->flags & (MCLOCK_FLAGS_INIT | MCLOCK_FLAGS_RESET)) {
			int nb_clean = dmadrv_dma_clean(dev);

			if (nb_clean > 0) {
				if (dev->flags & MCLOCK_FLAGS_INIT) {
					/* Skip first timestamps generated after hw start */
					/* Remain in init state until we have a valid timestamp */

					dma->total_clean += nb_clean;

					if (dma->total_clean <= DMA_SKIP_TIMESTAMPS)
						goto exit;

					dmadrv_w_idx_update(dev, dma->total_clean - DMA_SKIP_TIMESTAMPS);
				} else {
					dmadrv_w_idx_update(dev, nb_clean);
				}

				dev->flags &= ~(MCLOCK_FLAGS_INIT | MCLOCK_FLAGS_RESET);
				rational_init(&dma->clk_media, dev->clk_timer, 1);
				dev->next_drift = dev->clk_timer + dev->drift_period;
				dma->next_clean = dev->clk_timer + dma->clean_period;

				if (dev->flags & MCLOCK_FLAGS_WAKE_UP)
					mclock_wake_up_init_now(dev, dev->clk_timer);

				dma->stats.init++;
			}
			else
				goto exit;
		}

		/* Clean DMA buffers and update media clock
		 */
		if (avtp_after_eq(dev->clk_timer, dma->next_clean)) {
			int nb_clean = dmadrv_dma_clean(dev);
			struct rational delta = dev->ts_period;

			if (nb_clean > 0) {
				dmadrv_w_idx_update(dev, nb_clean);

				rational_int_mul2(&delta, &delta, nb_clean);
				rational_add(&dma->clk_media, &dma->clk_media, &delta);

				/* Drift check */
				if (mclock_drift_adapt(dev, dma->clk_media.i) < 0) {
					dev->flags |= MCLOCK_FLAGS_RESET;
					dev->clk_timer = 0;
					dma->stats.err_drift++;
				}

				dma->next_clean = dev->clk_timer + dma->clean_period;
			}
		}

		/* Update shared grid variables */
		if (dev->type == GEN) {
			*dev->ptp = ptp_now;
		}
	}

exit:
	return 0;
}

/**
 * dmadrv_rec_start() - start the DMA engine
 * @dev: pointer to the media clock device
 * @start: pointer to a mclock_start struct which holds the 2 first timestamps
 *
 * For recovery the DMA is started by the first ENET TCCR compare.
 * 2 timestamps need to be configured in a row (the first one should not expire before
 * the second one has expired).
 * For generation it's just required to start the DMA
 *
 * Return : 0 on success, negative otherwise
 */
static int dmadrv_dma_start(struct mclock_dev *dev, struct mclock_start *start)
{
	struct mclock_dma *dma = container_of(dev, struct mclock_dma, dev);
	int rc = 0;

	dma->dma_rem = 0;
	dma->nb_clean_total = 0;
	atomic_set(&dma->nb_clean, 0);

	/* Check DMA status and incorrect state */
	rc = dmadrv_dma_clean(dev);
	if (rc) {
		if (rc > 0)
			pr_err("%s: DMA not clean: %d\n", __func__, rc);

		rc = -1;
		goto out;
	}

	rc = mclock_drv_register_timer(dev, dmadrv_dma_timer_irq, 1);
	if (rc < 0) {
		pr_err("%s : cannot register timer\n", __func__);
		goto out;
	}

	dev->flags |= MCLOCK_FLAGS_INIT;
	dev->clk_timer = 0;

	dma->total_clean = 0;

	/* Clean on the next timer */
	dma->next_clean = dev->clk_timer + dev->timer_period;
	dmadrv_set_clean_period(dev);

	if (dev->type == GEN) {
		*dev->count = 0;
		*dev->w_idx = DMA_SKIP_TIMESTAMPS;	/* Always skip the first timestamps generated */
	}

	dma_async_issue_pending(dma->chan);

	if (dev->type == REC) {
		writel(start->ts_0, dma->tccr_addr);
		writel(dma->tmode,  dma->tcsr_addr);
		writel(start->ts_1,  dma->tccr_addr);
		/* Set recovery status to running */
		atomic_set(&dma->rec_status, MCLOCK_RUNNING);
		dev->flags &= ~MCLOCK_FLAGS_RUNNING_LOCKED;
	}
	else
		writel(dma->tmode,  dma->tcsr_addr);

	dma->stats.start++;

out:
	return rc;
}

/**
 * dmadrv_dma_stop() - stop the DMA engine
 * @dev: pointer to the media clock device
 *
 * Return : 0 on success, negative otherwise
 */
static int dmadrv_dma_stop(struct mclock_dev *dev)
{
	struct mclock_dma *dma = container_of(dev, struct mclock_dma, dev);

	writel(0, dma->tccr_addr);
	writel(0x80, dma->tcsr_addr);

	mclock_drv_unregister_timer(dev);

	dmaengine_terminate_all(dma->chan);

	/* Set recovery status to stopped */
	if (dev->type == REC) {
		atomic_set(&dma->rec_status, MCLOCK_STOPPED);
		dev->flags &= ~MCLOCK_FLAGS_RUNNING_LOCKED;
	}

	dma->stats.stop++;

	return 0;
}

/**
 * dmadrv_dma_reset() - reset the DMA clock device
 * @dev: pointer to the media clock device
 *
 * Reset the DMA buffer descriptors, and setup any signal
 * to be ready for a start.
 *
 * Return : 0 on success, negative otherwise
 */
static int dmadrv_dma_reset(struct mclock_dev *dev)
{
	struct mclock_dma *dma = container_of(dev, struct mclock_dma, dev);

	if (dev->type == REC) {
		struct mclock_dma_drv *drv = container_of(dma, struct mclock_dma_drv, rec);

		/* Put signal output in high value */
		if (drv->dev_type == IMX6QDL_DMA)
			writel(dma->tmode, dma->tcsr_addr);

		/* Set recovery status to stopped */
		atomic_set(&dma->rec_status, MCLOCK_STOPPED);
		dev->flags &= ~MCLOCK_FLAGS_RUNNING_LOCKED;
	}

	/* Clear flag, stop compare/capture */
	writel(0x80, dma->tcsr_addr);

	return dmadrv_dma_prep_sg(dma);
}

static int dmadrv_dma_open(struct mclock_dev *dev, int port)
{
	struct mclock_dma *dma = container_of(dev, struct mclock_dma, dev);
	struct avb_drv *avb = container_of(dev->drv, struct avb_drv, mclock_drv);

	/*
	 * Ethernet port comes from driver here
	 */
	dev->eth = &avb->eth[dma->eth_port];
	if (!dev->eth ) {
		pr_err("cannot get eth avb\n");
		return -EINVAL;
	}

	return 0;
}

static int dmadrv_dma_config(struct mclock_dev *dev, struct mclock_sconfig *cfg)
{
	struct mclock_dma *dma = container_of(dev, struct mclock_dma, dev);
	int rc = 0;

	switch (cfg->cmd) {
	case MCLOCK_CFG_TS_SRC:
		if (dev->flags & MCLOCK_FLAGS_RUNNING) {
			rc = -EBUSY;
			goto exit;
		}

		if (cfg->ts_src != TS_EXTERNAL) {
			rc = -EINVAL;
			pr_err("%s : internal recovery mode is not supported\n", __func__);
			goto exit;
		}
		dev->ts_src = cfg->ts_src;

		break;

	case MCLOCK_CFG_FREQ:
	{
		unsigned int ts_freq_q = cfg->ts_freq.q;

		if (dev->flags & MCLOCK_FLAGS_RUNNING) {
			rc = -EBUSY;
			goto exit;
		}

		if (dma->tmode == FEC_TMODE_TOGGLE)
			ts_freq_q <<= 1;

		rc = dma->ext_dev->configure(dma->ext_dev->dev, cfg->ts_freq.p, ts_freq_q);

		if (rc >= 0) {
			mclock_set_ts_freq(dev, cfg->ts_freq.p, cfg->ts_freq.q);
			dma->stats.ts_freq_p = cfg->ts_freq.p;
			dma->stats.ts_freq_q = cfg->ts_freq.q;
		}
		break;
	}
	default:
		rc = -EINVAL;
		break;
	}

exit:
	return rc;
}

static int of_avb_get_tc_id(struct device_node *avb_np, const char *name)
{
	int count, idx, found = 0;
	int tc_id = -1;

	count = of_property_count_strings(avb_np, "dma-names");
	if (count <= 0) {
		pr_err("%s: dma-names property missing or empty\n", __func__);
		goto exit;
	}

	for (idx = 0; idx < count; idx++) {
		const char *s;

		if (of_property_read_string_index(avb_np, "dma-names", idx, &s))
			break;

		if (!strcmp(name, s)) {
			found = 1;
			break;
		}
	}

	if (!found) {
		pr_err("%s: cannot match the %s DMA channel \n", __func__, name);
		goto exit;
	}

	if (of_property_read_u32_index(avb_np, "tc_reg_id", idx, &tc_id)) {
		pr_err("%s: cannot get tc_reg_id \n", __func__);
		goto exit;
	}
exit:
	return tc_id;
}

static int dmadrv_dma_init(struct platform_device *pdev, struct mclock_dma *dma, mclock_t type,
					char *dma_name, char *ext_dev_name, unsigned int eth_port)
{
	int ret = 0;
	int tc_id;
	struct device_node *mult_np;
	struct mclock_dma_drv *drv = platform_get_drvdata(pdev);
	struct mclock_dev *dev = &dma->dev;

	dev->type = type;
	dev->domain = drv->domain;
	dev->start = dmadrv_dma_start;
	dev->stop = dmadrv_dma_stop;
	dev->reset =  dmadrv_dma_reset;
	dev->config = dmadrv_dma_config;
	dev->open = dmadrv_dma_open;
	dev->release = NULL;

	dev->timer_period = HW_TIMER_PERIOD_NS;

	if (dev->type == REC) {
		if (drv->dev_type == IMX6QDL_DMA)
			dma->tmode = FEC_TMODE_PULSE_LOW;
		else if (drv->dev_type == VF610_DMA)
			dma->tmode = FEC_TMODE_TOGGLE;

 		dev->num_ts = MCLOCK_REC_NUM_TS;
		dev->sh_mem_size = MCLOCK_REC_DMA_SIZE;
		dev->mmap_size = MCLOCK_REC_MMAP_SIZE;
		dev->clean = dmadrv_dma_clean_get;

		mclock_set_ts_freq(dev, MCLOCK_REC_TS_FREQ_INIT, 1);
	}
	else if (dev->type == GEN) {
		dma->tmode = FEC_TMODE_CAPTURE_RISING;
		dev->num_ts = MCLOCK_GEN_NUM_TS;
		dev->sh_mem_size = MCLOCK_GEN_DMA_SIZE;
		dev->mmap_size = MCLOCK_GEN_MMAP_SIZE;
		dev->clean = NULL;

		mclock_set_ts_freq(dev, MCLOCK_GEN_DMA_TS_FREQ_INIT, 1);
	}
	else {
		dev_err(&pdev->dev, "invalid type: %d\n", dev->type);
		ret = -EINVAL;
		goto err;
	}

	dma->stats.ts_freq_p = dev->ts_freq_p;
	dma->stats.ts_freq_q = dev->ts_freq_q;

	tc_id = of_avb_get_tc_id(pdev->dev.of_node, dma_name);
	if ((tc_id < 0) || (tc_id > 3)) {
		ret = -EINVAL;
		goto err;
	}

	dma->eth_port = eth_port;
	dma->tccr_addr = FEC_ENET_TCCR(drv->enet, tc_id);
	dma->tcsr_addr = FEC_ENET_TCSR(drv->enet, tc_id);

	mult_np = of_parse_phandle(pdev->dev.of_node, ext_dev_name, 0);
	if (!mult_np) {
		dev_err(&pdev->dev, "%s phandle missing or invalid\n", ext_dev_name);
		ret = -EINVAL;
		goto err;
	}

	dma->ext_dev = mclock_drv_bind_ext_device(mult_np);
	if (!dma->ext_dev) {
		dev_err(&pdev->dev, "failed to find a %s device\n", ext_dev_name);
		ret = -EPROBE_DEFER;
		goto err;
	}

	ret = dma->ext_dev->configure(dma->ext_dev->dev, dev->ts_freq_p, dev->ts_freq_q);
	if (ret < 0) {
		dev_err(&pdev->dev, "error configuring external device\n");
		goto err;
	}

	dma->chan = dma_request_slave_channel(&pdev->dev, dma_name);
	if (!dma->chan) {
		dev_err(&pdev->dev, "request for %s dma channel failed\n", dma_name);
		ret = -EINVAL;
		goto err;
	}

	ret = dmadrv_dma_enet_config(dma, FEC_ENET_TCCR(drv->enet_p, tc_id), FEC_ENET_TCSR(drv->enet_p, tc_id));
	if (ret < 0) {
		dev_err(&pdev->dev, "dma_drv_setup_dma failed\n");
		goto err;
	}

	dev->sh_mem = dma_zalloc_coherent(&pdev->dev, dev->sh_mem_size, &dev->sh_mem_dma, GFP_KERNEL);
	if (!dev->sh_mem) {
		ret = -ENOMEM;
		dev_err(&pdev->dev, "Array allocation failed\n");
		goto err_dma;
	}

	dma->tmode_addr = dev->sh_mem + dev->sh_mem_size - sizeof(unsigned int);
	dma->tmode_addr_p = dev->sh_mem_dma + dev->sh_mem_size - sizeof(unsigned int);
	*(unsigned int *)(dma->tmode_addr) = dma->tmode;

	if (dev->type == GEN) {
		dev->w_idx = (unsigned int *)(dev->sh_mem + dev->num_ts * sizeof(unsigned int));
		dev->ptp = dev->w_idx + 1;
		dev->count = dev->w_idx + 2;
	}

	mclock_drv_register_device(dev);

	if (dev->drv->mclock_dentry)
		mclock_dma_debugfs_init(&dma->stats, dev->drv->mclock_dentry, dma_name, dev->domain);

	dev_info(&pdev->dev, "%s: domain: %d, dma %s channel %d, array %p, array_phys %p, tmode %p, tmode_phys %p, tmode %x, tc_id %d \n",
		__func__, dev->domain, dma_name, dma->chan->chan_id, dev->sh_mem, (void *)dev->sh_mem_dma,
		dma->tmode_addr, (void *)dma->tmode_addr_p, *(unsigned int *)(dma->tmode_addr), tc_id);

	return ret;

err_dma:
	dma_release_channel(dma->chan);
err:
	return ret;
}

static int dmadrv_rec_init(struct platform_device *pdev, struct mclock_dma *dma, unsigned int eth_port)
{
	return dmadrv_dma_init(pdev, dma, REC, "rec", "multiplier", eth_port);
}

static int dmadrv_gen_init(struct platform_device *pdev, struct mclock_dma *dma, unsigned int eth_port)
{
	return dmadrv_dma_init(pdev, dma, GEN, "gen", "divider", eth_port);
}

static void dmadrv_dma_exit(struct platform_device *pdev, struct mclock_dma *dma)
{
	struct mclock_dev *dev = &dma->dev;

	if (!mclock_drv_unregister_device(dev)) {
		mclock_drv_unbind_ext_device(dma->ext_dev);
		dma_free_coherent(&pdev->dev, dev->sh_mem_size, dev->sh_mem, dev->sh_mem_dma);
		dma_release_channel(dma->chan);
	}
}

extern const struct of_device_id avb_dt_ids[];

static int dmadrv_probe(struct platform_device *pdev)
{
	struct mclock_dma_drv *drv;
	struct platform_device *enet_pdev;
	struct device_node *np = pdev->dev.of_node;
	struct device_node *enet_np;
	int rec_idx, gen_idx;
	u32 reg[2];
	const struct of_device_id *of_id = of_match_device(avb_dt_ids, &pdev->dev);
	unsigned int eth_port;
	int rc = 0;

	if (of_id) {
		pdev->id_entry = of_id->data;
	}
	else {
		pr_err("Failed to find the right device id.\n");
		rc = -ENOMEM;
		goto err;
	}
	dev_info(&pdev->dev, "%s\n", pdev->id_entry->name);

	rec_idx = of_property_match_string(np, "dma-names", "rec");
	gen_idx = of_property_match_string(np, "dma-names", "gen");

	if ((rec_idx < 0) && (gen_idx < 0)) {
		dev_err(&pdev->dev, "invalid node\n");
		rc = -EINVAL;
		goto err;
	}

	drv = devm_kzalloc(&pdev->dev, sizeof(*drv), GFP_KERNEL);
	if (!drv) {
		rc = -ENOMEM;
		goto err;
	}

	/* FIXME use fec enet driver instead */
	enet_np = of_parse_phandle(pdev->dev.of_node, "enet", 0);
	if (!enet_np) {
		dev_err(&pdev->dev, "enet phandle missing or invalid\n");
		rc = -EINVAL;
		goto err;
	}

	enet_pdev = of_find_device_by_node(enet_np);
	if (!enet_pdev) {
		dev_err(&pdev->dev, "cannot get enet pdevice\n");
		rc = -EINVAL;
		goto err;
	}

	if (of_property_read_u32_array(enet_np, "reg", reg, ARRAY_SIZE(reg))) {
		pr_err("cannot get enet reg field\n");
		rc = -EINVAL;
		goto err;
	}

	drv->enet_p = reg[0];
	drv->enet = devm_ioremap(&pdev->dev, reg[0], reg[1]);
	/* FIXME end */

	if (of_property_read_u32(pdev->dev.of_node, "domain", &drv->domain)) {
		rc = -EINVAL;
		pr_err("%s: cannot get domain\n", __func__);
		goto err;
	}

	drv->dev_type = pdev->id_entry->driver_data;

	eth_port = 0;

	platform_set_drvdata(pdev, drv);

	if (rec_idx >= 0) {
	 	rc = dmadrv_rec_init(pdev, &drv->rec, eth_port);
		if (rc < 0) {
			pr_err("dmadrv_rec_init failed\n");
			goto err;
		}
	}

	if (gen_idx >= 0) {
	 	rc = dmadrv_gen_init(pdev, &drv->gen, eth_port);
		if (rc < 0) {
			pr_err("dmadrv_gen_init failed\n");
			goto err_rec;
		}
	}

err:
	return rc;
err_rec:
	dmadrv_dma_exit(pdev, &drv->rec);

	return rc;
}

static int dmadrv_remove(struct platform_device *pdev)
{
	struct mclock_dma_drv *drv = platform_get_drvdata(pdev);

	dmadrv_dma_exit(pdev, &drv->rec);
	dmadrv_dma_exit(pdev, &drv->gen);

	return 0;
}

static struct platform_device_id mclock_dma_devtypes[] = {
	[VF610_DMA] = {
		.name = "mclock-dma-vf610",
		.driver_data = VF610_DMA,
	},
	[IMX6QDL_DMA] = {
		.name = "mclock-dma-imx6qdl",
		.driver_data = IMX6QDL_DMA,
	}, {
	/* sentinel */
	}
};

MODULE_DEVICE_TABLE(platform, mclock_dma_devtypes);

const struct of_device_id avb_dt_ids[] = {
	{ .compatible = "fsl,imx6q-avb", .data = &mclock_dma_devtypes[IMX6QDL_DMA],},
	{ .compatible = "fsl,vf610-avb", .data = &mclock_dma_devtypes[VF610_DMA],},
	{ /* sentinel */ }
};

MODULE_DEVICE_TABLE(of, avb_dt_ids);

int dmadrv_init(struct platform_driver *p_driver)
{
	p_driver->driver.name = "mclock-dma";
	p_driver->driver.owner = THIS_MODULE;
	p_driver->driver.of_match_table = avb_dt_ids;
	p_driver->probe = dmadrv_probe;
	p_driver->remove = dmadrv_remove;
	p_driver->id_table = mclock_dma_devtypes;

	return platform_driver_register(p_driver);
}

void dmadrv_exit(struct platform_driver *p_driver)
{
	platform_driver_unregister(p_driver);
}

