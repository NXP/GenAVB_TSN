/*
* Copyright 2018, 2020 NXP
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
 @brief Stats printing
*/

#include "debug_print.h"
#include "common/log.h"
#include "slist.h"

#include "hw_timer.h"
#include "hr_timer.h"
#include "media_clock_drv.h"
#include "media_clock_rec_pll.h"
#include "media_clock_gen_ptp.h"

#include "net_port.h"
#include "net_tx.h"
#include "net_rx.h"
#include "avtp.h"

#if (defined(CONFIG_AVTP) && SHOW_MCLOCK_STATS)
extern struct mclock_drv *mclk_drv_h;
static void mclock_rec_pll_stats(struct mclock_rec_pll *rec)
{
	struct mclock_rec_pll_stats *stats = &rec->stats;
#if MCLOCK_PLL_REC_TRACE
	int i;
#endif

	os_log(LOG_INFO, "adjust              = %u\n", stats->adjust);
	os_log(LOG_INFO, "reset               = %u\n", stats->reset);
	os_log(LOG_INFO, "start               = %u\n", stats->start);
	os_log(LOG_INFO, "stop                = %u\n", stats->stop);
	os_log(LOG_INFO, "GPTP error          = %u\n", stats->err_gptp_reload);
	os_log(LOG_INFO, "GPTP start error    = %u\n", stats->err_gptp_start);
	os_log(LOG_INFO, "GPTP gettime error  = %u\n", stats->err_gptp_gettime);
	os_log(LOG_INFO, "measurement error   = %u\n", stats->err_meas);
	os_log(LOG_INFO, "watchdog error      = %u\n", stats->err_wd);
	os_log(LOG_INFO, "ts error            = %u\n", stats->err_ts);
	os_log(LOG_INFO, "drift error         = %u\n", stats->err_drift);
	os_log(LOG_INFO, "error (Hz/s)        = %d\n", stats->err_per_sec);
	os_log(LOG_INFO, "gpt_rec event       = %d\n", stats->irq_count);
	os_log(LOG_INFO, "gpt_rec event fec   = %d\n", stats->irq_count_fec_event);
	os_log(LOG_INFO, "fec_reloaded        = %d\n", stats->fec_reloaded);
	os_log(LOG_INFO, "numerator           = %d\n", stats->pll_numerator);
	os_log(LOG_INFO, "measure             = %d\n", stats->measure);
	os_log(LOG_INFO, "err_set_pll_rate    = %d\n", stats->err_set_pll_rate);
	os_log(LOG_INFO, "err_pll_prec        = %d\n", stats->err_pll_prec);
	os_log(LOG_INFO, "last_app_adjust     = %d\n", stats->last_app_adjust);

#if MCLOCK_PLL_REC_TRACE
	os_log(LOG_INFO, "\n%-3s %-15s %-15s %-15s %-15s %-15s %-15s %-15s %-15s %-15s \n","idx", "GPT meas", "GPT ticks", "state", "err", "adjust_val", "previous_rate", "new_rate", "pi_err_input", "pi_control_out");

	for (i = 0; i < rec->trace_count; i++) {
		struct mclock_rec_trace *t = &rec->trace[i];
		os_log(LOG_INFO, "[%-3d] %-15u %-15u %-15u %-15d %-15d %-15lu %-15lu %-15d %-15d \n", i, t->meas, t->ticks, t->state, t->err, t->adjust_value, t->previous_rate, t->new_rate, t->pi_err_input, t->pi_control_output);
	}

	rec->trace_count = 0;
	rec->trace_freeze = 0;
#endif /* MCLOCK_PLL_REC_TRACE */
}

static void mclock_gen_ptp_stats(struct mclock_gen_ptp *gen_ptp)
{
	struct mclock_gen_ptp_stats *stats = &gen_ptp->stats;

	os_log(LOG_INFO, "reset           = %d\n", stats->reset);
	os_log(LOG_INFO, "start           = %d\n", stats->start);
	os_log(LOG_INFO, "stop            = %d\n", stats->stop);
	os_log(LOG_INFO, "TS period       = %d\n", stats->ts_period);
	os_log(LOG_INFO, "PTP now         = %d\n", stats->ptp_now);
	os_log(LOG_INFO, "drift error     = %d\n", stats->err_drift);
	os_log(LOG_INFO, "PTP jump error  = %d\n", stats->err_jump);
}

