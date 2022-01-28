/*
 * AVB 1733 (ipv4) management functions
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

#include "ipv4.h"
#include "rtp.h"

#define NET_STATS_IPV4_DT_BIN_WIDTH_SHIFT		13 /*8192 ns*/

#ifndef CFG_AVB_IPV4_1733
void ipv4_socket_unbind(struct net_socket *sock)
{
}

int ipv4_socket_bind(struct net_socket *sock, struct net_address *addr)
{
	return -1;
}

void ipv4_socket_disconnect(struct logical_port *port, struct net_socket *sock)
{
}


int ipv4_socket_connect(struct logical_port *port, struct net_socket *sock, struct net_address *addr)
{
	return -1;
}

#else

struct ipv4_rx_hdlr ipv4_rx_hdlr;

#ifdef CFG_NET_STREAM_STATS
static void ip_stream_init_stats(struct net_socket *sock)
{
	stream_init_stats(sock);
}

static void ip_stream_update_stats(struct net_socket *sock, struct net_rx_desc *desc)
{
	stream_update_stats(sock, desc, NET_STATS_IPV4_DT_BIN_WIDTH_SHIFT);
}
#else
static void ip_stream_update_stats(struct net_socket *sock, struct net_rx_desc *desc)
{
}

static void ip_stream_init_stats(struct net_socket *sock)
{
}
#endif

static inline int ipv4_stream_match(struct ipv4_socket *sock, u32 daddr, u32 saddr, u16 dport, u16 sport)
{
	if ((sock->daddr == daddr) &&
		(sock->saddr == saddr) &&
		(sock->dport == dport) &&
		(sock->sport == sport))
		return 1;

	return 0;
}

static inline unsigned int ipv4_stream_hash(u32 daddr, u32 saddr, u16 dport, u16 sport)
{
	/* FIXME Needs to be improved */
	return (daddr ^ saddr ^ (dport << 16) ^ sport) & (IPV4_STREAM_HASH - 1);
}

static int ipv4_stream_hdlr_add(struct ipv4_stream_rx_hdlr *hdlr, struct ipv4_socket *sock, u32 daddr, u32 saddr, u16 dport, u16 sport)
{
	unsigned int hash;

	hash = ipv4_stream_hash(daddr, saddr, dport, sport);

	hlist_add_head(&sock->node, &hdlr->sock_head[hash]);

	return 0;
}

static void ipv4_stream_hdlr_del(struct ipv4_stream_rx_hdlr *hdlr, struct ipv4_socket *sock)
{
	hlist_del(&sock->node);
}

static struct ipv4_socket *ipv4_stream_hdlr_find(struct ipv4_stream_rx_hdlr *hdlr, u32 daddr, u32 saddr, u16 dport, u16 sport)
{
	struct hlist_node *node;
	struct ipv4_socket *sock;
	unsigned int hash;

	hash = ipv4_stream_hash(daddr, saddr, dport, sport);

	hlist_for_each(node, &hdlr->sock_head[hash]) {
		sock = hlist_entry(node, struct ipv4_socket, node);

		if (ipv4_stream_match(sock, daddr, saddr, dport, sport))
			return sock;
	}

	return NULL;
}

void ipv4_socket_unbind(struct net_socket *sock)
{
	struct ipv4_socket *ipv4_sock;
	unsigned int del;

	ipv4_sock = ipv4_stream_hdlr_find(&ipv4_rx_hdlr.stream[sock->addr.port], sock->addr.u.ipv4.daddr, sock->addr.u.ipv4.saddr, sock->addr.u.ipv4.dport, sock->addr.u.ipv4.sport);
	if (!ipv4_sock)
		return;

	if (ipv4_sock->l5_rx) {
		del = !rtp_socket_unbind(&ipv4_sock->l5_rx_hdlr, sock);
	} else
		del = 1;

	if (del) {
		ipv4_stream_hdlr_del(&ipv4_rx_hdlr.stream[sock->addr.port], ipv4_sock);
		kfree(ipv4_sock);
	}
}


