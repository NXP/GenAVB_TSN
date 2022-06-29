/*
 * AVB debugfs driver
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

#include <linux/debugfs.h>

#include "genavb/rtp.h"
#include "media_clock.h"
#include "media_clock_drv.h"
#include "dmadrv.h"
#include "net_port.h"
#include "ptp.h"
#include "avtp.h"
#include "ipv4.h"
#include "hw_timer.h"
#include "switch.h"

static int net_qos_queue_show(struct seq_file *s, void *data)
{
	struct qos_queue *qos_q = s->private;
	unsigned long flags;

	seq_printf(s, "flags      = %10u\n", qos_q->flags);
	seq_printf(s, "tx         = %10u\n", qos_q->tx);
	seq_printf(s, "dropped    = %10u\n", qos_q->dropped);
	seq_printf(s, "queue full = %10u\n", qos_q->full);
	seq_printf(s, "disabled   = %10u\n", qos_q->disabled);

	raw_spin_lock_irqsave(&ptype_lock, flags);
	if (qos_q->queue)
		seq_printf(s, "pending    = %10u\n", queue_pending(qos_q->queue));
	else
		seq_printf(s, "pending    = %10u\n", 0);

	raw_spin_unlock_irqrestore(&ptype_lock, flags);

	return 0;
}

static int net_qos_queue_seq_open(struct inode *inode, struct file *file)
{
	return single_open(file, net_qos_queue_show, inode->i_private);
}

static const struct file_operations net_qos_queue_fops = {
	.open		= net_qos_queue_seq_open,
	.release	= single_release,
	.read		= seq_read,
	.llseek		= seq_lseek,
};

static int net_stream_show(struct seq_file *s, void *data)
{
	struct stream_queue *stream = s->private;
	unsigned long flags;

	seq_printf(s, "flags           = %10x\n", stream->flags);

	raw_spin_lock_irqsave(&ptype_lock, flags);

	if (stream->qos_queue) {
		seq_printf(s, "tx         = %10u\n", stream->qos_queue->tx);
		seq_printf(s, "dropped    = %10u\n", stream->qos_queue->dropped);
		seq_printf(s, "queue full = %10u\n", stream->qos_queue->full);
		seq_printf(s, "disabled   = %10u\n", stream->qos_queue->disabled);
		seq_printf(s, "pending    = %10u\n", queue_pending(stream->qos_queue->queue));
	} else
		seq_printf(s, "pending    = %10u\n", 0);

	raw_spin_unlock_irqrestore(&ptype_lock, flags);

	seq_printf(s, "credit     = %10d\n", stream->shaper.credit);
	seq_printf(s, "credit min = %10d\n", stream->shaper.credit_min);
	seq_printf(s, "last       = %10u\n", stream->shaper.tlast);
	seq_printf(s, "rate       = %10u (bits/interval)\n", stream->shaper.rate);
#ifdef PORT_TRACE
	seq_printf(s, "burst      = %10u\n", stream->burst_max);
#endif
	seq_printf(s, "id         = %08x%08x\n", ntohl(((u32 *)stream->id)[0]), ntohl(((u32 *)stream->id)[1]));

	return 0;
}

static int net_stream_seq_open(struct inode *inode, struct file *file)
{
	return single_open(file, net_stream_show, inode->i_private);
}

static int net_stream_seq_release(struct inode *inode, struct file *file)
{
#ifdef PORT_TRACE
	struct stream_queue *stream = inode->i_private;
#endif
	int rc = single_release(inode, file);

#ifdef PORT_TRACE
	stream->burst_max = 0;
#endif

	return rc;
}

static const struct file_operations net_stream_fops = {
	.open		= net_stream_seq_open,
	.release	= net_stream_seq_release,
	.read		= seq_read,
	.llseek		= seq_lseek,
};

static int net_class_show(struct seq_file *s, void *data)
{
	struct sr_class *class = s->private;

	seq_printf(s, "tx         = %10u\n", class->tc->tx);
	seq_printf(s, "interval   = %10u (ns)\n", rational_int_mul(class->scale, &class->interval));
	seq_printf(s, "scale      = %10u\n", class->scale);
	seq_printf(s, "credit     = %10d (bits/interval)\n", class->shaper.credit);
	seq_printf(s, "credit min = %10d (bits/interval)\n", class->shaper.credit_min);
	seq_printf(s, "last       = %10u (interval)\n", class->shaper.tlast);
	seq_printf(s, "rate       = %10u (bits/interval)\n", class->shaper.rate);
	seq_printf(s, "pending    = %10lx\n", class->pending_mask);
	seq_printf(s, "scheduled  = %10lx\n", class->tc->scheduled_mask);
	seq_printf(s, "shared     = %10lx\n", class->tc->shared_pending_mask);
	seq_printf(s, "streams    = %10u\n", class->streams);

	return 0;
}

static int net_class_seq_open(struct inode *inode, struct file *file)
{
	return single_open(file, net_class_show, inode->i_private);
}

static const struct file_operations net_class_fops = {
	.open		= net_class_seq_open,
	.release	= single_release,
	.read		= seq_read,
	.llseek		= seq_lseek,
};

static int net_traffic_class_show(struct seq_file *s, void *data)
{
	struct traffic_class *tc = s->private;

	seq_printf(s, "tx         = %10u\n", tc->tx);
	seq_printf(s, "scheduled  = %10lx\n", tc->scheduled_mask);
	seq_printf(s, "shared     = %10lx\n", tc->shared_pending_mask);

	return 0;
}

static int net_traffic_class_seq_open(struct inode *inode, struct file *file)
{
	return single_open(file, net_traffic_class_show, inode->i_private);
}

static const struct file_operations net_traffic_class_fops = {
	.open		= net_traffic_class_seq_open,
	.release	= single_release,
	.read		= seq_read,
	.llseek		= seq_lseek,
};

static int net_port_show(struct seq_file *s, void *data)
{
	struct port_qos *port = s->private;
#ifdef PORT_TRACE
	int i;
#endif

	seq_printf(s, "tx         = %10u\n", port->tx);
	seq_printf(s, "tx full    = %10u\n", port->tx_full);
	seq_printf(s, "interval   = %10u\n", port->interval);
	seq_printf(s, "credit     = %10d (bits/interval)\n", port->shaper.credit);
	seq_printf(s, "credit min = %10d (bits/interval)\n", port->shaper.credit_min);
	seq_printf(s, "last       = %10u (interval)\n", port->shaper.tlast);
	seq_printf(s, "rate       = %10u (bits/interval)\n", port->shaper.rate);
	seq_printf(s, "max rate   = %10u (bits/s)\n", port->max_rate);
	seq_printf(s, "streams    = %10u\n", port->streams);
	seq_printf(s, "used rate  = %10u (bits/s)\n", port->used_rate);

	seq_printf(s, "\ntimer interval:\n");
	seq_printf(s, "dt_mean  = %u\n", port->jitter_stats.dt_mean);
	seq_printf(s, "dt_mean2 = %u\n", port->jitter_stats.dt_mean2);
	seq_printf(s, "dt_min   = %u\n", port->jitter_stats.dt_min);
	seq_printf(s, "dt_max   = %u\n", port->jitter_stats.dt_max);

	seq_printf(s, "\nclock grid:\n");
	seq_printf(s, "now      = %10u (ns)\n", port->ptp_grid.now);
	seq_printf(s, "period   = %10u (ns)\n", port->ptp_grid.period);
	seq_printf(s, "err      = %10d\n", port->ptp_grid.pi.err);
	seq_printf(s, "integral = %10lld\n", port->ptp_grid.pi.integral);
	seq_printf(s, "count    = %10u\n", port->ptp_grid.count);
	seq_printf(s, "reset    = %10u\n", port->ptp_grid.reset);

#ifdef PORT_TRACE
	for (i = 0; i < PORT_TRACE_SIZE; i++) {

		seq_printf(s, "%10u %10u %6u %4u %6d %16lld %10llu\n", port->ptp_grid.trace[i].now, port->ptp_grid.trace[i].ptp,
			port->ptp_grid.trace[i].period, port->ptp_grid.trace[i].period_frac,
			port->ptp_grid.trace[i].err, port->ptp_grid.trace[i].integral, port->ptp_grid.trace[i].total);
	}

	for (i = 0; i < PORT_TRACE_SIZE; i++) {
		struct port_trace *t = &port->trace[i];

		seq_printf(s, "%10u %10u %10u %11d %2u %2u % 9d % 9d % 9d %4x %4x %1x %2d\n",
			t->tnow, t->ptp, t->ts, t->ts == 0? 0 : t->ts - t->ptp,
			t->class, t->queue,
			t->port_credit, t->class_credit, t->queue_credit,
			t->scheduled_mask, t->pending_mask, t->ready, t->pending);
	}
#endif

	return 0;
}

static int net_port_seq_open(struct inode *inode, struct file *file)
{
	return single_open(file, net_port_show, inode->i_private);
}

static int net_port_seq_release(struct inode *inode, struct file *file)
{
	struct port_qos *port = inode->i_private;

	int rc = single_release(inode, file);

	port_jitter_stats_init(&port->jitter_stats);
#ifdef PORT_TRACE
	port->trace_freeze = 0;
	port->ptp_grid.trace_count = 0;
#endif

	return rc;
}

static const struct file_operations net_port_fops = {
	.open		= net_port_seq_open,
	.release	= net_port_seq_release,
	.read		= seq_read,
	.llseek		= seq_lseek,
};

void net_qos_debugfs_init(struct net_qos *net, struct dentry *avb_dentry)
{
	struct dentry *tx_dentry, *port_dentry, *tc_dentry, *class_dentry;
	char name[32];
	int i, j, k;

	tx_dentry = debugfs_create_dir("tx", avb_dentry);
	if (!tx_dentry)
		return;

	for (i = 0; i < CFG_PORTS; i++) {

		snprintf(name, 32, "port%d", i);

		port_dentry = debugfs_create_dir(name, tx_dentry);
		if (!port_dentry)
			continue;

		debugfs_create_file("shaper", S_IRUSR, port_dentry, &net->port[i], &net_port_fops);

		for (j = 0; j < CFG_TRAFFIC_CLASS_MAX; j++) {
			struct traffic_class *tc = &net->port[i].traffic_class[j];

			snprintf(name, 32, "tc%d", j);

			tc_dentry = debugfs_create_dir(name, port_dentry);
			if (!tc_dentry)
				continue;

			debugfs_create_file("total", S_IRUSR, tc_dentry, tc, &net_traffic_class_fops);

			for (k = 0; k < CFG_TRAFFIC_CLASS_QUEUE_MAX; k++) {

				snprintf(name, 32, "queue%d", k);

				debugfs_create_file(name, S_IRUSR, tc_dentry, &tc->qos_queue[k], &net_qos_queue_fops);
			}
		}

		for (j = 0; j < CFG_SR_CLASS_MAX; j++) {
			struct sr_class *class = &net->port[i].sr_class[j];

			snprintf(name, 32, "class%d", j);

			class_dentry = debugfs_create_dir(name, port_dentry);
			if (!class_dentry)
				continue;

			debugfs_create_file("shaper", S_IRUSR, class_dentry, class, &net_class_fops);

			for (k = 0; k < class->stream_max; k++) {

				snprintf(name, 32, "stream%d", k);

				debugfs_create_file(name, S_IRUSR, class_dentry, &class->stream[k], &net_stream_fops);
			}
		}
	}
}

#ifdef CFG_NET_STREAM_STATS
static int net_avtp_rx_stream_show(struct seq_file *s, void *data)
{
	struct avtp_stream_rx_hdlr *hdlr = s->private;
	struct hlist_node *node;
	struct net_socket *sock;
	unsigned int hash, i, prio;
	unsigned long flags;

	raw_spin_lock_irqsave(&ptype_lock, flags);

	for (prio = 0; prio < CFG_SR_CLASS_MAX; prio++) {
		for (hash = 0; hash < STREAM_HASH; hash++) {
			hlist_for_each(node, &hdlr->sock_head[hash]) {
				sock = hlist_entry(node, struct net_socket, node);

				if (sr_class_prio(sock->addr.u.avtp.sr_class) != prio)
					continue;

				seq_printf(s, "id      = %08x%08x\n", ntohl(((u32 *)sock->addr.u.avtp.stream_id)[0]), ntohl(((u32 *)sock->addr.u.avtp.stream_id)[1]));
				seq_printf(s, "rx      = %u\n", sock->rx);
				seq_printf(s, "tlast   = %u\n", sock->tlast);
				seq_printf(s, "dt_min  = %u\n", sock->dt_min);
				seq_printf(s, "dt_max  = %u\n", sock->dt_max);

				seq_printf(s, "dt_bin  =");
				for (i = 0; i < NET_STATS_BIN_MAX; i++)
					seq_printf(s, " %u", sock->dt_bin[i]);

				seq_printf(s, " (%d ns/bin)\n", 1 << dt_bin_width_shift[sock->addr.u.avtp.sr_class]);
				seq_printf(s, "dt_avtp_min  = %d\n", sock->avtp.dt_min);
				seq_printf(s, "dt_avtp_max  = %d\n", sock->avtp.dt_max);

				seq_printf(s, "dt_avtp_bin  =");
				for (i = 0; i < NET_STATS_BIN_MAX; i++)
					seq_printf(s, " %u", sock->avtp.dt_bin[i]);

				seq_printf(s, " (%d ns/bin)\n", 1 << avtp_dt_bin_width_shift[sock->addr.u.avtp.sr_class]);
			}
		}
	}

	raw_spin_unlock_irqrestore(&ptype_lock, flags);

	return 0;
}

static int net_avtp_rx_stream_seq_open(struct inode *inode, struct file *file)
{
	return single_open(file, net_avtp_rx_stream_show, inode->i_private);
}

static const struct file_operations net_avtp_rx_stream_fops = {
	.open		= net_avtp_rx_stream_seq_open,
	.release	= single_release,
	.read		= seq_read,
	.llseek		= seq_lseek,
};

static int net_rtp_rx_stream_show(struct seq_file *s, void *data)
{
	struct ipv4_stream_rx_hdlr *hdlr = s->private;
	struct rtp_stream_rx_hdlr *rtp_hdlr;
	struct hlist_node *node, *rtp_node;
	struct ipv4_socket *ipv4_sock;
	struct net_socket *sock;
	unsigned int hash, rtp_hash, i;
	unsigned long flags;

	raw_spin_lock_irqsave(&ptype_lock, flags);

	for (hash = 0; hash < IPV4_STREAM_HASH; hash++) {
		hlist_for_each(node, &hdlr->sock_head[hash]) {
			ipv4_sock = hlist_entry(node, struct ipv4_socket, node);

			if ((!ipv4_sock->l5_rx) || (ipv4_sock->l5_proto != L5PROTO_RTP))
				continue;

			rtp_hdlr = &ipv4_sock->l5_rx_hdlr;

			seq_printf(s, "saddr   = %08x\n", ntohl(ipv4_sock->saddr));
			seq_printf(s, "daddr   = %08x\n", ntohl(ipv4_sock->daddr));
			seq_printf(s, "sport   = %04x\n", ntohs(ipv4_sock->sport));
			seq_printf(s, "dport   = %04x\n", ntohs(ipv4_sock->dport));

			for (rtp_hash = 0; rtp_hash < RTP_STREAM_HASH; rtp_hash++) {
				hlist_for_each(rtp_node, &rtp_hdlr->sock_head[rtp_hash]) {
					sock = hlist_entry(rtp_node, struct net_socket, node);

					seq_printf(s, "ssrc    = %08x\n", ntohl(sock->addr.u.ipv4.u.rtp.ssrc));
					seq_printf(s, "rx      = %u\n", sock->rx);
					seq_printf(s, "tlast   = %u\n", sock->tlast);
					seq_printf(s, "dt_min  = %u\n", sock->dt_min);
					seq_printf(s, "dt_max  = %u\n", sock->dt_max);
					seq_printf(s, "max-min = %u (ns)\n", sock->dt_max - sock->dt_min);

					seq_printf(s, "dt_bin  =");
					for (i = 0; i < NET_STATS_BIN_MAX; i++) {
						seq_printf(s, " %u", sock->dt_bin[i]);
						if ((i + 1) % 20 == 0)
							seq_printf(s, " |\n         ");
					}

					seq_printf(s, " (%dus/bin, so %dus by line)\n", NET_STATS_RTP_DT_BIN_WIDTH / 1000,
						(NET_STATS_RTP_DT_BIN_WIDTH * 20) / 1000);

					seq_printf(s, "dt_rtp_min  = %d\n", sock->rtp.dt_min);
					seq_printf(s, "dt_rtp_max  = %d\n", sock->rtp.dt_max);

					seq_printf(s, "dt_rtp_bin  =");
					for (i = 0; i < NET_STATS_BIN_MAX; i++)
						seq_printf(s, " %u", sock->rtp.dt_bin[i]);

					seq_printf(s, " (%dus/bin)\n", NET_STATS_RTP_DT_BIN_WIDTH / 1000);
				}
			}
		}
	}

	raw_spin_unlock_irqrestore(&ptype_lock, flags);

	return 0;
}

static int net_rtp_rx_stream_seq_open(struct inode *inode, struct file *file)
{
	return single_open(file, net_rtp_rx_stream_show, inode->i_private);
}

static const struct file_operations net_rtp_rx_stream_fops = {
	.open		= net_rtp_rx_stream_seq_open,
	.release	= single_release,
	.read		= seq_read,
	.llseek		= seq_lseek,
};

#endif /* CFG_NET_STREAM_STATS */

