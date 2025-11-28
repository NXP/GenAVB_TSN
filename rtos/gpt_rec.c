/*
 * Copyright 2018-2019, 2021-2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief AVB GPT media clock recovery driver
 @details
*/

#include "rtos_abstraction_layer.h"

#include "config.h"

#include "gpt_rec.h"
#include "clock.h"
#include "imx-pll.h"
#include "media_clock_rec_pll.h"

#if defined(CONFIG_GENAVB_TSN_AVTP) && (CFG_NUM_GPT > 0) && defined(BOARD_GPT_REC_BASE)

#include "fsl_gpt.h"

#define DOMAIN_0		0

struct gpt_input_capture {
	gpt_input_capture_channel_t channel;
	gpt_interrupt_enable_t interrupt_mask;
	gpt_status_flag_t status_flag_mask;
	gpt_input_operation_mode_t operation_mode;
};

struct gpt_rec {
	void *base;
	struct mclock_rec_pll rec;
	bool sw_sampling;                 /* Software based sampling of PLL counter/ gPTP timestamps using software register reads */
	struct gpt_input_capture capture; /* Hardware based sampling of PLL counter / gPTP timestamps using the GPT Input Capture feature. */

	unsigned int eth_port;
};

static struct gpt_rec gpt_rec = {
	.base = BOARD_GPT_REC_BASE,
#if !defined(BOARD_GPT_REC_CHANNEL) && defined(BOARD_GPT_REC_SW_SAMPLING)
	.sw_sampling = true,
#else
	.sw_sampling = false,

	.capture = {
#if (BOARD_GPT_REC_CHANNEL == 1)
		.channel = kGPT_InputCapture_Channel1,
		.interrupt_mask = kGPT_InputCapture1InterruptEnable,
		.status_flag_mask = kGPT_InputCapture1Flag,
#elif (BOARD_GPT_REC_CHANNEL == 2)
		.channel = kGPT_InputCapture_Channel2,
		.interrupt_mask = kGPT_InputCapture2InterruptEnable,
		.status_flag_mask = kGPT_InputCapture2Flag,
#elif (BOARD_GPT_REC_CHANNEL == 3)
		.channel = kGPT_InputCapture_Channel3,
		.interrupt_mask = kGPT_InputCapture3InterruptEnable,
		.status_flag_mask = kGPT_InputCapture3Flag,
#else
#error Invalid BOARD_GPT_REC_CHANNEL
#endif
		.operation_mode = kGPT_InputOperation_BothEdge,
	},
#endif /* !defined(BOARD_GPT_REC_CHANNEL) && defined(BOARD_GPT_REC_SW_SAMPLING) */
};

static int gpt_rec_timer_irq(struct mclock_dev *dev, void *data, unsigned int ticks)
{
	struct mclock_rec_pll *rec = mclock_dev_to_rec(dev);
	struct gpt_rec *gpt_rec = container_of(rec, struct gpt_rec, rec);
	uint32_t cnt_val;

	/* get status */
	if (GPT_GetStatusFlags(gpt_rec->base, gpt_rec->capture.status_flag_mask)) {
		/* get value */
		cnt_val = GPT_GetInputCaptureValue(gpt_rec->base, gpt_rec->capture.channel);

		mclock_rec_pll_common_timer_irq(&rec->c, MCLOCK_REC_PLL_FLAGS_GPTP_EVENT, (uint32_t)(cnt_val - rec->c.audio_pll_cnt_last), cnt_val, ticks);
		rec->c.audio_pll_cnt_last = cnt_val;

		/* clear status bit
		 * Do this _after_ mclock_rec_pll_common_timer_irq() because internal resets
		 * can cause FEC ENET compare output signal toggling.
		 */
		GPT_ClearStatusFlags(gpt_rec->base, gpt_rec->capture.status_flag_mask);

		rec->stats.irq_count_fec_event++;
	} else {
		mclock_rec_pll_common_timer_irq(&rec->c, 0, 0, 0, ticks);
	}

	rec->stats.irq_count++;

	return 0;
}

