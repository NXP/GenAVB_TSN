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

#ifndef _RTP_H_
#define _RTP_H_

#include "net_socket.h"
#include "net_rx.h"
#include "netdrv.h"

/* histogram slot width (in ns) */
#define NET_STATS_RTP_DT_BIN_WIDTH	50000

#define RTP_STREAM_HASH		16

struct rtp_stream_rx_hdlr {
	struct hlist_head sock_head[RTP_STREAM_HASH];
	unsigned int count;
};

int rtp_socket_unbind(void *l5_rx_hdlr, struct net_socket *sock);
int rtp_socket_bind(void *l5_rx_hdlr, struct net_socket *sock, struct net_address *addr);
int rtp_rx(struct eth_avb *eth, void *l5_rx_hdlr, struct net_rx_desc *desc, void *hdr, struct net_rx_stats *stats);

#endif /* _RTP_H_ */
