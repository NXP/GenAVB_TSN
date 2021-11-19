/*
 * AVB ethernet rx/tx functions
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

#include "net_rx.h"

#include "genavb/ether.h"

#include "sja1105.h"
#include "ptp.h"
#include "mrp.h"
#include "avtp.h"
#include "ipv4.h"
#include "ipv6.h"

struct ptype_handler ptype_hdlr[PTYPE_MAX];

raw_rwlock_t ptype_lock;

int net_rx_thread(struct net_drv *drv)
{
	struct net_socket *sock[SOCKET_MAX_RX];
	unsigned int i, n = 0;

//	pr_info("%s\n", __func__);

	raw_read_lock(&ptype_lock);

	generic_rx_wakeup_all(drv, sock, &n);

	raw_read_unlock(&ptype_lock);

	for (i = 0; i < n; i++) {
		wake_up(&sock[i]->wait);
		clear_bit(SOCKET_ATOMIC_FLAGS_BUSY, &sock[i]->atomic_flags);
	}

	return 0;
}

int net_tx_thread(struct eth_avb *eth)
{
	struct logical_port *port = physical_to_logical_port(eth);

//	pr_info("%s\n", __func__);

	ptp_tx_ts_wakeup(port);

	return 0;
}

int net_tx_available_thread(struct eth_avb *eth)
{
	struct net_socket *sock[SOCKET_MAX_RX];
	unsigned int i, n = 0;

	raw_read_lock(&eth->lock);

	avtp_tx_wakeup(eth, sock, &n);

	raw_read_unlock(&eth->lock);

	for (i = 0; i < n; i++)
		wake_up(&sock[i]->wait);

	return 0;
}

int net_rx_slow(struct eth_avb *eth, struct net_rx_desc *desc, struct net_rx_stats *stats)
{
	if (queue_enqueue(&eth->rx_queue, (unsigned long)desc) < 0) {
		stats->slow_dropped++;
		pool_dma_free(eth->buf_pool, desc);
	} else
		stats->slow++;

	return AVB_NET_RX_SLOW;
}

int net_rx_drop(struct eth_avb *eth, struct net_rx_desc *desc, struct net_rx_stats *stats)
{
	stats->dropped++;
	pool_dma_free(eth->buf_pool, desc);

	return AVB_NET_RX_DROP;
}

static inline int vlan_rx(struct eth_avb *eth, struct net_rx_desc *desc, void *hdr)
{
	struct vlan_hdr *vlan = hdr;
	unsigned int ether_type = vlan->type;

	desc->vid = VLAN_VID(vlan);

	desc->l3_offset = desc->l2_offset + sizeof(struct eth_hdr) + sizeof(struct vlan_hdr);

	if (likely(ether_type == htons(ETHERTYPE_AVTP)))
		return avtp_rx(eth, desc, vlan + 1, 1);

	switch (ether_type) {
	case htons(ETHERTYPE_IPV4):
		return ipv4_rx(eth, desc, vlan + 1, 1);
		break;

	case htons(ETHERTYPE_IPV6):
		return ipv6_rx(eth, desc, vlan + 1, 1);
		break;

	default:
		break;
	}

	return net_rx_slow(eth, desc, &ptype_hdlr[PTYPE_OTHER].stats[desc->port]);
}

int eth_rx(struct eth_avb *eth, struct net_rx_desc *desc)
{
	struct eth_hdr *ethhdr = (void *)desc + desc->l2_offset;
	unsigned int ether_type = ethhdr->type;

	desc->port = eth->port;

	if (likely(ether_type == htons(ETHERTYPE_VLAN)))
		return vlan_rx(eth, desc, ethhdr + 1);

	desc->ethertype = ntohs(ether_type);
	desc->l3_offset = desc->l2_offset + sizeof(struct eth_hdr);

	switch (desc->ethertype) {
	case ETHERTYPE_AVTP:
		return avtp_rx(eth, desc, ethhdr + 1, 0);
		break;

	case ETHERTYPE_PTP:
		return ptp_rx(eth, desc, ethhdr + 1);
		break;

#if defined (CONFIG_SJA1105)
	case ETHERTYPE_SJAMETA: /* not really en ethertype, 802.3 frame with len field of 8 bytes */
		return sja1105_metaframe_rx(eth, desc);
		break;
#endif

	case ETHERTYPE_MSRP:
	case ETHERTYPE_MVRP:
	case ETHERTYPE_MMRP:
		return mrp_rx(eth, desc, ethhdr + 1);
		break;

	default:
		break;
	}

	return net_rx_slow(eth, desc, &ptype_hdlr[PTYPE_OTHER].stats[desc->port]);
}
