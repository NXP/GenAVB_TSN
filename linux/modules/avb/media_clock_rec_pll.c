/*
 * AVB media clock recovery
 * Copyright 2014-2015 Freescale Semiconductor, Inc.
 * Copyright 2018, 2023 NXP
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "avtp.h"

#include "avbdrv.h"
#include "hw_timer.h"
#include "debugfs.h"
#include <linux/clk.h>
#include <linux/kthread.h>

#include "media_clock_rec_pll.h"

int mclock_rec_pll_clean_get(struct mclock_dev *dev, struct mclock_clean *clean)
{
	int rc = 0;
	struct mclock_rec_pll *rec = container_of(dev, struct mclock_rec_pll, dev);

	clean->nb_clean = atomic_xchg(&rec->ts_read, 0);
	clean->status = atomic_read(&rec->status);

	return rc;
}

/**
 * mclock_rec_pll_get_next_ts() - get next ts to program in ENET time compare
 * @rec: pointer to the PLL media clock recovery context
 * @next_ts: pointer to the timestamp
 * Return : 0 on success, negative otherwise
 *
 */
int mclock_rec_pll_ts_get(struct mclock_rec_pll *rec, unsigned int *next_ts)
{
	unsigned int _next_ts = 0;

	if (rec->dev.ts_src == TS_INTERNAL) {
		rational_add(&rec->fec_next_ts, &rec->fec_next_ts, &rec->fec_period);
		*next_ts = rec->fec_next_ts.i;
	}
	else {
		unsigned int w_idx = *rec->dev.w_idx;
		unsigned int n_read = 0;

		while (rec->r_idx != w_idx)
		{
			if (++rec->ts_slot == rec->div) {
				_next_ts = mclock_shmem_read(&rec->dev, rec->r_idx);
				rec->ts_slot = 0;
			}

			mclock_rec_pll_inc_r_idx(rec);
			n_read++;

			if (_next_ts) {
				*next_ts = _next_ts + rec->ts_offset;
				break;
			}
		}

		atomic_add(n_read, &rec->ts_read);

		if (!_next_ts)
			return -1;
	}
	return 0;
}

int mclock_rec_pll_ts_reset(struct mclock_rec_pll *rec, unsigned int *next_ts)
{
	unsigned int _next_ts = 0;
	unsigned int n_read = 0;
	unsigned int w_idx = *rec->dev.w_idx;

	/* Clean and get last ts */
	while (rec->r_idx != w_idx) {
		mclock_rec_pll_inc_r_idx(rec);
		n_read++;
	}

	atomic_add(n_read, &rec->ts_read);

	rec->ts_slot = 0;
	_next_ts = mclock_shmem_read(&rec->dev, rec->r_idx - 1);

	if (!_next_ts)
		return -1;
	else {
		*next_ts = _next_ts + rec->ts_offset;
		return 0;
	}
}

static int mclock_rec_pll_check_ts_freq(struct mclock_rec_pll *rec, unsigned int ts_freq_p,
			unsigned int ts_freq_q)
{
	u64 ratio = (u64)rec->pll_ref_freq * ts_freq_q;

	/* ts_freq must be a divider of pll frequency (e.g ratio needs to be an integer)
	 * This limitation comes from the codec (generally configured to operate with 48Khz family)
	 * and not from the internal recovery mechanism ready to work with any frequency as it is using
	 * rationals*/


	if (do_div(ratio, ts_freq_p)) {
		pr_err("%s: ts frequency %u / %u is not a divider of PLL freq %u \n",
			 __func__, ts_freq_p, ts_freq_q, rec->pll_ref_freq);
		return -1;
	}

	return 0;
}