static int net_rx_show(struct seq_file *s, void *data)
{
	struct net_rx_stats *stats = s->private;

	seq_printf(s, "rx           = %u\n", stats->rx);
	seq_printf(s, "dropped      = %u\n", stats->dropped);
	seq_printf(s, "slow         = %u\n", stats->slow);
	seq_printf(s, "slow_dropped = %u\n", stats->slow_dropped);

	return 0;
}

static int net_rx_seq_open(struct inode *inode, struct file *file)
{
	return single_open(file, net_rx_show, inode->i_private);
}

static const struct file_operations net_rx_fops = {
	.open		= net_rx_seq_open,
	.release	= single_release,
	.read		= seq_read,
	.llseek		= seq_lseek,
};

void net_rx_debugfs_init(struct eth_avb *eth, struct dentry *avb_dentry)
{
	struct dentry *rx_dentry;
	struct dentry *port_dentry;
	int i;

	rx_dentry = debugfs_create_dir("rx", avb_dentry);
	if (!rx_dentry)
		return;

	for (i = 0; i < CFG_PORTS; i++) {
		char name[32];

		snprintf(name, 32, "port%d", i);

		port_dentry = debugfs_create_dir(name, rx_dentry);
		if (!port_dentry)
			continue;

		debugfs_create_file("gptp", S_IRUSR, port_dentry, &ptype_hdlr[PTYPE_PTP].stats[i], &net_rx_fops);
		debugfs_create_file("avtp", S_IRUSR, port_dentry, &ptype_hdlr[PTYPE_AVTP].stats[i], &net_rx_fops);
		debugfs_create_file("mrp", S_IRUSR, port_dentry, &ptype_hdlr[PTYPE_MRP].stats[i], &net_rx_fops);
		debugfs_create_file("other", S_IRUSR, port_dentry, &ptype_hdlr[PTYPE_OTHER].stats[i], &net_rx_fops);
#ifdef CFG_AVB_IPV4_1733
		debugfs_create_file("ipv4", S_IRUSR, port_dentry, &ptype_hdlr[PTYPE_IPV4].stats[i], &net_rx_fops);
#endif

		debugfs_create_file("maap", S_IRUSR, port_dentry, &avtp_rx_hdlr.maap.stats[i], &net_rx_fops);
		debugfs_create_file("avdecc", S_IRUSR, port_dentry, &avtp_rx_hdlr.avdecc.stats[i], &net_rx_fops);

#ifdef CFG_NET_STREAM_STATS
		debugfs_create_file("stream", S_IRUSR, port_dentry, &avtp_rx_hdlr.stream[i], &net_avtp_rx_stream_fops);
#endif

#ifdef CFG_AVB_IPV4_1733
#ifdef CFG_NET_STREAM_STATS
		debugfs_create_file("rtp", S_IRUSR, port_dentry, &ipv4_rx_hdlr.stream[i], &net_rtp_rx_stream_fops);
#endif
#endif

	}
}