#if !defined(BOARD_GPT_REC_CHANNEL) && defined(BOARD_GPT_REC_SW_SAMPLING)
static int gpt_sw_rec_timer_irq(struct mclock_dev *dev, void *data, unsigned int ticks)
{
	struct mclock_rec_pll *rec = mclock_dev_to_rec(dev);
	struct gpt_rec *gpt_rec = container_of(rec, struct gpt_rec, rec);
	struct gptp_dev *gptp_dev = rec->gptp_event_dev;
	uint32_t cnt_val;
	uint64_t now;
	int rc = 0;

	/* Sample ptp counter and audio pll cycles "simultaneously" (with interrupts
	 * and scheduling disabled).
	 */
	rtos_spin_lock(&rtos_global_spinlock, &rtos_global_key);
	if (os_clock_gettime64_isr(gptp_dev->port->clock[PORT_CLOCK_GPTP_0], &now) < 0) {
		rc = -1;
		rec->c.stats.err_gptp_gettime++;
		rtos_spin_unlock(&rtos_global_spinlock, rtos_global_key);
		goto out;
	}

	cnt_val = GPT_GetCurrentTimerCount(gpt_rec->base);
	rtos_spin_unlock(&rtos_global_spinlock, rtos_global_key);

	mclock_rec_pll_common_sw_sampling_irq(&rec->c, cnt_val, (uint32_t)now, ticks);

out:
	return rc;
}
#endif

static int gpt_rec_start(struct mclock_dev *dev, struct mclock_start *start)
{
	struct mclock_rec_pll *rec = mclock_dev_to_rec(dev);
	int rc = 0;

	/* Start ENET */
	mclock_rec_pll_start(rec, start);

	if (rec->c.is_hw_recovery_mode)
		rc = mclock_register_timer(dev, &gpt_rec_timer_irq, 1);
#if !defined(BOARD_GPT_REC_CHANNEL) && defined(BOARD_GPT_REC_SW_SAMPLING)
	else
		rc = mclock_register_timer(dev, &gpt_sw_rec_timer_irq, 1);
#endif

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
	struct mclock_rec_pll *rec = mclock_dev_to_rec(dev);
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
	struct mclock_rec_pll *rec = mclock_dev_to_rec(dev);
	struct gpt_rec *gpt_rec = container_of(rec, struct gpt_rec, rec);

	dev->eth = &ports[gpt_rec->eth_port];
	if (!dev->eth)
		return -1;

	return 0;
}

__init int gpt_rec_init(void)
{
	struct mclock_rec_pll *rec = &gpt_rec.rec;
	struct mclock_dev *clock_dev = &gpt_rec.rec.c.dev;
	unsigned int input_clk_rate;
	unsigned int prescale;

	input_clk_rate = BOARD_GPT_clk_freq(gpt_rec.base);
	prescale = GPT_GetClockDivider(gpt_rec.base);

	if (!gpt_rec.sw_sampling) {
		rec->c.is_hw_recovery_mode = true;
		GPT_SetInputOperationMode(gpt_rec.base, gpt_rec.capture.channel, gpt_rec.capture.operation_mode);
	} else {
		rec->c.is_hw_recovery_mode = false;
	}

	if (imx_pll_init(&rec->c.pll) < 0) {
		os_log(LOG_ERR, "cannot init the audio PLL\n");
		goto err;
	}

	clock_dev->domain = DOMAIN_0;

	rec->c.pll_ref_freq = input_clk_rate / prescale;
	rec->c.pll_timer_clk_div = BOARD_GPT_clk_src_div(gpt_rec.base);

	// FIXME register 2 devices, a REC and a GEN
	clock_dev->type = REC;
	clock_dev->ts_src = TS_INTERNAL;
	clock_dev->start = &gpt_rec_start;
	clock_dev->stop = &gpt_rec_stop;
	clock_dev->reset = &gpt_rec_reset;
	clock_dev->clean = &mclock_rec_pll_common_clean_get;
	clock_dev->open = &gpt_rec_open;
	clock_dev->release = NULL;
	clock_dev->config = &mclock_rec_pll_common_config;

	if (mclock_rec_pll_init(rec) < 0) {
		os_log(LOG_ERR, "mclock_rec_pll_init error\n");
		goto err_rec_pll;
	}

	os_log(LOG_INIT, "GPT %s rec device(%p), clk input: %ld Hz, pll frequency %lu divided by %u, pll parent (osc) frequency %lu\n",
		gpt_rec.sw_sampling ? "Software" : "Hardware", clock_dev, input_clk_rate,
		imx_pll_get_rate(&rec->c.pll), rec->c.pll_timer_clk_div, rec->c.pll.parent_rate);

	return 0;

err_rec_pll:
	imx_pll_deinit(&rec->c.pll);

err:
	return -1;
}

__exit void gpt_rec_exit(void)
{
	mclock_rec_pll_exit(&gpt_rec.rec);
	imx_pll_deinit(&gpt_rec.rec.c.pll);
}
#else
__init int gpt_rec_init(void) { return -1; }
__exit void gpt_rec_exit(void) { }
#endif /* defined(CONFIG_GENAVB_TSN_AVTP) && (CFG_NUM_GPT > 0) && defined(BOARD_GPT_REC_BASE) */