/*This function will set the sampling frequency and check if the chosen ts frequency is acceptable*/
static int mclock_rec_pll_configure_freqs(struct mclock_rec_pll *rec, mclock_ts_src_t ts_src,
						unsigned int ts_freq_p, unsigned int ts_freq_q)
{
	unsigned int div = 1;

	if (ts_src == TS_INTERNAL) {
		rational_init(&rec->fec_period, MCLOCK_PLL_SAMPLING_PERIOD_NS, 1);
		ts_freq_p = MCLOCK_PLL_SAMPLING_FREQ;
		ts_freq_q = 1;
	}
	else {
		struct rational ts_period;

		if (!ts_freq_p || !ts_freq_q) {
			pr_err("%s : invalid ts frequency: %u / %u Hz\n", __func__, ts_freq_p, ts_freq_q);
			return -1;
		}

		rational_init(&ts_period, (unsigned long long) NSEC_PER_SEC * ts_freq_q, ts_freq_p);
		rec->fec_period = ts_period;

		if ((ts_period.i > MCLOCK_PLL_SAMPLING_PERIOD_NS)
		|| ((2 * MCLOCK_PLL_SAMPLING_PERIOD_NS / ts_period.i) >= rec->dev.num_ts)) {
			pr_err("%s : invalid ts frequency: %u / %u Hz\n", __func__, ts_freq_p, ts_freq_q);
			return -1;
		}

		/* Divide ts input frequency until periode is greater than samplinq period (10 ms) */
		while (rec->fec_period.i < MCLOCK_PLL_SAMPLING_PERIOD_NS) {
			rational_add(&rec->fec_period, &rec->fec_period, &ts_period);
			div++;
		}

		rec->div = div;
	}

	if (mclock_rec_pll_check_ts_freq(rec, ts_freq_p, ts_freq_q) < 0)
		return -1;

	rational_init(&rec->pll_clk_period, (unsigned long long)rec->pll_ref_freq * ts_freq_q * div, ts_freq_p);
	rec->dev.drift_period = rec->fec_period.i;

	//pr_info("%s : fec sampling period: %u + %u/%u, pll period / fec period: %u + %u/%u, div: %d\n",
	//	__func__, rec->fec_period.i, rec->fec_period.p, rec->fec_period.q,
	//	rec->pll_clk_period.i, rec->pll_clk_period.p, rec->pll_clk_period.q, div);

	return 0;
}

int __mclock_rec_pll_start(struct mclock_rec_pll *rec, struct mclock_start *start)
{
	struct eth_avb *eth = rec->dev.eth;
	struct mclock_dev *dev = &rec->dev;
	u32 now, ts_0, ts_1;
	int rc = 0;

	if (fec_ptp_read_cnt(eth->fec_data, &now) < 0) {
		rc = -EIO;
		rec->stats.err_fec++;
		goto out;
	}

	if (rec->dev.ts_src == TS_INTERNAL) {
		ts_0 = ((now / rec->fec_period.i) + 2) * rec->fec_period.i;
		ts_1 = ts_0 + rec->fec_period.i;
	}
	else {
		/* user-space start, use provided timestamps */
		if (start) {
			rec->r_idx = 0;
			rec->ts_slot = 0;
			rec->ts_offset = 2 * rec->fec_period.i;
			atomic_set(&rec->ts_read, 0);

			/* Simple case, we can use ts_0 and ts_1, the next ts
			 * in array will be continuous
			 */
			if (rec->div == 1) {
				ts_0 = start->ts_0 + rec->ts_offset;
				ts_1 = start->ts_1 + rec->ts_offset;
			}
			/* If frequency is divided, it's not possible to use ts_1 as-is,
			 * generate it with theoric sampling period and adjust offset
			 * so that next read ts in array will be continuous.
			 */
			else {
				ts_0 = start->ts_0 + rec->ts_offset / 2;
				ts_1 = ts_0 + rec->fec_period.i;
				rec->ts_slot++;
			}
		}
		/* internal reset */
		else {
			/* Get last ts in array for ts_1 and generate ts_0 */
			if (mclock_rec_pll_ts_reset(rec, &ts_1) < 0) {
				rec->stats.err_ts++;
				rc = -1;
				goto out;
			}

			ts_0 = ts_1 - rec->fec_period.i;
		}
	}

	rational_init(&rec->fec_next_ts, ts_1, 1);

	if (rec->is_hw_recovery_mode) {
		/* Start HW sampling ENET clock */
		rc = fec_ptp_tc_start(eth->fec_data, rec->fec_tc_id, ts_wa(ts_0), ts_wa(ts_1), FEC_TMODE_TOGGLE);
		if (rc < 0) {
			rec->stats.err_fec++;
			goto out;
		}
	} else {
		/* Start SW sampling on ts_1. */
		rec->next_ts = ts_1;
	}

	rec->state = START;

	/* This function can be called in hard interrupt context, avoid calls to SCU API (sleeping) functions from here.
	 * The current_rate value is used for trace and in sw recovery calculation and should be updated on every
	 * ppb adjustement anyway. Using a cached value (either from previous adjustments or imx_pll_init()) as initial value should be ok.
	 */
	if (!rec->pll_scu_controlled)
		rec->pll.current_rate = imx_pll_get_rate(&rec->pll);

	/* Initial control output is 0 (e.g 0 ppb variation) */
	pi_reset(&rec->pi, 0);
	rational_init(&rec->pll_clk_target, 0, 1);
	rec->pll_clk_meas = 0;
	rec->meas = 0;
	rec->start_ppb_err = 0;
	rec->accepted_ppb_err_nb = 0;
	mclock_rec_pll_wd_reset(rec);
	/* Set the status to running */
	atomic_set(&rec->status, MCLOCK_RUNNING);
	dev->flags &= ~MCLOCK_FLAGS_RUNNING_LOCKED;

	rec->stats.err_per_sec = 0;
	rec->stats.err_cum = 0;
	rec->stats.err_time = 0;
	rec->stats.locked_state = 0;

out:
	return rc;
}