static int mclock_gen_ptp_show(struct seq_file *s, void *data)
{
	struct mclock_gen_ptp_stats *stats = s->private;

	seq_printf(s, "reset           = %u\n", stats->reset);
	seq_printf(s, "start           = %u\n", stats->start);
	seq_printf(s, "stop            = %u\n", stats->stop);
	seq_printf(s, "TS period       = %u\n", stats->ts_period);
	seq_printf(s, "PTP now         = %u\n", stats->ptp_now);
	seq_printf(s, "drift error     = %u\n", stats->err_drift);
	seq_printf(s, "PTP jump error  = %u\n", stats->err_jump);

	return 0;
}

static int mclock_gen_ptp_seq_open(struct inode *inode, struct file *file)
{
	return single_open(file, mclock_gen_ptp_show, inode->i_private);
}

static const struct file_operations mclock_gen_ptp_fops = {
	.open		= mclock_gen_ptp_seq_open,
	.release	= single_release,
	.read		= seq_read,
	.llseek		= seq_lseek,
};

struct dentry *mclock_gen_ptp_debugfs_init(struct mclock_drv *drv, struct mclock_gen_ptp_stats *stats, int port)
{
	char name[32];

	if (!drv->mclock_dentry)
		goto err;

	snprintf(name, 32, "gen_ptp_%d", port);

