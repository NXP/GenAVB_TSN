/*
 * AVB 1733 (ipv6) management functions
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

#include "ipv6.h"


#ifndef CFG_AVB_IPV6_1733
void ipv6_socket_unbind(struct net_socket *sock)
{
}

int ipv6_socket_bind(struct net_socket *sock, struct net_address *addr)
{
	return -1;
}

void ipv6_socket_disconnect(struct logical_port *port, struct net_socket *sock)
{
}

int ipv6_socket_connect(struct logical_port *port, struct net_socket *sock, struct net_address *addr)
{
	return -1;
}

#else

struct ipv6_rx_hdlr ipv6_rx_hdlr[CFG_PORTS];

void ipv6_socket_unbind(struct net_socket *sock)
{
	stream_hdlr_del(ptype_hdlr[sock->ptype].hdlr[sock->port], dev);
}


int ipv6_socket_bind(struct net_socket *sock, struct net_address *addr)
{
	if (stream_ipv6_1733_hdlr_find(ptype_hdlr[addr->ptype].hdlr[addr->port], addr->ipv6.daddr, addr->ipv6.saddr, addr->ipv4.sport, addr->ipv4.dport, addr->ipv4.rtp.ssrc))
		goto err;

	stream_ipv6_1733_hdlr_add(ptype_hdlr[addr->ptype].hdlr[addr->port], dev, addr->port, &addr->avtp_data.id);

	return 0;

err:
	return -1;
}

void ipv6_socket_disconnect(struct logical_port *port, struct net_socket *sock)
{
	struct eth_avb *eth = port->eth;

	net_qos_stream_disconnect(eth->qos, sock->qos_queue);
	sock->qos_queue = NULL;
}


int ipv6_socket_connect(struct logical_port *port, struct net_socket *sock, struct net_address *addr)
{
	struct eth_avb *eth = port->eth;

	if (port->is_bridge)
		return -1;

	sock->qos_queue = net_qos_stream_connect(eth->qos, addr->ipv4.class, addr->ipv4.id, &sock->queue);
	if (!sock->qos_queue)
		goto err;

	sock->flags |= SOCKET_FLAGS_VLAN;
	sock->vlan_label = sock->qos_queue->stream->vlan_label;

	return 0;

err:
	return -1;
}

static inline int ipv6_stream_match(struct net_socket *sock, u32 *daddr, u32 *saddr, u16 dport, u16 sport, u32 ssrc)
{
	/* FIXME */
	return 0;
}

static inline unsigned int ipv6_stream_hash(u32 *daddr, u32 *saddr, u16 dport, u16 sport, u32 ssrc)
{
	/* FIXME */
	return 0;
}

static int ipv6_stream_hdlr_add(struct stream_handler *hdlr, struct net_socket *sock, unsigned int port, u32 *daddr, u32 *saddr, u16 dport, u16 sport, u32 ssrc)
{
	unsigned int hash;

	hash = ipv6_stream_hash(daddr, saddr, dport, sport, ssrc);

	hlist_add_head(&sock->node, &hdlr->dev_head[hash]);

	stream_init_stats(dev);

	return 0;
}

static void ipv6_stream_hdlr_del(struct stream_handler *hdlr, struct net_socket *sock)
{
	hlist_del(&sock->node);
}

static struct net_dev *ipv6_stream_hdlr_find(struct stream_handler *hdlr, struct ipv6hdr *ip)
{
	struct udphdr *udp = (struct udphdr *)(ip + 1);
	struct rtphdr *rtp = (struct rtphdr *)(udp + 1);
	struct hlist_node *node;
	struct net_socket *sock;
	unsigned int hash;

	hash = ipv6_stream_hash(ip->daddr, ip->saddr, udp->dest, udp->source, rtp->ssrc);

	hlist_for_each(node, &hdlr->dev_head[hash]) {
		dev = hlist_entry(node, struct net_dev, node);

		if (ipv6_stream_match(dev, ip->daddr, ip->saddr, udp->dest, udp->source, rtp->ssrc))
			return dev;
	}

	return NULL;
}

int ipv6_rx(struct eth_avb *eth, struct net_rx_desc *desc, void *hdr, unsigned int is_vlan)
{
	struct ipv6hdr *ipv6 = hdr;

	return -1;
}

#endif