int mclock_rec_pll_start(struct mclock_rec_pll *rec, struct mclock_start *start)
{
	struct mclock_dev *dev = &rec->dev;
	struct eth_avb *eth = dev->eth;
	unsigned long flags;
	int rc = 0;

	raw_spin_lock_irqsave(&eth->lock, flags);

	rec->stats.start++;

#if MCLOCK_PLL_REC_TRACE
	rec->trace_freeze = 0;
	rec->trace_count = 0;
#endif

	if (!(eth->flags & PORT_FLAGS_ENABLED)) {
		rc = -EIO;
		rec->stats.err_port_down++;
		rec->state = RESET;
		goto out;
	}

	rc = __mclock_rec_pll_start(rec, start);

	if (!rc && (dev->flags & MCLOCK_FLAGS_WAKE_UP))
		mclock_wake_up_init(dev, dev->clk_timer);

out:
	raw_spin_unlock_irqrestore(&eth->lock, flags);
	return rc;
}

void __mclock_rec_pll_stop(struct mclock_rec_pll *rec)
{
	struct eth_avb *eth = rec->dev.eth;
	struct mclock_dev *dev = &rec->dev;

	if (rec->is_hw_recovery_mode)
		fec_ptp_tc_stop(eth->fec_data, rec->fec_tc_id);

	atomic_set(&rec->status, MCLOCK_STOPPED);
	dev->flags &= ~MCLOCK_FLAGS_RUNNING_LOCKED;
}

int mclock_rec_pll_stop(struct mclock_rec_pll *rec)
{
	struct eth_avb *eth = rec->dev.eth;
	unsigned long flags;
	int rc = 0;

	raw_spin_lock_irqsave(&eth->lock, flags);

	rec->stats.stop++;

	if (!(eth->flags & PORT_FLAGS_ENABLED)) {
		rec->stats.err_port_down++;
		rc = -EIO;
		goto out;
	}

	__mclock_rec_pll_stop(rec);

out:
	raw_spin_unlock_irqrestore(&eth->lock, flags);
	return rc;
}

void mclock_rec_pll_reset(struct mclock_rec_pll *rec)
{
	/* Restart FEC sampling clock */
	__mclock_rec_pll_stop(rec);
	__mclock_rec_pll_start(rec, NULL);
	rec->stats.reset++;
}

static int __mclock_rec_pll_adjust(struct mclock_rec_pll *rec, int ppb_adjust, bool get_rate)
{
	struct imx_pll *pll = &rec->pll;
	int rc;

	rc = imx_pll_adjust(pll, &ppb_adjust);

	if (rc == IMX_CLK_PLL_SUCCESS) {
		/* Save the returned (exact) pbb adjust */
		pll->ppb_adjust = ppb_adjust;
		if (get_rate)
			pll->current_rate = imx_pll_get_rate(pll);
	} else if (rc == -IMX_CLK_PLL_PREC_ERR) {
		rec->stats.err_pll_prec++;
	} else {
		rec->stats.err_set_pll_rate++;
	}

	return rc;
}