	return debugfs_create_file(name, S_IRUSR, drv->mclock_dentry, stats, &mclock_gen_ptp_fops);

err:
	return NULL;
}

void mclock_gen_ptp_debugfs_exit(struct dentry *mclock_dentry)
{
	debugfs_remove(mclock_dentry);
}

static int mclock_rec_pll_show(struct seq_file *s, void *data)
{
	struct mclock_rec_pll *rec = s->private;
	struct mclock_rec_pll_stats *stats = &rec->stats;
#if MCLOCK_PLL_REC_TRACE
	int i;
#endif

	seq_printf(s, "adjust				= %u\n", stats->adjust);
	seq_printf(s, "last applied ppb adjust		= %d\n", stats->last_app_adjust);
	seq_printf(s, "reset				= %u\n", stats->reset);
	seq_printf(s, "start				= %u\n", stats->start);
	seq_printf(s, "stop				= %u\n", stats->stop);
	seq_printf(s, "FEC error			= %u\n", stats->err_fec);
	seq_printf(s, "set PLL rate error		= %u\n", stats->err_set_pll_rate);
	seq_printf(s, "PLL Precision error		= %u\n", stats->err_pll_prec);
	seq_printf(s, "measurement error		= %u\n", stats->err_meas);
	seq_printf(s, "port down error			= %u\n", stats->err_port_down);
	seq_printf(s, "watchdog error			= %u\n", stats->err_wd);
	seq_printf(s, "ts error				= %u\n", stats->err_ts);
	seq_printf(s, "drift error			= %u\n", stats->err_drift);
	seq_printf(s, "error (Hz/s)			= %d\n", stats->err_per_sec);
#if MCLOCK_PLL_REC_TRACE
	seq_printf(s, "\n%-3s %-15s %-15s %-15s %-15s %-15s %-15s %-15s %-15s %-15s \n","idx", "GPT meas", "GPT ticks", "state", "err", "adjust_val", "previous_rate", "new_rate", "pi_err_input", "pi_control_out");

	for (i = 0; i < rec->trace_count; i++) {
		struct mclock_rec_trace *t = &rec->trace[i];
		seq_printf(s, "[%-3d] %-15u %-15u %-15u %-15d %-15d %-15lu %-15lu %-15d %-15d \n", i, t->meas, t->ticks, t->state, t->err, t->adjust_value, t->previous_rate, t->new_rate, t->pi_err_input, t->pi_control_output);
	}
#endif
	return 0;
}