void mclock_show_stats(void)
{
	struct mclock_drv *drv = mclk_drv_h;
	struct mclock_dev *dev;
	struct slist_node *entry;

	if (!drv)
		return;

	for (entry = slist_first(&drv->mclock_devices); !slist_is_last(entry); entry = slist_next(entry)) {
		dev = container_of(entry, struct mclock_dev, list_node);

		if (dev->flags & MCLOCK_FLAGS_FREE)
			continue;

		if (dev->type == REC)
			mclock_rec_pll_stats((struct mclock_rec_pll *)dev);
		else if (dev->type == PTP)
			mclock_gen_ptp_stats((struct mclock_gen_ptp *)dev);
	}
}
#endif /* (defined(CONFIG_AVTP) && SHOW_MCLOCK_STATS) */


#if SHOW_HW_TIMER_STATS
extern struct hw_avb_timer *gtimer_h;
static inline void hw_timer_stats(struct stats *s)
{
	stats_compute(s);
	stats_reset(s);
	os_log(LOG_INFO, "%s min %d mean %d max %d rms^2 %llu stddev^2 %llu\n", s->priv, s->min, s->mean, s->max, s->ms, s->variance);
}

void hw_timer_show_stats(void)
{
	struct hw_avb_timer *timer = gtimer_h;
	if (timer) {
		hw_timer_stats(&timer->runtime_stats);
		hw_timer_stats(&timer->delay_stats);
	}
}
#endif /* SHOW_HW_TIMER_STATS */

#if SHOW_HR_TIMER_STATS
extern struct hr_timer_drv *hr_timer_drv_h;
static void hr_timer_task_stats(struct hr_timer_task_ctx *ctx)
{
	struct hr_timer_task_stats *stats = &ctx->stats;

	os_log(LOG_INFO, "hr timer task(%p)\n", ctx);
	os_log(LOG_INFO, "enqueue: %u, cancel: %u, run: %u\n",
		stats->enqueue, stats->cancel, stats->run);
	os_log(LOG_INFO, "errors sched: %u, timeout: %u\n",
		stats->err_sched, stats->err_timeout);
}

static void hr_timer_stats(struct hr_timer *timer)
{
	struct hr_timer_stats *stats = &timer->stats;

	os_log(LOG_INFO, "timer(%p), hw_timer(%p), clock id: %d\n",
	       timer, timer->hw_timer, timer->clk_id);
	os_log(LOG_INFO, "period: %llu, next_event: %llu\n",
	       timer->period, timer->next_event);
	os_log(LOG_INFO, "start: %u, stop: %u, events: %u, clock discont: %u\n",
	       stats->start, stats->stop, stats->events, stats->clock_discont);
	os_log(LOG_INFO, "errors start: %u, event: %u, event isr: %u, clock: %u\n",
	       stats->err_start, stats->err_event, stats->err_event_isr, stats->err_clock);
}

void hr_timer_show_stats(void)
{
	struct hr_timer_drv *drv = hr_timer_drv_h;

	if (drv) {
		struct slist_node *entry;

		hr_timer_task_stats(&drv->task_ctx);

		xSemaphoreTake(drv->lock, portMAX_DELAY);

		slist_for_each(&drv->list, entry)
			hr_timer_stats(container_of(entry, struct hr_timer, node_drv));

		xSemaphoreGive(drv->lock);
	}
}
#endif /* SHOW_HR_TIMER_STATS */