static void mclock_rec_pll_adjust(struct mclock_rec_pll *rec, int err)
{
	int adjust_val;
	int last_req_ppb, new_ppb_adjust;
	struct imx_pll *pll = &rec->pll;

	pi_update(&rec->pi, err);

	/* Save the last ppb adjustement. */
	last_req_ppb = rec->req_ppb_adjust;

	/* Anti wind-up */
	adjust_val = (rec->pi.u) - pll->ppb_adjust;

#if MCLOCK_PLL_REC_TRACE
	if (!rec->trace_freeze) {
		rec->trace[rec->trace_count].pi_err_input = err;
		rec->trace[rec->trace_count].pi_control_output = rec->pi.u;
	}
#endif

	if (adjust_val > rec->max_adjust)
		adjust_val = rec->max_adjust;
	else if (adjust_val < (-rec->max_adjust))
		adjust_val = (-rec->max_adjust);

	new_ppb_adjust = pll->ppb_adjust + adjust_val;

#if MCLOCK_PLL_REC_TRACE
	if (!rec->trace_freeze)
		rec->trace[rec->trace_count].previous_rate = pll->current_rate;
#endif

	/* Check if we really need to update the PLL settings */
	if (last_req_ppb == new_ppb_adjust)
		goto no_adjust;

	/* Save the requested ppb adjust*/
	rec->req_ppb_adjust = new_ppb_adjust;

	/* SCU api functions can not be called from interrupt context, defer work to kernel thread */
	if (!rec->pll_scu_controlled) {
		__mclock_rec_pll_adjust(rec, new_ppb_adjust, true);
	} else {
#if MCLOCK_PLL_REC_TRACE
		if (!rec->trace_freeze) {
			/* ppb adjustment values will be overriden on deferred kernel thread execution. */
			rec->override_trace_idx = rec->trace_count;
			rec->trace[rec->trace_count].adjust_value = 0;
			rec->trace[rec->trace_count].new_rate = 0;
		}
#endif

		wake_up_process(rec->mcr_kthread);
	}

no_adjust:

#if MCLOCK_PLL_REC_TRACE
	if (!rec->trace_freeze && !rec->pll_scu_controlled) {
		rec->trace[rec->trace_count].adjust_value = pll->ppb_adjust;
		rec->trace[rec->trace_count].new_rate = pll->current_rate;
	}
#endif
	rec->stats.last_app_adjust = pll->ppb_adjust;
	rec->stats.adjust++;
}


/**
 * mclock_rec_pll_sw_timer_irq() - Handles the software sampling recovery pll interrupt
 * @rec: pointer to the PLL media clock recovery context
 * @audio_pll_cnt: value of audio pll counter
 * @ptp_now: value of ptp counter on audio pll sampling (must be a 32 bits value)
 * @ticks: number of hw timer ticks
 * Return: 0 on success, negative otherwise.
 */
int mclock_rec_pll_sw_sampling_irq(struct mclock_rec_pll *rec, u32 audio_pll_cnt, u32 ptp_now, unsigned int ticks)
{
	u32 pll_latency_cycles, ptp_ts_latency_ns;
	int rc;

	/* check if sampling period has passed: extrapolation distance of hw timer period */
	if (avtp_after_eq(ptp_now, rec->next_ts)) {

		/* How late is the ptp_now to the timestamp. */
		ptp_ts_latency_ns = abs((s32)(ptp_now - rec->next_ts));
		/* Extrapolate the latency to pll cycles and estimate the pll counter at timestamp:
		 * Use the real pll counter rate (timer clk rate) after pll adjustement.
		 */
		pll_latency_cycles = (u32)div_u64((u64)ptp_ts_latency_ns * rec->pll.current_rate, 1000000000ULL);
		pll_latency_cycles /= rec->pll_timer_clk_div;

		audio_pll_cnt -= pll_latency_cycles;

		/* Pass the audio pll measurement (over the sampling period) to the recovery mechanism */
		rc = mclock_rec_pll_timer_irq(rec, 1, (u32)(audio_pll_cnt - rec->audio_pll_cnt_last), ticks);
		rec->audio_pll_cnt_last = audio_pll_cnt;

	} else {
		rc = mclock_rec_pll_timer_irq(rec, 0, 0, ticks);
	}

	return rc;
}