static int mclock_rec_pll_seq_open(struct inode *inode, struct file *file)
{
		return single_open(file, mclock_rec_pll_show, inode->i_private);
}

static int mclock_rec_pll_seq_release(struct inode *inode, struct file *file)
{
#if MCLOCK_PLL_REC_TRACE
	struct mclock_rec_pll *rec = inode->i_private;
#endif
	int rc = single_release(inode, file);

#if MCLOCK_PLL_REC_TRACE
	rec->trace_count = 0;
	rec->trace_freeze = 0;
#endif
	return rc;
}

static const struct file_operations mclock_rec_pll_fops = {
	.open		= mclock_rec_pll_seq_open,
	.release	= mclock_rec_pll_seq_release,
	.read		= seq_read,
	.llseek		= seq_lseek,
};

void mclock_rec_pll_debugfs_init(struct mclock_drv *drv, struct mclock_rec_pll *rec, int domain)
{
	char name[32];

	if (!drv->mclock_dentry)
		goto out;

	snprintf(name, 32, "rec_pll_%d", domain);

	debugfs_create_file(name, S_IRUSR, drv->mclock_dentry, rec, &mclock_rec_pll_fops);

out:
	return;
}

static int mclock_dma_show(struct seq_file *s, void *data)
{
	struct mclock_dma_stats *stats = s->private;

	seq_printf(s, "init         = %u\n", stats->init);
	seq_printf(s, "start        = %u\n", stats->start);
	seq_printf(s, "stop         = %u\n", stats->stop);
	seq_printf(s, "DMA clean    = %u\n", stats->dma_clean);
	seq_printf(s, "TS freq      = %u / %u Hz\n", stats->ts_freq_p, stats->ts_freq_q);
	seq_printf(s, "init error   = %u\n", stats->err_init);
	seq_printf(s, "drift error  = %u\n", stats->err_drift);

	return 0;
}

static int mclock_dma_seq_open(struct inode *inode, struct file *file)
{
		return single_open(file, mclock_dma_show, inode->i_private);
}

static const struct file_operations mclock_dma_fops = {
	.open		= mclock_dma_seq_open,
	.release	= single_release,
	.read		= seq_read,
	.llseek		= seq_lseek,
};