int ipv4_socket_bind(struct net_socket *sock, struct net_address *addr)
{
	struct ipv4_socket *ipv4_sock;
	unsigned int new = 0;

	if (addr->port >= CFG_PORTS)
		goto err;

	if (addr->u.ipv4.proto != IPPROTO_UDP)
		goto err;

	ipv4_sock = ipv4_stream_hdlr_find(&ipv4_rx_hdlr.stream[addr->port], addr->u.ipv4.daddr, addr->u.ipv4.saddr, addr->u.ipv4.dport, addr->u.ipv4.sport);
	if (!ipv4_sock) {
		new = 1;

		ipv4_sock = kzalloc(sizeof(*ipv4_sock), GFP_ATOMIC);
		if (!ipv4_sock)
			goto err;
	} else {
		if (ipv4_sock->l5_proto != addr->u.ipv4.l5_proto)
			goto err;
	}

	switch (addr->u.ipv4.l5_proto) {
	case L5PROTO_ANY:
		ip_stream_init_stats(sock);
		ipv4_sock->sock = sock;
		break;

	case L5PROTO_RTP:
	case L5PROTO_RTCP:
		if (rtp_socket_bind(&ipv4_sock->l5_rx_hdlr, sock, addr) < 0)
			goto err;

		ipv4_sock->l5_rx = rtp_rx;
		break;

	default:
		goto err;
		break;
	}

	if (new) {
		ipv4_sock->l5_proto = addr->u.ipv4.l5_proto;

		ipv4_sock->daddr = addr->u.ipv4.daddr;
		ipv4_sock->saddr = addr->u.ipv4.saddr;

		ipv4_sock->dport = addr->u.ipv4.dport;
		ipv4_sock->sport = addr->u.ipv4.sport;

		ipv4_stream_hdlr_add(&ipv4_rx_hdlr.stream[addr->port], ipv4_sock, addr->u.ipv4.daddr, addr->u.ipv4.saddr, addr->u.ipv4.dport, addr->u.ipv4.sport);
	}

	return 0;

err:
	if (new && ipv4_sock)
		kfree(ipv4_sock);

	return -1;
}

void ipv4_socket_disconnect(struct logical_port *port, struct net_socket *sock)
{
	struct eth_avb *eth = port->eth;

	switch (sock->addr.u.ipv4.l5_proto) {
	case L5PROTO_RTP:
		net_qos_stream_disconnect(eth->qos, sock->qos_queue);
		sock->qos_queue = NULL;
		break;

	default:
		break;
	}
}


int ipv4_socket_connect(struct logical_port *port, struct net_socket *sock, struct net_address *addr)
{
	struct eth_avb *eth = port->eth;

	if (port->is_bridge)
		goto err;

	if (addr->u.ipv4.proto != IPPROTO_UDP)
		goto err;

	switch (addr->u.ipv4.l5_proto) {
	case L5PROTO_RTP:
		if (!sr_class_enabled(addr->u.ipv4.u.rtp.sr_class))
			goto err;

		sock->qos_queue = net_qos_stream_connect(eth->qos, addr->u.ipv4.u.rtp.sr_class, addr->u.ipv4.u.rtp.stream_id, &sock->queue);
		if (!sock->qos_queue)
			goto err;

		sock->flags |= SOCKET_FLAGS_VLAN;
		sock->vlan_label = sock->qos_queue->stream->vlan_label;

		break;

	case L5PROTO_RTCP:
		break;

	default:
		goto err;
		break;
	}

	return 0;

err:
	return -1;
}

static int udp_rx(struct eth_avb *eth, struct net_rx_desc *desc, struct iphdr *ipv4, struct net_rx_stats *stats)
{
	struct udphdr *udp = (struct udphdr *)(ipv4 + 1);
	struct ipv4_socket *ipv4_sock;
	int rc;

	if (ntohs(udp->len) != (desc->len - (desc->l4_offset - desc->l2_offset) - FCS_LEN))
		goto slow;

	raw_read_lock(&ptype_lock);

	ipv4_sock = ipv4_stream_hdlr_find(&ipv4_rx_hdlr.stream[desc->port], ipv4->saddr, ipv4->daddr, udp->source, udp->dest);
	if (!ipv4_sock)
		goto slow_unlock;

	if (ipv4_sock->l5_rx) {
		desc->l5_offset = desc->l4_offset + sizeof(*udp);

		rc = ipv4_sock->l5_rx(eth, &ipv4_sock->l5_rx_hdlr, desc, udp + 1, stats);
	} else {
		ip_stream_update_stats(ipv4_sock->sock, desc);

		rc = net_rx(eth, ipv4_sock->sock, desc, stats);
	}

	raw_read_unlock(&ptype_lock);

	return rc;

slow_unlock:
	raw_read_unlock(&ptype_lock);

slow:
	return net_rx_slow(eth, desc, stats);
}

int ipv4_rx(struct eth_avb *eth, struct net_rx_desc *desc, void *hdr, unsigned int is_vlan)
{
	struct iphdr *ipv4 = hdr;
	struct net_rx_stats *stats = &ptype_hdlr[PTYPE_IPV4].stats[desc->port];
	unsigned int l2_len, l4_len;

	if (ipv4->version != 4)
		goto slow;

	if (ipv4->ihl != 5)
		goto slow;

	if (ipv4->frag_off & htons(IP_MF | IP_OFFSET))
		goto slow;

	l2_len = desc->l3_offset - desc->l2_offset;
	l4_len = ntohs(ipv4->tot_len);

	if (l4_len > (desc->len - l2_len - FCS_LEN))
		goto slow;

	desc->len = l4_len + l2_len + FCS_LEN;

	if (ipv4->protocol != IPPROTO_UDP)
		goto slow;

	desc->l4_offset = desc->l3_offset + sizeof(struct iphdr);

	return udp_rx(eth, desc, ipv4, stats);

slow:
	return net_rx_slow(eth, desc, stats);
}

#endif