/**
 * mclock_rec_pll_timer_irq() - Handles the recovery pll timer interrupt
 * @rec: pointer to the PLL media clock recovery context
 * @fec_event: 1 if fec capture event is triggered (timestamp matched the gPTP counter), 0 otherwise
 * @meas: audio PLL meas
 * @ticks: number of hw timer ticks
 * Return: 0 on success, negative otherwise.
 */
int mclock_rec_pll_timer_irq(struct mclock_rec_pll *rec, int fec_event, unsigned int meas, unsigned int ticks)
{
	struct mclock_dev *dev = &rec->dev;
	struct eth_avb *eth = dev->eth;
	unsigned int pll_clk_last, pll_clk_meas_last, dt_pll_clk_target;
	unsigned int next_ts;
	int err;
	s64 err_ppb;
	int rc = 0;

	dev->clk_timer += ticks * dev->timer_period;

	if (!(eth->flags & PORT_FLAGS_ENABLED)) {
		rec->stats.err_port_down++;
		goto out_no_trace;
	}

	/* No capture event */
	if (!fec_event) {
		if (avtp_after(dev->clk_timer, rec->wd)) {
			rec->state = RESET;
			rec->stats.err_wd++;
		}
		goto out_no_trace;
	}

#if MCLOCK_PLL_REC_TRACE
	if (!rec->trace_freeze) {
		rec->trace[rec->trace_count].meas = meas;
		rec->trace[rec->trace_count].ticks = ticks;
		rec->trace[rec->trace_count].err = 0;
		rec->trace[rec->trace_count].adjust_value = 0;
	}
#endif


	mclock_rec_pll_wd_reset(rec);

	if (mclock_rec_pll_ts_get(rec, &next_ts) < 0) {
		rec->stats.err_ts++;
		goto out;
	}

	if (rec->is_hw_recovery_mode) {
		/* Reload FEC timer */
		rc = fec_ptp_tc_reload(eth->fec_data, rec->fec_tc_id, ts_wa(next_ts));
		if (rc < 0) {
			/*
			 * Do not reset the logic here, we can have a spurious FEC output edge at startup.
			 * If the TS compare chain has not properly started, no further events will happen
			 * and this will be caught by the timer watchdog.
			 */
			rec->stats.err_fec++;
			goto out;
		}
	} else {
		/* Save the next sw sampling timestamp. */
		rec->next_ts = next_ts;
	}

	/* Check that measurement is fine */
	if (abs(rec->pll_clk_period.i - meas) >  (rec->pll_clk_period.i / 1000)) {
		/* First value is often wrong, skip it */
		if (rec->meas) {
			rec->state = RESET;
			rc = -1;
		}
		rec->stats.err_meas++;
		rec->meas++;
		goto out;
	}

	pll_clk_last = rec->pll_clk_target.i;
	pll_clk_meas_last = rec->pll_clk_meas;

	rational_add(&rec->pll_clk_target, &rec->pll_clk_target, &rec->pll_clk_period);
	/* Expected pll clock ticks. */
	dt_pll_clk_target = rec->pll_clk_target.i - pll_clk_last;
	rec->pll_clk_meas += meas;

	/* err over a sampling period */
	err = dt_pll_clk_target - (rec->pll_clk_meas - pll_clk_meas_last);

	/* make the error in ppb (divide by the sampling period) */
	err_ppb = div_s64(err * 1000000000LL, dt_pll_clk_target);

	/* err stats */
	rec->stats.err_time += rec->fec_period.i;
	rec->stats.err_cum += err;

	if (rec->stats.err_time >= NSEC_PER_SEC) {
		rec->stats.err_per_sec = rec->stats.err_cum;
		rec->stats.err_cum = 0;
		rec->stats.err_time = 0;
	}

#if MCLOCK_PLL_REC_TRACE
	if (!rec->trace_freeze) {
		rec->trace[rec->trace_count].meas = meas;
		rec->trace[rec->trace_count].ticks = ticks;
		rec->trace[rec->trace_count].err = err;
		rec->trace[rec->trace_count].pi_err_input = 0;
		rec->trace[rec->trace_count].pi_control_output = 0;
		rec->trace[rec->trace_count].new_rate = 0;
		rec->trace[rec->trace_count].previous_rate = 0;
		rec->trace[rec->trace_count].adjust_value = 0;
	}
#endif

	switch (rec->state) {
	case START:
		if (rec->meas > MCLOCK_REC_PLL_NB_MEAS_START_SKIP) { /* Skip the first values to have a proper average */
			rec->start_ppb_err += err_ppb;
		}

		if(rec->meas++ >= (MCLOCK_REC_PLL_NB_MEAS + MCLOCK_REC_PLL_NB_MEAS_START_SKIP)) {
			rec->state = ADJUST;
			rational_init(&rec->clk_media, dev->clk_timer , 1);
			/* Divide the accumulated error by the start measurement window
			 * to get an estimation of the initial drift.
			 */
			rec->start_ppb_err = div_s64(rec->start_ppb_err, MCLOCK_REC_PLL_NB_MEAS);

			/* PI reset should take into account the measured
			 * ppb error and the previously set ppb adjust
			 */
			pi_reset(&rec->pi, rec->start_ppb_err + rec->pll.ppb_adjust);

		}
		break;
	case ADJUST:
		if (abs(err_ppb) < MCLOCK_REC_PLL_IN_LOCKED_PPB_ERR) {
			/* After few consecutive measurements under MCLOCK_REC_PLL_IN_LOCKED_PPB_ERR, we can go to locked */
			if (++rec->accepted_ppb_err_nb >= MCLOCK_REC_PLL_IN_LOCKED_NB_VALID_PPB_ERR) {
				/* Declare clock domain locked. */
				if (!(dev->flags & MCLOCK_FLAGS_RUNNING_LOCKED)) {
					atomic_set(&rec->status, MCLOCK_RUNNING_LOCKED);
					dev->flags |= MCLOCK_FLAGS_RUNNING_LOCKED;
				}
				/* Go to ADJUST_LOCKED state with larger sampling period to reduce jitter. */
				rec->state = ADJUST_LOCKED;
				rec->stats.locked_state++;
			}
		} else
			rec->accepted_ppb_err_nb = 0;

		/* Do PLL adjustement on every sampling measure */
		mclock_rec_pll_adjust(rec, err_ppb);

		rec->locked_meas = 0;
		rec->locked_ppb_err = 0;
		fallthrough;
	case ADJUST_LOCKED:
		/* Do PLL adjustement every 2^MCLOCK_REC_PLL_ADJUST_LOCKED_SAMPLING_SHIFT measurement. */
		rec->locked_ppb_err += err_ppb;
		if (++rec->locked_meas >= (1 << MCLOCK_REC_PLL_ADJUST_LOCKED_SAMPLING_SHIFT)) {

			/* Scale the accumulated error measurements to the sampling window. */
			rec->locked_ppb_err >>= MCLOCK_REC_PLL_ADJUST_LOCKED_SAMPLING_SHIFT;

			mclock_rec_pll_adjust(rec, rec->locked_ppb_err);

			/* If error is larger than MCLOCK_REC_PLL_OUT_LOCKED_PPB_ERR, go back to ADJUST state for quicker adjustments. */
			if (abs(rec->locked_ppb_err) >= MCLOCK_REC_PLL_OUT_LOCKED_PPB_ERR) {
				rec->accepted_ppb_err_nb = 0;
				rec->state = ADJUST;
			}

			rec->locked_meas = 0;
			rec->locked_ppb_err = 0;
		}

		rational_add(&rec->clk_media, &rec->clk_media, &rec->fec_period);

		/* Not needed to reset here there are enough checks before*/
		if (mclock_drift_adapt(&rec->dev, rec->clk_media.i) < 0)
			rec->stats.err_drift++;

		break;
	default:
		break;
	}

out:
#if MCLOCK_PLL_REC_TRACE
	if (!rec->trace_freeze) {
		rec->trace[rec->trace_count].state = rec->state;
		if (++rec->trace_count > MCLOCK_REC_TRACE_SIZE - 1)
		       rec->trace_freeze = 1;
	}
#endif

out_no_trace:

	if (rec->state == RESET)
		mclock_rec_pll_reset(rec);

	return rc;
}