void mclock_dma_debugfs_init(struct mclock_dma_stats *stats, struct dentry *mclock_dentry, char *dma_name, int domain)
{
	char name[32];

	snprintf(name, 32, "%s_dma_%d", dma_name, domain);

	debugfs_create_file(name, S_IRUSR, mclock_dentry, stats, &mclock_dma_fops);
}

struct dentry *mclock_debugfs_init(struct dentry *avb_dentry)
{
	return debugfs_create_dir("mclock", avb_dentry);
}

static int hw_timer_show(struct seq_file *s, void *data)
{
	struct hw_timer *timer = s->private;
	struct hw_timer_dev *dev;
	struct stats *runtime_stats;
	struct stats *delay_stats;
	int i;

	mutex_lock(&timer->lock);

	if (!timer->dev) {
		seq_printf(s, "No HW timer was registered !! \n");
		goto exit;
	}

	dev = timer->dev;

	runtime_stats = &timer->runtime_stats;
	delay_stats = &timer->delay_stats;

	seq_printf(s, "runtime (ns)\n");
	seq_printf(s, "min         = %u\n", hw_timer_cycles_to_ns(dev, runtime_stats->abs_min));
	seq_printf(s, "max         = %u\n", hw_timer_cycles_to_ns(dev, runtime_stats->abs_max));
	seq_printf(s, "min_cur     = %u\n", hw_timer_cycles_to_ns(dev, runtime_stats->min));
	seq_printf(s, "max_cur     = %u\n", hw_timer_cycles_to_ns(dev, runtime_stats->max));
	seq_printf(s, "mean        = %u\n", hw_timer_cycles_to_ns(dev, runtime_stats->mean));

	seq_printf(s, "\ndelay (ns)\n");
	seq_printf(s, "min         = %u\n", hw_timer_cycles_to_ns(dev, delay_stats->abs_min));
	seq_printf(s, "max         = %u\n", hw_timer_cycles_to_ns(dev, delay_stats->abs_max));
	seq_printf(s, "min_cur     = %u\n", hw_timer_cycles_to_ns(dev, delay_stats->min));
	seq_printf(s, "max_cur     = %u\n", hw_timer_cycles_to_ns(dev, delay_stats->max));
	seq_printf(s, "mean        = %u\n", hw_timer_cycles_to_ns(dev, delay_stats->mean));

	seq_printf(s, "\nmiscs\n");
	seq_printf(s, "recovery    = %u\n", dev->recovery_errors);

	seq_printf(s, "\ntick histogram\n");

	for (i = 0; i < HW_TIMER_MAX_BINS; i++)
		seq_printf(s, "%u ", dev->tick_histogram[i]);

	seq_printf(s, "\n");

	seq_printf(s, "\ndelay histogram (%u ns/bin)\n", 1 << HW_TIMER_DELAY_BIN_WIDTH_NS_SHIFT);

	for (i = 0; i < HW_TIMER_DELAY_MAX_BINS; i++)
		seq_printf(s, "%u ", dev->delay_histogram[i]);

	seq_printf(s, "\n");

exit:
	mutex_unlock(&timer->lock);

	return 0;
}

static int hw_timer_seq_open(struct inode *inode, struct file *file)
{
	struct hw_timer *timer = inode->i_private;

	stats_compute(&timer->runtime_stats);
	stats_reset(&timer->runtime_stats);

	stats_compute(&timer->delay_stats);
	stats_reset(&timer->delay_stats);

	return single_open(file, hw_timer_show, inode->i_private);
}

static int hw_timer_seq_release(struct inode *inode, struct file *file)
{
	struct hw_timer *timer = inode->i_private;
	int rc = single_release(inode, file);

	mutex_lock(&timer->lock);

	if (timer->dev) {
		memset(timer->dev->tick_histogram, 0, sizeof(timer->dev->tick_histogram));
		memset(timer->dev->delay_histogram, 0, sizeof(timer->dev->delay_histogram));
	}

	mutex_unlock(&timer->lock);

	return rc;
}

static const struct file_operations hw_timer_fops = {
	.open		= hw_timer_seq_open,
	.release	= hw_timer_seq_release,
	.read		= seq_read,
	.llseek		= seq_lseek,
};

void hw_timer_debugfs_init(struct hw_timer *timer, struct dentry *avb_dentry)
{
	debugfs_create_file("hw_timer", S_IRUSR, avb_dentry, timer, &hw_timer_fops);
}



/** switch_stats_show - printout statistics to the ssy fs framework
 * \return 0 on success or negative value on failure
 * \param s
 * \param data
 */
