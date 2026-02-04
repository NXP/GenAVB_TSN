/*
 * Common AVB media clock recovery
 * Copyright 2014-2015 Freescale Semiconductor, Inc.
 * Copyright 2018, 2023-2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "media_clock_rec_pll_common.h"

/* Implemented by os specific layer and used here */
static void mclock_rec_pll_deferred_adjust(struct mclock_rec_pll *rec);
static int mclock_rec_pll_gptp_compare_reload(struct mclock_rec_pll *rec, uint32_t next_ts);
static int mclock_rec_pll_gptp_compare_start(struct mclock_rec_pll *rec, uint32_t ts_0, uint32_t ts_1);
static void mclock_rec_pll_gptp_stop(struct mclock_rec_pll *rec);
static int mclock_rec_pll_gptp_read(struct mclock_rec_pll *rec, uint32_t *now);
static unsigned int mclock_shared_mem_read(struct mclock_dev *dev, unsigned int idx);

static inline void mclock_rec_pll_common_wd_reset(struct mclock_rec_pll_common *rec)
{
	rec->wd = rec->dev.clk_timer + 10 * rec->gptp_period.i;
}

static inline void mclock_rec_pll_common_inc_r_idx(struct mclock_rec_pll_common *rec)
{
	rec->r_idx = (rec->r_idx + 1) & (rec->dev.num_ts - 1);
}

int mclock_rec_pll_common_clean_get(struct mclock_dev *dev, struct mclock_clean *clean)
{
	struct mclock_rec_pll_common *rec = container_of(dev, struct mclock_rec_pll_common, dev);
	int rc = 0;

	clean->nb_clean = common_os_atomic_xchg(&rec->ts_read, 0);
	clean->status = common_os_atomic_read(&rec->status);

	return rc;
}

/**
 * mclock_rec_pll_common_ts_get() - get next ts to program in ENET time compare
 * @rec: pointer to the PLL media clock recovery context
 * @next_ts: pointer to the timestamp
 * Return : 0 on success, negative otherwise
 *
 */
static int mclock_rec_pll_common_ts_get(struct mclock_rec_pll_common *rec, unsigned int *next_ts)
{
	unsigned int _next_ts = 0;

	if (rec->dev.ts_src == TS_INTERNAL) {
		rational_add(&rec->gptp_next_ts, &rec->gptp_next_ts, &rec->gptp_period);
		*next_ts = rec->gptp_next_ts.i;
	}
	else {
		unsigned int w_idx = *rec->dev.w_idx;
		unsigned int n_read = 0;

		while (rec->r_idx != w_idx)
		{
			if (++rec->ts_slot == rec->div) {
				_next_ts = mclock_shared_mem_read(&rec->dev, rec->r_idx);
				rec->ts_slot = 0;
			}

			mclock_rec_pll_common_inc_r_idx(rec);
			n_read++;

			if (_next_ts) {
				*next_ts = _next_ts + rec->ts_offset;
				break;
			}
		}

		common_os_atomic_add(n_read, &rec->ts_read);

		if (!_next_ts)
			return -1;
	}
	return 0;
}

static int mclock_rec_pll_common_ts_reset(struct mclock_rec_pll_common *rec, unsigned int *next_ts)
{
	unsigned int w_idx = *rec->dev.w_idx;
	unsigned int _next_ts = 0;
	unsigned int n_read = 0;

	/* Clean and get last ts */
	while (rec->r_idx != w_idx) {
		mclock_rec_pll_common_inc_r_idx(rec);
		n_read++;
	}

	common_os_atomic_add(n_read, &rec->ts_read);

	rec->ts_slot = 0;
	_next_ts = mclock_shared_mem_read(&rec->dev, rec->r_idx - 1);

	if (!_next_ts) {
		return -1;
	} else {
		*next_ts = _next_ts + rec->ts_offset;
		return 0;
	}
}

static int mclock_rec_pll_common_check_ts_freq(struct mclock_rec_pll_common *rec, unsigned int ts_freq_p,
			unsigned int ts_freq_q)
{
	u64 ratio = (u64)rec->pll_ref_freq * ts_freq_q;

	/* ts_freq must be a divider of pll frequency (e.g ratio needs to be an integer)
	 * This limitation comes from the codec (generally configured to operate with 48Khz family)
	 */

	if (common_os_do_div(ratio, ts_freq_p)) {
		common_os_log_err("%s: ts frequency %u / %u is not a divider of PLL freq %u \n",
			 __func__, ts_freq_p, ts_freq_q, rec->pll_ref_freq);
		return -1;
	}

	return 0;
}