int mclock_rec_pll_config(struct mclock_dev *dev, struct mclock_sconfig *cfg)
{
	int rc = 0;
	struct mclock_rec_pll *rec = container_of(dev, struct mclock_rec_pll, dev);

	switch (cfg->cmd) {
	case MCLOCK_CFG_TS_SRC:
		if (dev->flags & MCLOCK_FLAGS_RUNNING) {
			rc = -EBUSY;
			goto exit;
		}

		if (mclock_rec_pll_configure_freqs(rec, cfg->ts_src, dev->ts_freq_p, dev->ts_freq_q) < 0) {
			rc = -EINVAL;
			pr_err("%s : mclock_rec_pll_configure_freqs error\n", __func__);
			goto exit;
		}

		dev->ts_src = cfg->ts_src;
		break;

	case MCLOCK_CFG_FREQ:
		if (dev->flags & MCLOCK_FLAGS_RUNNING) {
			rc = -EBUSY;
			goto exit;
		}

		if (dev->ts_src == TS_INTERNAL) {
			rc = -EINVAL;
			pr_err("%s : cannot set ts freq in internal mode\n", __func__);
			goto exit;
		}
		else if (dev->ts_src == TS_EXTERNAL) {

			if (mclock_rec_pll_configure_freqs(rec, dev->ts_src, cfg->ts_freq.p, cfg->ts_freq.q) < 0) {
				rc = -EINVAL;
				pr_err("%s : mclock_rec_pll_configure_freqs error\n", __func__);
				goto exit;

			}
			mclock_set_ts_freq(dev, cfg->ts_freq.p, cfg->ts_freq.q);
		}
		else
			rc = -EINVAL;

		break;

	default:
		rc = -EINVAL;
		break;
	}

exit:
	return rc;
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

		ppb = rec->req_ppb_adjust;

		/* current_rate variable is only used either for tracing or in sw based mcr:
		 * Avoid doing unnecessary costly calls to SCU.
		 */
		__mclock_rec_pll_adjust(rec, ppb, false);

#if MCLOCK_PLL_REC_TRACE
		if (!rec->trace_freeze) {
			struct imx_pll *pll = &rec->pll;

			rec->trace[rec->override_trace_idx].adjust_value = pll->ppb_adjust;
			rec->trace[rec->override_trace_idx].new_rate = pll->current_rate;
		}
#endif
	}

	return 0;
}

int mclock_rec_pll_init(struct mclock_rec_pll *rec)
{
	struct mclock_dev *dev = &rec->dev;
	int rc = 0;

	mclock_rec_pll_configure_freqs(rec, TS_INTERNAL, 0, 0);

	/* For rec_pll, the ts freq holds value for the external ts
	  Set the default value here */
	mclock_set_ts_freq(&rec->dev, MCLOCK_REC_TS_FREQ_INIT, 1);

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
#if MCLOCK_PLL_REC_TRACE
	rec->trace_count = 0;
	rec->trace_freeze = 0;
#endif

	rec->mcr_kthread = kthread_run(mcr_handler_kthread, rec, "mcr handler");
	if (IS_ERR(rec->mcr_kthread)) {
		pr_err("%s: kthread_create() failed\n", __func__);
		rc = -EINVAL;
		goto exit;
	}

	mclock_drv_register_device(dev);

	mclock_rec_pll_debugfs_init(dev->drv, rec, rec->dev.domain);

exit:
	return rc;
}

void mclock_rec_pll_exit(struct mclock_rec_pll *rec)
{
	kthread_stop(rec->mcr_kthread);

	mclock_drv_unregister_device(&rec->dev);

	free_page((unsigned long)rec->dev.sh_mem);
}