#if SHOW_QOS_STATS
void net_port_qos_show_stats(void)
{
	int p;

	for (p = 0; p < CFG_PORTS; p++) {
		struct net_port *port = &ports[p];
		struct port_qos *qos = port->qos;
		int i, j;

		os_log(LOG_INFO, "port%d qos statistics:\n", port->index);
		os_log(LOG_INFO, "    tx: %u, dropped: %u\n", qos->tx, qos->tx_drop);
		os_log(LOG_INFO, "    interval: %u (ns), streams: %u\n", qos->interval, qos->streams);
		os_log(LOG_INFO, "    shaper:\n");
		os_log(LOG_INFO, "        rate: %u, credit: %d, credit_min: %d (bits/interval) last: %u\n",
		       qos->shaper.rate, qos->shaper.credit, qos->shaper.credit_min, qos->shaper.tlast);
		os_log(LOG_INFO, "        used rate: %u, max rate: %u (bits/s)\n", qos->used_rate, qos->max_rate);
		os_log(LOG_INFO, "    clock grid:\n");
		os_log(LOG_INFO, "        now: %u (ns), period: %u (ns), count: %u, reset: %u\n",
		       qos->ptp_grid.now, qos->ptp_grid.period, qos->ptp_grid.count, qos->ptp_grid.reset);
		os_log(LOG_INFO, "        pi err: %d, integral: %lld\n", qos->ptp_grid.pi.err, qos->ptp_grid.pi.integral);

		for (i = CFG_TRAFFIC_CLASS_MAX - 1; i >= 0; i--) {
			struct traffic_class *tc = &qos->traffic_class[i];

			os_log(LOG_INFO, "    tc%d tx: %u, sched_mask: %lx, shared_mask: %lx\n",
			       tc->index, tc->tx, tc->scheduled_mask, tc->shared_pending_mask);

			for (j = 0; j < CFG_TRAFFIC_CLASS_QUEUE_MAX; j++) {
				struct qos_queue *qos_q = &tc->qos_queue[j];

				if (qos_q->flags & (QOS_QUEUE_FLAG_ENABLED))
					os_log(LOG_INFO, "        q%d tx: %u, dropped: %u, full: %u, disabled: %u, flags: %x, pending: %u\n",
					       qos_q->index, qos_q->tx, qos_q->dropped, qos_q->full, qos_q->disabled,
					       qos_q->flags, qos_q->queue ? queue_pending(qos_q->queue) : 0);
			}
		}
	}
}
#endif /* SHOW_QOS_STATS */

#if SHOW_PORT_STATS

#define net_port_rx_show(ptype_str, stats)							\
{												\
	os_log(LOG_INFO, "    %-6s rx: %10u, dropped: %10u, slow: %10u, slow_dropped: %10u\n",	\
	       ptype_str, (stats)->rx, (stats)->dropped, (stats)->slow, (stats)->slow_dropped);	\
}												\

void net_port_show_stats(void)
{
	int p;

	for (p = 0; p < CFG_PORTS; p++) {
		struct net_rx_ctx *net_rx = &net_rx_ctx;
		struct net_port *port = &ports[p];

		os_log(LOG_INFO, "port%d rx statistics:\n", port->index);
		net_port_rx_show("gptp", &net_rx->ptype_hdlr[PTYPE_PTP].stats[p]);
		net_port_rx_show("avtp", &net_rx->ptype_hdlr[PTYPE_AVTP].stats[p]);
		net_port_rx_show("mrp", &net_rx->ptype_hdlr[PTYPE_MRP].stats[p]);
		net_port_rx_show("l2", &net_rx->ptype_hdlr[PTYPE_L2].stats[p]);
		net_port_rx_show("other", &net_rx->ptype_hdlr[PTYPE_OTHER].stats[p]);
		net_port_rx_show("maap", &avtp_rx_hdlr.maap.stats[p]);
		net_port_rx_show("avdecc", &avtp_rx_hdlr.avdecc.stats[p]);
	}

	for (p = 0; p < CFG_PORTS; p++) {
		struct net_port *port = &ports[p];
		int q;

		os_log(LOG_INFO, "port%d queues statistics:\n", port->index);

		os_log(LOG_INFO, "    rx:\n");
		for (q = port->num_rx_q - 1; q >= 0; q--)
			os_log(LOG_INFO, "        q%d rx: %10u, err: %10u, err_alloc: %u\n",
			       q, port->stats[q].rx, port->stats[q].rx_err, port->stats[q].rx_alloc_err);

		os_log(LOG_INFO, "    tx:\n");
		for (q = port->num_tx_q - 1; q >= 0; q--)
			os_log(LOG_INFO, "        q%d tx: %10u, err: %10u, err_ts: %u\n",
			       q, port->stats[q].tx, port->stats[q].tx_err, port->stats[q].tx_ts_err);
	}
}
#endif /* SHOW_PORT_STATS */