/* This function will set the sampling frequency and check if the chosen ts frequency is acceptable */
static int mclock_rec_pll_common_configure_freqs(struct mclock_rec_pll_common *rec, mclock_ts_src_t ts_src,
						unsigned int ts_freq_p, unsigned int ts_freq_q)
{
	unsigned int div = 1;

	if (ts_src == TS_INTERNAL) {
		rational_init(&rec->gptp_period, MCLOCK_PLL_SAMPLING_PERIOD_NS, 1);
		ts_freq_p = MCLOCK_PLL_SAMPLING_FREQ;
		ts_freq_q = 1;
	}
	else {
		struct rational ts_period;

		if (!ts_freq_p || !ts_freq_q) {
			common_os_log_err("%s : invalid ts frequency: %u / %u Hz\n", __func__, ts_freq_p, ts_freq_q);
			return -1;
		}

		rational_init(&ts_period, (unsigned long long) NSEC_PER_SEC * ts_freq_q, ts_freq_p);
		rec->gptp_period = ts_period;

		if ((ts_period.i > MCLOCK_PLL_SAMPLING_PERIOD_NS)
		|| ((2 * MCLOCK_PLL_SAMPLING_PERIOD_NS / ts_period.i) >= rec->dev.num_ts)) {
			common_os_log_err("%s : invalid ts frequency: %u / %u Hz\n", __func__, ts_freq_p, ts_freq_q);
			return -1;
		}

		/* Divide ts input frequency until period is greater than sampling period (10 ms) */
		while (rec->gptp_period.i < MCLOCK_PLL_SAMPLING_PERIOD_NS) {
			rational_add(&rec->gptp_period, &rec->gptp_period, &ts_period);
			div++;
		}

		rec->div = div;
	}

	if (mclock_rec_pll_common_check_ts_freq(rec, ts_freq_p, ts_freq_q) < 0)
		return -1;

	rational_init(&rec->pll_clk_period, (unsigned long long)rec->pll_ref_freq * ts_freq_q * div, ts_freq_p);
	rec->dev.drift_period = rec->gptp_period.i;

	//common_os_log_info("%s : fec sampling period: %u + %u/%u, pll period / fec period: %u + %u/%u, div: %d\n",
	//	__func__, rec->gptp_period.i, rec->gptp_period.p, rec->gptp_period.q,
	//	rec->pll_clk_period.i, rec->pll_clk_period.p, rec->pll_clk_period.q, div);

	return 0;
}

static int __mclock_rec_pll_common_adjust(struct mclock_rec_pll_common *rec, int ppb_adjust, bool get_rate)
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

