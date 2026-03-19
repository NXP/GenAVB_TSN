/*
 * Copyright 2014-2015 Freescale Semiconductor, Inc.
 * Copyright 2018-2020, 2022-2025 NXP
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "avtp.h"

#include "avbdrv.h"
#include "hw_timer.h"
#include "debugfs.h"
#include <linux/clk.h>
#include <linux/kthread.h>
#include <linux/math64.h>

#include "media_clock_rec_pll.h"

#include "media_clock_rec_pll_common.c"

static void mclock_rec_pll_deferred_adjust(struct mclock_rec_pll *rec)
{
	wake_up_process(rec->mcr_kthread);
}

static int mclock_rec_pll_gptp_compare_reload(struct mclock_rec_pll *rec, uint32_t next_ts)
{
	return fec_ptp_tc_reload(rec->c.dev.eth->fec_data, rec->fec_tc_id, ts_wa(next_ts));
}

static int mclock_rec_pll_gptp_compare_start(struct mclock_rec_pll *rec, uint32_t ts_0, uint32_t ts_1)
{
	return fec_ptp_tc_start(rec->c.dev.eth->fec_data, rec->fec_tc_id, ts_wa(ts_0), ts_wa(ts_1), FEC_TMODE_TOGGLE);
}

static void mclock_rec_pll_gptp_stop(struct mclock_rec_pll *rec)
{
	fec_ptp_tc_stop(rec->c.dev.eth->fec_data, rec->fec_tc_id);
}

static int mclock_rec_pll_gptp_read(struct mclock_rec_pll *rec, uint32_t *now)
{
	return fec_ptp_read_cnt(rec->c.dev.eth->fec_data, now);
}

static unsigned int mclock_shared_mem_read(struct mclock_dev *dev, unsigned int idx)
{
	return mclock_shmem_read(dev, idx);

}

static int __mclock_rec_pll_start(struct mclock_rec_pll *rec, struct mclock_start *start)
{
	unsigned int ki_factor = DEFAULT_REC_PI_KI_FACTOR;
	unsigned int kp_factor = DEFAULT_REC_PI_KP_FACTOR;

	if (rec->req_ki_factor >= 0) {
		ki_factor = rec->req_ki_factor;

		pr_info("Using custom Ki (1/2^%u) for recovery PI controller\n",
			 ki_factor);

		rec->req_ki_factor = -1;
	}

	if (rec->req_kp_factor >= 0) {
		kp_factor = rec->req_kp_factor;

		pr_info("Using custom Kp (1/2^%u) for recovery PI controller\n",
			 kp_factor);

		rec->req_kp_factor = -1;
	}

	return mclock_rec_pll_common_start(&rec->c, start, ki_factor, kp_factor);
}

int mclock_rec_pll_start(struct mclock_rec_pll *rec, struct mclock_start *start)
{
	struct mclock_dev *dev = &rec->c.dev;
	struct eth_avb *eth = dev->eth;
	unsigned long flags;
	int rc = 0;

	raw_spin_lock_irqsave(&eth->lock, flags);

	if (!(eth->flags & PORT_FLAGS_ENABLED)) {
		rc = -EIO;
		rec->stats.err_start_port_down++;
		rec->c.state = RESET;
		goto out;
	}

	rc = __mclock_rec_pll_start(rec, start);

	if (!rc && (dev->flags & MCLOCK_FLAGS_WAKE_UP))
		mclock_wake_up_init(dev, dev->clk_timer);

out:
	raw_spin_unlock_irqrestore(&eth->lock, flags);
	return rc;
}

static void __mclock_rec_pll_stop(struct mclock_rec_pll *rec)
{
	mclock_rec_pll_common_stop(&rec->c);
}

int mclock_rec_pll_stop(struct mclock_rec_pll *rec)
{
	struct eth_avb *eth = rec->c.dev.eth;
	unsigned long flags;
	int rc = 0;

	raw_spin_lock_irqsave(&eth->lock, flags);

	if (!(eth->flags & PORT_FLAGS_ENABLED)) {
		rec->stats.err_stop_port_down++;
		rc = -EIO;
		goto out;
	}

	mclock_rec_pll_common_stop(&rec->c);

out:
	raw_spin_unlock_irqrestore(&eth->lock, flags);
	return rc;
}

void mclock_rec_pll_reset(struct mclock_rec_pll *rec)
{
	struct mclock_rec_pll_common *rec_common = &rec->c;

	/* Restart FEC sampling clock */
	__mclock_rec_pll_stop(rec);
	__mclock_rec_pll_start(rec, NULL);
	rec_common->stats.reset++;
}

static int mcr_handler_kthread(void *data)
{
	struct mclock_rec_pll *rec = data;
	int ppb;

	set_current_state(TASK_INTERRUPTIBLE);

	while (1) {
		schedule();

		if (kthread_should_stop())
			break;

		set_current_state(TASK_INTERRUPTIBLE);

		ppb = rec->c.req_ppb_adjust;

		/* current_rate variable is only used either for tracing or in sw based mcr:
		 * Avoid doing unnecessary costly calls to SCU.
		 */
		__mclock_rec_pll_common_adjust(&rec->c, ppb, false);

#if MCLOCK_PLL_REC_TRACE
		/* Update the trace with the configured adjustment and rate if needed */
		__mclock_rec_pll_common_trace_override(&rec->c);
#endif
	}

	return 0;
}

int mclock_rec_pll_init(struct mclock_rec_pll *rec, unsigned int eth_port)
{
	struct mclock_dev *dev = &rec->c.dev;
	struct avb_drv *avb;
	int rc = 0;

	mclock_rec_pll_common_init(&rec->c);

	dev->sh_mem = (void *)__get_free_page(GFP_KERNEL);
	if (!dev->sh_mem) {
		rc = -ENOMEM;
		pr_err("%s : array allocation failed\n", __func__);
		goto exit;
	}

	dev->w_idx = (unsigned int *)((char *)dev->sh_mem + MCLOCK_REC_BUF_SIZE);
	dev->mmap_size = MCLOCK_REC_MMAP_SIZE;
	dev->num_ts = MCLOCK_REC_NUM_TS;
	dev->timer_period = HW_TIMER_PERIOD_NS;

	rec->mcr_kthread = kthread_run(&mcr_handler_kthread, rec, "mcr handler");
	if (IS_ERR(rec->mcr_kthread)) {
		pr_err("%s: kthread_create() failed\n", __func__);
		rc = -EINVAL;
		goto exit;
	}

	mclock_drv_register_device(dev);

	/* After mclock device registration, bind the eth device to it */
	avb = container_of(dev->drv, struct avb_drv, mclock_drv);
	dev->eth = &avb->eth[eth_port];

	rec->req_ki_factor = -1;
	rec->req_kp_factor = -1;

	mclock_rec_pll_debugfs_init(dev->drv, rec, rec->c.dev.domain);

exit:
	return rc;
}

void mclock_rec_pll_exit(struct mclock_rec_pll *rec)
{
	struct mclock_dev *dev = &rec->c.dev;

	kthread_stop(rec->mcr_kthread);

	mclock_drv_unregister_device(dev);

	free_page((unsigned long)dev->sh_mem);
}
