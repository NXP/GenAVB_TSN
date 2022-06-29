/*
 * AVB 1733 management functions
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

#ifndef _IPV4_H_
#define _IPV4_H_

#include <linux/ip.h>
#include "net_port.h"
#include "net_socket.h"
#include "net_logical_port.h"
#include "netdrv.h"
#include "rtp.h"

void ipv4_socket_unbind(struct net_socket *sock);
int ipv4_socket_bind(struct net_socket *sock, struct net_address *addr);
void ipv4_socket_disconnect(struct logical_port *port, struct net_socket *sock);
int ipv4_socket_connect(struct logical_port *port, struct net_socket *sock, struct net_address *addr);

#define IPV4_STREAM_HASH	16

struct ipv4_socket {
	struct hlist_node node;

	u32 saddr;
	u32 daddr;
	u16 sport;
	u16 dport;

	int l5_proto;
	int (*l5_rx)(struct eth_avb *, void *, struct net_rx_desc *, void *, struct net_rx_stats *);

	union {
		struct rtp_stream_rx_hdlr l5_rx_hdlr;

		struct net_socket *sock;
	};
};

struct ipv4_stream_rx_hdlr {
	struct hlist_head sock_head[IPV4_STREAM_HASH];
};

struct ipv4_rx_hdlr {
	struct ipv4_stream_rx_hdlr stream[CFG_PORTS];
};


#ifndef CFG_AVB_IPV4_1733
static inline int ipv4_rx(struct eth_avb *eth, struct net_rx_desc *desc, void *hdr, unsigned int is_vlan)
{
	return -1;
}
#else
int ipv4_rx(struct eth_avb *eth, struct net_rx_desc *desc, void *hdr, unsigned int is_vlan);

extern struct ipv4_rx_hdlr ipv4_rx_hdlr;
#endif


#endif /* _IPV4_H_ */