static void mclock_rec_pll_common_adjust(struct mclock_rec_pll_common *rec, int err, bool update_pi)
{
	struct imx_pll *pll = &rec->pll;
	int last_req_ppb, new_ppb_adjust;
	int adjust_val;

	/* If initial adjustement (after PI reset), do not update PI values.
	 * Just do the ppb adjustement.
	 */
	if (update_pi)
		pi_update(&rec->pi, err);

	/* Save the last ppb adjustement. */
	last_req_ppb = rec->req_ppb_adjust;

	/* Anti wind-up : PI output is ratio * 10^9 */
	adjust_val = (rec->pi.u - 1000000000LL) - pll->ppb_adjust;

#if MCLOCK_PLL_REC_TRACE
	if (!rec->trace_freeze) {
		rec->trace[rec->trace_count].pi_err_input = (update_pi) ? err : 0;
		rec->trace[rec->trace_count].pi_control_output = rec->pi.u;
	}
#endif

	/* Check the per step max adjust */
	if (adjust_val > rec->max_adjust)
		adjust_val = rec->max_adjust;
	else if (adjust_val < (-rec->max_adjust))
		adjust_val = (-rec->max_adjust);

	new_ppb_adjust = pll->ppb_adjust + adjust_val;

#define MAX_PLL_ADJUST_PPB	250000 //250 ppm

	/* Anti windup: check compared to allowed max absolute adjustement. */
	if (new_ppb_adjust > MAX_PLL_ADJUST_PPB)
		new_ppb_adjust = MAX_PLL_ADJUST_PPB;
	else if (new_ppb_adjust < (-MAX_PLL_ADJUST_PPB))
		new_ppb_adjust = (-MAX_PLL_ADJUST_PPB);

#if MCLOCK_PLL_REC_TRACE
	if (!rec->trace_freeze)
		rec->trace[rec->trace_count].previous_rate = pll->current_rate;
#endif

	/* Check if we really need to update the PLL settings */
	if (last_req_ppb == new_ppb_adjust)
		goto no_adjust;

	/* Save the requested ppb adjust*/
	rec->req_ppb_adjust = new_ppb_adjust;

	/* PLL api functions can not be called from interrupt context, defer work to dedicated thread */
	if (!rec->pll_deferred_adjust) {
		__mclock_rec_pll_common_adjust(rec, new_ppb_adjust, true);
	} else {
#if MCLOCK_PLL_REC_TRACE
		if (!rec->trace_freeze) {
			/* ppb adjustment values will be overriden on deferred kernel thread execution. */
			rec->override_trace_idx = rec->trace_count;
			rec->trace[rec->trace_count].adjust_value = 0;
			rec->trace[rec->trace_count].new_rate = 0;
		}
#endif
		mclock_rec_pll_deferred_adjust(mclock_rec_pll_common_to_rec(rec));
	}

no_adjust:

#if MCLOCK_PLL_REC_TRACE
	if (!rec->trace_freeze && !rec->pll_deferred_adjust) {
		rec->trace[rec->trace_count].adjust_value = pll->ppb_adjust;
		rec->trace[rec->trace_count].new_rate = pll->current_rate;
	}
#endif
	rec->stats.last_app_adjust = pll->ppb_adjust;
	rec->stats.adjust++;
}


/**
 * mclock_rec_pll_common_sw_sampling_irq() - Handles the software sampling recovery pll interrupt
 * @rec: pointer to PLL media clock recovery context
 * @audio_pll_cnt: value of audio pll counter
 * @ptp_now: value of ptp counter on audio pll sampling (must be a 32 bits value)
 * @ticks: number of hw timer ticks
 * Return: 0 on success, negative otherwise.
 */
int mclock_rec_pll_common_sw_sampling_irq(struct mclock_rec_pll_common *rec, u32 audio_pll_cnt, u32 ptp_now, unsigned int ticks)
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
		pll_latency_cycles = (u32)common_os_div_u64((u64)ptp_ts_latency_ns * rec->pll.current_rate, 1000000000ULL);
		pll_latency_cycles /= rec->pll_timer_clk_div;

		audio_pll_cnt -= pll_latency_cycles;

		/* Pass the audio pll measurement (over the sampling period) to the recovery mechanism */
		rc = mclock_rec_pll_common_timer_irq(rec, MCLOCK_REC_PLL_FLAGS_GPTP_EVENT, (u32)(audio_pll_cnt - rec->audio_pll_cnt_last), audio_pll_cnt, ticks);
		rec->audio_pll_cnt_last = audio_pll_cnt;

	} else {
		rc = mclock_rec_pll_common_timer_irq(rec, 0, 0, 0, ticks);
	}

	return rc;
}

/**
 * mclock_rec_pll_common_timer_irq() - Handles the recovery pll timer interrupt
 * @rec: pointer to the PLL media clock recovery context
 * @flags: event flags to specify external events like gptp capture events (timestamp matched the gPTP counter) and port interface down
 * @val: timer counter value at fec_event (audio pll tick at gptp event)
 * @meas: difference in ticks since last gptp event
 * @ticks: number of hw timer ticks
 * Return: 0 on success, negative otherwise.
 */