static int switch_stats_show(struct seq_file *s, void *data)
{
	struct switch_stats *stats = s->private;

	seq_printf(s, "\nreceive counters:\n");
	seq_printf(s, "rx_from_network      = %lu\n", stats->rx_from_network);
	seq_printf(s, "rx_from_network_ptp  = %lu\n", stats->rx_from_network_ptp);
	seq_printf(s, "rx_from_network_srp  = %lu\n", stats->rx_from_network_srp);
	seq_printf(s, "rx_from_network_meta = %lu\n", stats->rx_from_network_meta);

	seq_printf(s, "rx_to_host_sw  = %lu %lu %lu %lu %lu\n", stats->rx_to_host_sw[0], stats->rx_to_host_sw[1], stats->rx_to_host_sw[2], stats->rx_to_host_sw[3], stats->rx_to_host_sw[4]);
	seq_printf(s, "rx_to_host_sw_ptp  = %lu %lu %lu %lu %lu\n", stats->rx_to_host_sw_ptp[0], stats->rx_to_host_sw_ptp[1], stats->rx_to_host_sw_ptp[2], stats->rx_to_host_sw_ptp[3], stats->rx_to_host_sw_ptp[4]);
	seq_printf(s, "rx_to_host_sw_srp  = %lu %lu %lu %lu %lu\n", stats->rx_to_host_sw_srp[0], stats->rx_to_host_sw_srp[1], stats->rx_to_host_sw_srp[2], stats->rx_to_host_sw_srp[3], stats->rx_to_host_sw_srp[4]);
	seq_printf(s, "rx_to_host_sw_meta = %lu\n", stats->rx_to_host_sw_meta);
	seq_printf(s, "rx_to_host_ep      = %lu\n", stats->rx_to_host_ep);
	seq_printf(s, "rx_to_host_ep_ptp  = %lu\n", stats->rx_to_host_ep_ptp);
	seq_printf(s, "rx_to_host_ep_srp  = %lu\n", stats->rx_to_host_ep_srp);
	seq_printf(s, "rx_to_host_ep_meta = %lu\n", stats->rx_to_host_ep_meta);

	seq_printf(s, "\ntransmit counters:\n");
	seq_printf(s, "tx_from_host_sw      = %lu %lu %lu %lu %lu\n", stats->tx_from_host_sw[0], stats->tx_from_host_sw[1], stats->tx_from_host_sw[2], stats->tx_from_host_sw[3], stats->tx_from_host_sw[4]);
	seq_printf(s, "tx_from_host_sw_ptp  = %lu %lu %lu %lu %lu\n", stats->tx_from_host_sw_ptp[0], stats->tx_from_host_sw_ptp[1], stats->tx_from_host_sw_ptp[2], stats->tx_from_host_sw_ptp[3], stats->tx_from_host_sw_ptp[4]);
	seq_printf(s, "tx_from_host_sw_srp  = %lu %lu %lu %lu %lu\n", stats->tx_from_host_sw_srp[0], stats->tx_from_host_sw_srp[1], stats->tx_from_host_sw_srp[2], stats->tx_from_host_sw_srp[3], stats->tx_from_host_sw_srp[4]);
	seq_printf(s, "tx_from_host_ep      = %lu\n", stats->tx_from_host_ep);
	seq_printf(s, "tx_from_host_ep_ptp  = %lu\n", stats->tx_from_host_ep_ptp);
	seq_printf(s, "tx_from_host_ep_srp  = %lu\n", stats->tx_from_host_ep_srp);

	seq_printf(s, "tx_to_network_ep_ptp  = %lu\n", stats->tx_to_network_ep_ptp);
	seq_printf(s, "tx_to_network_ep_srp  = %lu\n", stats->tx_to_network_ep_srp);

	seq_printf(s, "tx_to_network_sw_ptp  = %lu %lu %lu %lu %lu\n", stats->tx_to_network_sw_ptp[0], stats->tx_to_network_sw_ptp[1], stats->tx_to_network_sw_ptp[2], stats->tx_to_network_sw_ptp[3], stats->tx_to_network_sw_ptp[4]);
	seq_printf(s, "tx_to_network_sw_srp  = %lu %lu %lu %lu %lu\n", stats->tx_to_network_sw_srp[0], stats->tx_to_network_sw_srp[1], stats->tx_to_network_sw_srp[2], stats->tx_to_network_sw_srp[3], stats->tx_to_network_sw_srp[4]);

	seq_printf(s, "\ncommon egress timestamp counters:\n");
	seq_printf(s, "num_egress_ts	      = %lu %lu %lu %lu %lu\n", stats->num_egress_ts[0], stats->num_egress_ts[1], stats->num_egress_ts[2], stats->num_egress_ts[3], stats->num_egress_ts[4]);
	seq_printf(s, "num_egress_ts_alloc    = %lu %lu %lu %lu %lu\n", stats->num_egress_ts_alloc[0], stats->num_egress_ts_alloc[1], stats->num_egress_ts_alloc[2], stats->num_egress_ts_alloc[3], stats->num_egress_ts_alloc[4]);
	seq_printf(s, "num_egress_ts_free     = %lu %lu %lu %lu %lu\n", stats->num_egress_ts_free[0], stats->num_egress_ts_free[1], stats->num_egress_ts_free[2], stats->num_egress_ts_free[3], stats->num_egress_ts_free[4]);
	seq_printf(s, "egress_ts_queue_full	  = %lu\n", stats->egress_ts_queue_full);
	seq_printf(s, "egress_ts_queue_lvl	  = %lu\n", stats->egress_ts_queue_lvl);
	seq_printf(s, "egress_ts_queue_count  = %lu\n", stats->egress_ts_queue_count);
	seq_printf(s, "egress_ts_dequeue_count  = %lu\n", stats->egress_ts_dequeue_count);

	seq_printf(s, "\ncommon receive queue counters:\n");
	seq_printf(s, "rx_queue_lvl       = %lu\n", stats->rx_queue_lvl);
	seq_printf(s, "rx_queue_full      = %lu\n", stats->rx_queue_full);
	seq_printf(s, "rx_enqueue_count   = %lu\n", stats->rx_enqueue_count);
	seq_printf(s, "rx_dequeue_count   = %lu\n", stats->rx_dequeue_count);

	seq_printf(s, "\ntransmit queue counters:\n");
	seq_printf(s, "tx_queue_ep_lvl    = %lu\n", stats->tx_queue_ep_lvl);
	seq_printf(s, "tx_queue_ep_full   = %lu\n", stats->tx_queue_ep_full);
	seq_printf(s, "tx_queue_ep_count  = %lu\n", stats->tx_queue_ep_count);
	seq_printf(s, "tx_queue_sw_lvl    = %lu\n", stats->tx_queue_sw_lvl);
	seq_printf(s, "tx_queue_sw_full   = %lu\n", stats->tx_queue_sw_full);
	seq_printf(s, "tx_queue_sw_count  = %lu\n", stats->tx_queue_sw_count);

	seq_printf(s, "\nallocations counters:\n");
	seq_printf(s, "num_rx_desc_alloc  = %lu\n", stats->num_rx_desc_alloc);
	seq_printf(s, "num_tx_desc_alloc  = %lu\n", stats->num_tx_desc_alloc);
	seq_printf(s, "num_desc_free      = %lu\n", stats->num_desc_free);
	seq_printf(s, "num_desc_free_rx      = %lu\n", stats->num_desc_free_rx);
	seq_printf(s, "num_desc_free_rx_sw   = %lu\n", stats->num_desc_free_rx_sw);
	seq_printf(s, "num_desc_free_rx_ep   = %lu\n", stats->num_desc_free_rx_ep);
	seq_printf(s, "num_desc_free_tx   = %lu\n", stats->num_desc_free_tx);

	seq_printf(s, "\nerrors counters:\n");
	seq_printf(s, "num_alloc_err      = %lu\n", stats->num_alloc_err);
	seq_printf(s, "num_egress_ts_alloc_err = %lu %lu %lu %lu %lu\n", stats->num_egress_ts_alloc_err[0], stats->num_egress_ts_alloc_err[1], stats->num_egress_ts_alloc_err[2], stats->num_egress_ts_alloc_err[3], stats->num_egress_ts_alloc_err[4]);
	seq_printf(s, "num_egress_ts_store_err = %lu %lu %lu %lu %lu\n", stats->num_egress_ts_store_err[0], stats->num_egress_ts_alloc_err[1], stats->num_egress_ts_alloc_err[2], stats->num_egress_ts_alloc_err[3], stats->num_egress_ts_alloc_err[4]);
	seq_printf(s, "num_egress_ts_lookup_err = %lu\n", stats->num_egress_ts_lookup_err);


	seq_printf(s, "rx_from_network_err    = %lu\n", stats->rx_from_network_err);
	seq_printf(s, "tx_from_host_sw_err    = %lu %lu %lu %lu %lu\n", stats->tx_from_host_sw_err[0], stats->tx_from_host_sw_err[1], stats->tx_from_host_sw_err[2], stats->tx_from_host_sw_err[3], stats->tx_from_host_sw_err[4]);
	seq_printf(s, "tx_from_host_ep_err    = %lu\n", stats->tx_from_host_ep_err);

	seq_printf(s, "tx_to_network_ep_ptp_err  = %lu\n", stats->tx_to_network_ep_ptp_err);
	seq_printf(s, "tx_to_network_ep_srp_err  = %lu\n", stats->tx_to_network_ep_srp_err);

	seq_printf(s, "tx_to_network_sw_ptp_err  = %lu %lu %lu %lu %lu\n", stats->tx_to_network_sw_ptp_err[0], stats->tx_to_network_sw_ptp_err[1], stats->tx_to_network_sw_ptp_err[2], stats->tx_to_network_sw_ptp_err[3], stats->tx_to_network_sw_ptp_err[4]);
	seq_printf(s, "tx_to_network_sw_srp_err  = %lu %lu %lu %lu %lu\n", stats->tx_to_network_sw_srp_err[0], stats->tx_to_network_sw_srp_err[1], stats->tx_to_network_sw_srp_err[2], stats->tx_to_network_sw_srp_err[3], stats->tx_to_network_sw_srp_err[4]);

	seq_printf(s, "num_sja_tick_err  = %lu\n", stats->num_sja_tick_err);

	seq_printf(s, "\n");

	return 0;
}



/** switch_stats_seq_open -
 * \return 0 on success or negative value on failure
 * \param inode
 * \param file
 */
static int switch_stats_seq_open(struct inode *inode, struct file *file)
{
	return single_open(file, switch_stats_show, inode->i_private);
}


static const struct file_operations switch_fops = {
	.open		= switch_stats_seq_open,
	.release	= single_release,
	.read		= seq_read,
	.llseek		= seq_lseek,
};


/** sja1105_debugfs_init -
 * \return none
 * \param avb_dentry
 */
void switch_debugfs_init(struct switch_drv *drv, struct dentry *avb_dentry)
{
	struct dentry *switch_dentry;

	switch_dentry = debugfs_create_dir("switch", avb_dentry);
	if (!switch_dentry)
		return;

	debugfs_create_file("stats", S_IRUSR, switch_dentry, &drv->stats, &switch_fops);
}


struct dentry *avb_debugfs_init(void)
{
	return debugfs_create_dir("avb", NULL);
}

void avb_debugfs_exit(struct dentry *dentry)
{
	debugfs_remove_recursive(dentry);
}
