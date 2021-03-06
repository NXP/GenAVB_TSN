/*
* Copyright 2018-2020 NXP
* 
* NXP Confidential. This software is owned or controlled by NXP and may only 
* be used strictly in accordance with the applicable license terms.  By expressly 
* accepting such terms or by downloading, installing, activating and/or otherwise 
* using the software, you are agreeing that you have read, and that you agree to 
* comply with and are bound by, such license terms.  If you do not agree to be 
* bound by the applicable license terms, then you may not retain, install, activate 
* or otherwise use the software.
*/

/**
 @file
 @brief AVB GPT media clock recovery driver
 @details
*/

#include "gpt_rec.h"
#include "gpt.h"
#include "imx-pll.h"

static int gpt_rec_timer_irq(struct mclock_dev *dev, void *data, unsigned int ticks)
{
	struct mclock_rec_pll *rec = container_of(dev, struct mclock_rec_pll, dev);
	struct gpt_rec *gpt_rec = container_of(rec, struct gpt_rec, rec);
	struct gpt_dev *gpt_dev = container_of(gpt_rec, struct gpt_dev, gpt_rec);
	uint32_t cnt_val;

	/* get status */
	if (GPT_GetStatusFlags(gpt_dev->base, gpt_rec->capture.status_flag_mask)) {
		/* get value */
		cnt_val = GPT_GetInputCaptureValue(gpt_dev->base, gpt_rec->capture.channel);

		mclock_rec_pll_timer_irq(rec, 1, (uint32_t)(cnt_val - gpt_rec->cnt_last), ticks);
		gpt_rec->cnt_last = cnt_val;

		/* clear status bit
		 * Do this _after_ mclock_rec_pll_timer_irq because internal resets
		 * can cause FEC ENET compare output signal toggling.
		 */
		GPT_ClearStatusFlags(gpt_dev->base, gpt_rec->capture.status_flag_mask);

		rec->stats.irq_count_fec_event++;
	} else {
		mclock_rec_pll_timer_irq(rec, 0, 0, ticks);
	}

	rec->stats.irq_count++;

	return 0;
}

static int gpt_rec_start(struct mclock_dev *dev, struct mclock_start *start)
{
	struct mclock_rec_pll *rec = container_of(dev, struct mclock_rec_pll, dev);
	int rc = 0;

	/* Start ENET */
	mclock_rec_pll_start(rec, start);

	rc = mclock_register_timer(dev, gpt_rec_timer_irq, 1);
	if (rc < 0) {
		mclock_rec_pll_stop(rec);
		os_log(LOG_ERR, "cannot register timer\n");
		goto out;
	}

out:
	return rc;
}

static int gpt_rec_stop(struct mclock_dev *dev)
{
	struct mclock_rec_pll *rec = container_of(dev, struct mclock_rec_pll, dev);
	int rc = 0;

	mclock_unregister_timer(dev);

	rc = mclock_rec_pll_stop(rec);
	if (rc < 0)
		os_log(LOG_ERR, "cannot stop enet\n");

	return rc;
}

static int gpt_rec_reset(struct mclock_dev *dev)
{
	return 0;
}

static int gpt_rec_open(struct mclock_dev *dev, int port)
{
	struct mclock_rec_pll *rec = container_of(dev, struct mclock_rec_pll, dev);
	struct gpt_rec *gpt_rec = container_of(rec, struct gpt_rec, rec);

	dev->eth = &ports[gpt_rec->eth_port];
	if (!dev->eth)
		return -1;

	return 0;
}

__init int gpt_rec_init(struct gpt_rec *gpt_rec)
{
	struct mclock_rec_pll *rec = &gpt_rec->rec;
	struct mclock_dev *clock_dev = &gpt_rec->rec.dev;
	struct gpt_dev *gpt_dev = container_of(gpt_rec, struct gpt_dev, gpt_rec);

	GPT_SetInputOperationMode(gpt_dev->base, gpt_rec->capture.channel, gpt_rec->capture.operation_mode);

	if (imx_pll_init(&rec->pll) < 0) {
		os_log(LOG_ERR, "cannot init the audio PLL\n");
		goto err;
	}

	clock_dev->domain = DOMAIN_0;

	rec->pll_ref_freq = gpt_dev->gpt_input_clk_rate / gpt_dev->prescale;
	rec->max_adjust = GPT_REC_MAX_ADJUST * IMX_PLL_ADJUST_FACTOR;
	pi_init(&rec->pi, GPT_REC_I_FACTOR, GPT_REC_P_FACTOR);
	rec->factor = GPT_REC_FACTOR * IMX_PLL_ADJUST_FACTOR;

	// FIXME register 2 devices, a REC and a GEN
	clock_dev->type = REC;
	clock_dev->ts_src = TS_INTERNAL;
	clock_dev->start = gpt_rec_start;
	clock_dev->stop = gpt_rec_stop;
	clock_dev->reset = gpt_rec_reset;
	clock_dev->clean = mclock_rec_pll_clean_get;
	clock_dev->open = gpt_rec_open;
	clock_dev->release = NULL;
	clock_dev->config = mclock_rec_pll_config;

	if (mclock_rec_pll_init(rec) < 0) {
		os_log(LOG_ERR, "mclock_rec_pll_init error\n");
		goto err_rec_pll;
	}

	os_log(LOG_INIT, "rec device(%p), clk input: %ld Hz, pll frequency %lu, pll parent (osc) frequency %lu\n",
		clock_dev, gpt_dev->gpt_input_clk_rate,
		imx_pll_get_rate(&rec->pll), rec->pll.parent_rate);

	return 0;

err_rec_pll:
	imx_pll_deinit(&rec->pll);

err:
	return -1;
}

__exit void gpt_rec_exit(struct gpt_rec *gpt_rec)
{
	mclock_rec_pll_exit(&gpt_rec->rec);
	imx_pll_deinit(&gpt_rec->rec.pll);
}