int mclock_rec_pll_common_timer_irq(struct mclock_rec_pll_common *rec, unsigned int flags, unsigned int meas, unsigned int pll_ticks_val, unsigned int ticks)
{
	struct mclock_dev *dev = &rec->dev;
	int32_t ticks_shift = 0;
	uint64_t dt_ts_ticks;
	unsigned int next_ts;
	int64_t ticks_err;
	int rc = 0;
	int err;

	dev->clk_timer += ticks * dev->timer_period;

	if (flags & MCLOCK_REC_PLL_FLAGS_PORT_DOWN) {
		rec->stats.err_port_down++;
		goto out_no_trace;
	}

	/* No capture event */
	if (!(flags & MCLOCK_REC_PLL_FLAGS_GPTP_EVENT)) {
		if (avtp_after(dev->clk_timer, rec->wd)) {
			rec->state = RESET;
			rec->stats.err_wd++;
		}
		goto out_no_trace;
	}

#if MCLOCK_PLL_REC_TRACE
	if (!rec->trace_freeze) {
		rec->trace[rec->trace_count].meas = meas;
		rec->trace[rec->trace_count].meas_diff = (int)(rec->pll_clk_period.i - meas);
		rec->trace[rec->trace_count].ticks = ticks;
		rec->trace[rec->trace_count].err = 0;
		rec->trace[rec->trace_count].adjust_value = 0;
	}
#endif


	mclock_rec_pll_common_wd_reset(rec);

	if (mclock_rec_pll_common_ts_get(rec, &next_ts) < 0) {
		rec->stats.err_ts++;
		goto out;
	}

	if (rec->is_hw_recovery_mode) {
		/* Reload FEC timer */
		rc = mclock_rec_pll_gptp_compare_reload(mclock_rec_pll_common_to_rec(rec), next_ts);
		if (rc < 0) {
			/*
			 * Do not reset the logic here, we can have a spurious FEC output edge at startup.
			 * If the TS compare chain has not properly started, no further events will happen
			 * and this will be caught by the timer watchdog.
			 */
			rec->stats.err_gptp_reload++;
			goto out;
		}

		rec->stats.gptp_reloaded++;
	} else {
		/* Save the next sw sampling timestamp. */
		rec->next_ts = next_ts;
	}

	/* Check that measurement is fine : if we have a drift > 500 ppm) it's probably phase offset change in gPTP counter */
	if (abs(rec->pll_clk_period.i - meas) >  ((rec->pll_clk_period.i * 500)/ 1000000)) {
		/* First value is often wrong, skip it. */
		if (rec->meas > 1) {
			rec->state = RESET;
			rc = -1;
		}
		rec->stats.err_meas++;
		rec->meas++;
		goto out;
	}

	/* Shift only in ADJUST_LOCKED phase. */
	if (rec->req_pll_shift_ns && rec->state == ADJUST_LOCKED) {
		ticks_shift = common_os_div64_s64((int64_t)rec->req_pll_shift_ns * rec->pll_ref_freq, 1000000000LL);

		rec->ts_ticks += ticks_shift;
		rec->total_pll_shift_ns += rec->req_pll_shift_ns;
		rec->req_pll_shift_ns = 0;
	}

	/* Absolute expected pll ticks: Add the expected pll ticks period for the desired ts frequency.
	 * NOTE: Ignore the fractional part of pll_clk_period as pll_clk_period is guaranteed to be integer
	 * by mclock_rec_pll_common_check_ts_freq() checking that pll_clk_frequency is multiple of ts_freq.
	 */
	rec->ts_ticks += rec->pll_clk_period.i;
	/* Absolute audio ticks value */
	rec->audio_pll_ticks += meas;

	/* Absolute phase error between absolute expected and sampled audio pll ticks. */
	ticks_err = rec->ts_ticks - rec->audio_pll_ticks;
	/* Elapsed period (in ticks) since last adjustement/detected phase eror. */
	dt_ts_ticks = (rec->ts_ticks - rec->prev_ts_ticks);

	/* make the error in ppb (divide phase error by the elapsed period since last error) */
	err = common_os_div64_s64(ticks_err * 1000000000LL, dt_ts_ticks);

	/* err stats */
	rec->stats.err_time += rec->gptp_period.i;

	if (rec->stats.err_time >= NSEC_PER_SEC) {
		rec->stats.err_per_sec = ticks_err;
		rec->stats.err_time = 0;
	}

#if MCLOCK_PLL_REC_TRACE
	if (!rec->trace_freeze) {
		rec->trace[rec->trace_count].ticks_shift = ticks_shift;
		rec->trace[rec->trace_count].pll_shift_ns = rec->total_pll_shift_ns;
		rec->trace[rec->trace_count].ts_ticks = rec->ts_ticks;
		rec->trace[rec->trace_count].audio_pll_ticks = rec->audio_pll_ticks;
		rec->trace[rec->trace_count].ticks_err = ticks_err;
		rec->trace[rec->trace_count].dt_ts_ticks = dt_ts_ticks;

		rec->trace[rec->trace_count].pll_ticks = pll_ticks_val;
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
	{
		/* Skip first 3 values measurement after reset to ensure we have proper sampling values. */
		if (rec->meas++ < MCLOCK_REC_PLL_NB_MEAS_START_SKIP)
			goto out;

		/* Use a common starting point: the captured pll ticks at the first "valid" timestamp. */
		rec->ts_ticks = pll_ticks_val;
		rec->audio_pll_ticks = pll_ticks_val;

		rec->initial_ts_ticks = rec->ts_ticks;
		rec->initial_pll_ticks = rec->audio_pll_ticks;

		rec->prev_ts_ticks = rec->ts_ticks;

		rec->state = MEASURE;
		break;
	}
	case MEASURE:
	{
		int64_t ticks_ppb;

		/* Delay first estimation of the ppb to reduce the initial error. */
		if(rec->meas++ < (MCLOCK_REC_PLL_NB_MEAS + MCLOCK_REC_PLL_NB_MEAS_START_SKIP))
			goto out;

		/* Initial frequency ratio, in ppb. (ppb = 10^9 * (ratio - 1)) */
		ticks_ppb = common_os_div64_u64(1000000000LL * (rec->ts_ticks - rec->initial_ts_ticks), rec->audio_pll_ticks - rec->initial_pll_ticks) - 1000000000LL;
		/* Take in consideration the previous ppb adjustement (if any). */
		ticks_ppb += rec->pll.ppb_adjust;

		/* After first ppb estimatiion calculation, resync the common starting point to avoid trying to catch phase offset accumulated in adjust phase. */
		rec->ts_ticks = pll_ticks_val;
		rec->audio_pll_ticks = pll_ticks_val;

		/* Reset PI controller to inital ppb estimation (u(0) = ratio * 10^9) and adjust the PLL accordingly. */
		pi_reset(&rec->pi, ticks_ppb + 1000000000ULL);
		/* Adjust the PLL with estimated drift. */
		mclock_rec_pll_common_adjust(rec, 0, false);

		rational_init(&rec->clk_media, dev->clk_timer , 1);
		rec->state = ADJUST;

		break;
	}
	case ADJUST:
		/* Sampling period starts with last detected error. */
		if (ticks_err)
			rec->prev_ts_ticks = rec->ts_ticks;

		if (abs(err) < MCLOCK_REC_PLL_IN_LOCKED_PPB_ERR) {
			/* After few consecutive measurements under MCLOCK_REC_PLL_IN_LOCKED_PPB_ERR, we can go to locked */
			if (++rec->accepted_ppb_err_nb >= MCLOCK_REC_PLL_IN_LOCKED_NB_VALID_PPB_ERR) {
				/* Declare clock domain locked. */
				if (!(dev->flags & MCLOCK_FLAGS_RUNNING_LOCKED)) {
					common_os_atomic_set(&rec->status, MCLOCK_RUNNING_LOCKED);
					dev->flags |= MCLOCK_FLAGS_RUNNING_LOCKED;
				}
				/* Go to ADJUST_LOCKED state with larger sampling period to reduce jitter. */
				rec->state = ADJUST_LOCKED;
				rec->prev_ts_ticks = rec->ts_ticks;
				rec->stats.locked_state++;
			}
		} else
			rec->accepted_ppb_err_nb = 0;

		/* Update the PI with error value and adjust pll afterwards. PI input is phase error divided by elapsed time. */
		mclock_rec_pll_common_adjust(rec, err, true);

		rec->locked_meas = 0;

		fallthrough;
	case ADJUST_LOCKED:
		/* Do PLL adjustement every 16 measurement. */
		if (++rec->locked_meas >= MCLOCK_REC_PLL_ADJUST_LOCKED_SAMPLING_NUM) {

			mclock_rec_pll_common_adjust(rec, err, true);

			rec->locked_meas = 0;
			/* Update the sampling period starting point. */
			rec->prev_ts_ticks = rec->ts_ticks;

			/* If error is larger than max adjust threshold, go back to ADJUST state for quicker adjustments. */
			if (abs(err) >= rec->adjust_locked_ppb_threshold) {
				rec->accepted_ppb_err_nb = 0;
				rec->state = ADJUST;
			}

		}

		rational_add(&rec->clk_media, &rec->clk_media, &rec->gptp_period);

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
		mclock_rec_pll_reset(mclock_rec_pll_common_to_rec(rec));

	return rc;
}

int mclock_rec_pll_common_config(struct mclock_dev *dev, struct mclock_sconfig *cfg)
{
	struct mclock_rec_pll_common *rec = container_of(dev, struct mclock_rec_pll_common, dev);
	int rc = 0;

	switch (cfg->cmd) {
	case MCLOCK_CFG_TS_SRC:
		if (dev->flags & MCLOCK_FLAGS_RUNNING) {
			rc = -MCLOCK_REC_PLL_EBUSY;
			goto exit;
		}

		if (mclock_rec_pll_common_configure_freqs(rec, cfg->ts_src, dev->ts_freq_p, dev->ts_freq_q) < 0) {
			rc = -MCLOCK_REC_PLL_EINVAL;
			common_os_log_err("%s : mclock_rec_pll_common_configure_freqs(%d, %u, %u) error\n",
						__func__, cfg->ts_src, dev->ts_freq_p, dev->ts_freq_q);
			goto exit;
		}

		dev->ts_src = cfg->ts_src;
		break;

	case MCLOCK_CFG_FREQ:
		if (dev->flags & MCLOCK_FLAGS_RUNNING) {
			rc = -MCLOCK_REC_PLL_EBUSY;
			goto exit;
		}

		if (dev->ts_src == TS_INTERNAL) {
			rc = -MCLOCK_REC_PLL_EINVAL;
			common_os_log_err("%s : cannot set ts freq in internal mode\n", __func__);
			goto exit;
		} else if (dev->ts_src == TS_EXTERNAL) {

			if (mclock_rec_pll_common_configure_freqs(rec, dev->ts_src, cfg->ts_freq.p, cfg->ts_freq.q) < 0) {
				rc = -MCLOCK_REC_PLL_EINVAL;
				common_os_log_err("%s : mclock_rec_pll_common_configure_freqs(%d, %u, %u) error\n",
							__func__, dev->ts_src, cfg->ts_freq.p, cfg->ts_freq.q);
				goto exit;

			}

			mclock_set_ts_freq(dev, cfg->ts_freq.p, cfg->ts_freq.q);
		} else {
			rc = -MCLOCK_REC_PLL_EINVAL;
		}

		break;

	default:
		rc = -MCLOCK_REC_PLL_EINVAL;
		break;
	}

exit:
	return rc;
}

static int mclock_rec_pll_common_start(struct mclock_rec_pll_common *rec, struct mclock_start *start,
					 unsigned int ki_factor, unsigned int kp_factor)
{
	struct mclock_dev *dev = &rec->dev;
	u32 now, ts_0, ts_1;
	int rc = 0;

	rec->stats.start++;

#if MCLOCK_PLL_REC_TRACE
	rec->trace_freeze = 0;
	rec->trace_count = 0;
#endif

	if (mclock_rec_pll_gptp_read(mclock_rec_pll_common_to_rec(rec), &now) < 0) {
		rc = -MCLOCK_REC_PLL_EIO;
		rec->stats.err_gptp_gettime++;
		goto out;
	}

	if (rec->dev.ts_src == TS_INTERNAL) {
		ts_0 = ((now / rec->gptp_period.i) + 2) * rec->gptp_period.i;
		ts_1 = ts_0 + rec->gptp_period.i;
	}
	else {
		/* user-space start, use provided timestamps */
		if (start) {
			rec->r_idx = 0;
			rec->ts_slot = 0;
			rec->ts_offset = 2 * rec->gptp_period.i;
			common_os_atomic_set(&rec->ts_read, 0);

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
				ts_1 = ts_0 + rec->gptp_period.i;
				rec->ts_slot++;
			}
		}
		/* internal reset */
		else {
			/* Get last ts in array for ts_1 and generate ts_0 */
			if (mclock_rec_pll_common_ts_reset(rec, (unsigned int *)&ts_1) < 0) {
				rec->stats.err_ts++;
				rc = -1;
				goto out;
			}

			ts_0 = ts_1 - rec->gptp_period.i;
		}
	}

	rational_init(&rec->gptp_next_ts, ts_1, 1);

	if (rec->is_hw_recovery_mode) {
		/* Start HW sampling ENET clock */
		rc = mclock_rec_pll_gptp_compare_start(mclock_rec_pll_common_to_rec(rec), ts_0, ts_1);
		if (rc < 0) {
			rec->stats.err_gptp_start++;
			goto out;
		}
	} else {
		/* Start SW sampling on ts_1. */
		rec->next_ts = ts_1;
	}

	rec->state = START;

	/* This function can be called in interrupt context, avoid calls to PLL API (sleeping) functions from here.
	 * The current_rate value is used for trace and in sw recovery calculation and should be updated on every
	 * ppb adjustement anyway. Using a cached value (either from previous adjustments or imx_pll_init()) as initial value should be ok.
	 */
	if (!rec->pll_deferred_adjust)
		rec->pll.current_rate = imx_pll_get_rate(&rec->pll);

	pi_init(&rec->pi, ki_factor, kp_factor);

	/* Initial control output is 0 (e.g 0 ppb variation) */
	pi_reset(&rec->pi, 0);
	rational_init(&rec->pll_clk_target, 0, 1);
	rec->pll_clk_meas = 0;
	rec->meas = 0;
	rec->start_ppb_err = 0;
	rec->accepted_ppb_err_nb = 0;
	rec->ts_ticks = 0;
	rec->audio_pll_ticks = 0;
	rec->prev_ts_ticks = 0;
	rec->initial_ts_ticks = 0;
	rec->initial_pll_ticks = 0;
	rec->total_pll_shift_ns = 0;
	rec->req_pll_shift_ns = 0;

	mclock_rec_pll_common_wd_reset(rec);
	/* Set the status to running */
	common_os_atomic_set(&rec->status, MCLOCK_RUNNING);
	dev->flags &= ~MCLOCK_FLAGS_RUNNING_LOCKED;

	rec->stats.err_per_sec = 0;
	rec->stats.err_time = 0;
	rec->stats.locked_state = 0;

out:
	return rc;
}

