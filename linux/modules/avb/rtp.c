/*
 * AVB 1733 (rtp) management functions
 * Copyright 2015 Freescale Semiconductor, Inc.
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

#include <net/ip.h>
#include <linux/udp.h>

#include "genavb/rtp.h"
#include "genavb/sr_class.h"

#include "rtp.h"
#include "netdrv.h"

#ifdef CFG_NET_STREAM_STATS
static void rtp_stream_init_stats(struct net_socket *sock)
{
	stream_init_stats(sock);

	sock->rtp.dt_min = 0x7fffffff;
	sock->rtp.dt_max = -0x7fffffff;

	memset(sock->rtp.dt_bin, 0, sizeof(sock->rtp.dt_bin));
}

static void rtp_stream_update_stats(struct net_socket *sock, struct net_rx_desc *desc)
{
	struct rtp_hdr *rtp = (void *)desc + desc->l5_offset;
	int dt;
	unsigned int bin, rtp_timestamp;

	if (sock->addr.u.ipv4.l5_proto == L5PROTO_RTP) {
		rtp_timestamp = ntohl(rtp->ts);

		if (sock->rx) {
			dt = (int)rtp_timestamp - (int)sock->rtp.timestamp;

			if (dt < sock->rtp.dt_min)
				sock->rtp.dt_min = dt;
			else if (dt > sock->rtp.dt_max)
				sock->rtp.dt_max = dt;

			if (dt < 0)
				bin = 0;
			else {
				bin = 1 + dt / NET_STATS_RTP_DT_BIN_WIDTH;

				if (bin >= NET_STATS_BIN_MAX)
					bin = NET_STATS_BIN_MAX - 1;
			}

			sock->rtp.dt_bin[bin]++;
		}

		sock->rtp.timestamp = rtp_timestamp;
	}

	stream_update_stats(sock, desc, dt_bin_width_shift[sock->addr.u.ipv4.u.rtp.sr_class]);
}
#else
static void rtp_stream_update_stats(struct net_socket *sock, struct net_rx_desc *desc)
{
}

static void rtp_stream_init_stats(struct net_socket *sock)
{
}
#endif

static inline int rtp_stream_match(struct net_socket *sock, u8 pt, u32 ssrc)
{
	if ((sock->addr.u.ipv4.u.rtp.ssrc == ssrc) && (sock->addr.u.ipv4.u.rtp.pt == pt))
		return 1;

	return 0;
}

static inline unsigned int rtp_stream_hash(u8 pt, u32 ssrc)
{
	/* Needs to be improved */
	return (pt ^ ssrc) & (RTP_STREAM_HASH - 1);
}

static int rtp_stream_hdlr_add(struct rtp_stream_rx_hdlr *hdlr, struct net_socket *sock, u8 pt, u32 ssrc)
{
	unsigned int hash;

	hash = rtp_stream_hash(pt, ssrc);

	hlist_add_head(&sock->node, &hdlr->sock_head[hash]);

	rtp_stream_init_stats(sock);

	return 0;
}

static void rtp_stream_hdlr_del(struct net_socket *sock)
{
	hlist_del(&sock->node);
}

static struct net_socket *rtp_stream_hdlr_find(struct rtp_stream_rx_hdlr *hdlr, u8 pt, u32 ssrc)
{
	struct hlist_node *node;
	struct net_socket *sock;
	unsigned int hash;

	hash = rtp_stream_hash(pt, ssrc);

	hlist_for_each(node, &hdlr->sock_head[hash]) {
		sock = hlist_entry(node, struct net_socket, node);

		if (rtp_stream_match(sock, pt, ssrc))
			return sock;
	}

	return NULL;
}

int rtp_socket_unbind(void *l5_rx_hdlr, struct net_socket *sock)
{
	struct rtp_stream_rx_hdlr *hdlr = l5_rx_hdlr;
	int rc = 0;

	switch (sock->addr.u.ipv4.l5_proto) {
	case L5PROTO_RTP:
	case L5PROTO_RTCP:
		hdlr->count--;
		rtp_stream_hdlr_del(sock);
		rc = hdlr->count;
		break;

	default:
		break;
	}

	return rc;
}


int rtp_socket_bind(void *l5_rx_hdlr, struct net_socket *sock, struct net_address *addr)
{
	struct rtp_stream_rx_hdlr *hdlr = l5_rx_hdlr;

	switch (addr->u.ipv4.l5_proto) {
	case L5PROTO_RTP:
	case L5PROTO_RTCP:
		if (!sr_class_enabled(addr->u.ipv4.u.rtp.sr_class))
			goto err;

		if (rtp_stream_hdlr_find(hdlr, addr->u.ipv4.u.rtp.pt, addr->u.ipv4.u.rtp.ssrc))
			goto err;

		rtp_stream_hdlr_add(hdlr, sock, addr->u.ipv4.u.rtp.pt, addr->u.ipv4.u.rtp.ssrc);
		hdlr->count++;

		break;

	default:
		goto err;
		break;
	}

	return 0;

err:
	return -1;
}

int rtcp_rx(struct eth_avb *eth, struct net_rx_desc *desc)
{
	return 0;
}

int rtp_rx(struct eth_avb *eth, void *l5_rx_hdlr, struct net_rx_desc *desc, void *hdr, struct net_rx_stats *stats)
{
	struct rtp_stream_rx_hdlr *hdlr = l5_rx_hdlr;
	struct rtp_hdr *rtp = hdr;
	struct net_socket *sock;

	sock = rtp_stream_hdlr_find(hdlr, rtp->pt, rtp->ssrc);
	if (!sock)
		goto slow;

	rtp_stream_update_stats(sock, desc);

	return net_rx(eth, sock, desc, stats);

slow:
	return net_rx_slow(eth, desc, stats);
}