static void mclock_rec_pll_common_stop(struct mclock_rec_pll_common *rec)
{
	struct mclock_dev *dev = &rec->dev;

	rec->stats.stop++;

	if (rec->is_hw_recovery_mode)
		mclock_rec_pll_gptp_stop(mclock_rec_pll_common_to_rec(rec));

	common_os_atomic_set(&rec->status, MCLOCK_STOPPED);
	dev->flags &= ~MCLOCK_FLAGS_RUNNING_LOCKED;
}

__init static void mclock_rec_pll_common_init(struct mclock_rec_pll_common *rec)
{
	/* Based on (audio pll) timer input frequency, select max allowed drift in ADJUST_LOCKED
	 * phase (higher measurement window and less reactive controller) before reverting back to ADJUST phase:
	 * Higher timer input frequency means less measurement error jitter, thus lower drift threshold
	 */
	if (rec->pll_ref_freq <= MCLOCK_REC_PLL_LOW_FREQ_MAX_HZ)
		rec->adjust_locked_ppb_threshold = MCLOCK_REC_PLL_LOW_FREQ_PPB_THRESHOLD;
	else if (rec->pll_ref_freq <= MCLOCK_REC_PLL_MEDIUM_FREQ_MAX_HZ)
		rec->adjust_locked_ppb_threshold = MCLOCK_REC_PLL_MEDIUM_FREQ_PPB_THRESHOLD;
	else
		rec->adjust_locked_ppb_threshold = MCLOCK_REC_PLL_HIGH_FREQ_PPB_THRESHOLD;

	rec->max_adjust = MCLOCK_REC_PLL_MAX_ADJUST_PPB;
	rec->req_pll_shift_ns = 0;

	common_os_log_info("%s : pll input frequency (%u Hz), using an adjust locked threshold (%u ppb)\n",
			   __func__, rec->pll_ref_freq, rec->adjust_locked_ppb_threshold);

	mclock_rec_pll_common_configure_freqs(rec, TS_INTERNAL, 0, 0);

	/* For rec_pll, the ts freq holds value for the external ts
	  Set the default value here */
	mclock_set_ts_freq(&rec->dev, MCLOCK_REC_TS_FREQ_INIT, 1);

#if MCLOCK_PLL_REC_TRACE
	rec->trace_freeze = 0;
	rec->trace_count = 0;
#endif

}

#if MCLOCK_PLL_REC_TRACE

/* Should be called after pll adjustments in the case of a deferred tuning mechanism. */
static inline void __mclock_rec_pll_common_trace_override(struct mclock_rec_pll_common *rec)
{
	if (!rec->trace_freeze) {
		struct imx_pll *pll = &rec->pll;

		rec->trace[rec->override_trace_idx].adjust_value = pll->ppb_adjust;
		rec->trace[rec->override_trace_idx].new_rate = pll->current_rate;
	}
}
#endif
