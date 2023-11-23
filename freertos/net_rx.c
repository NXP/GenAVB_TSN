/*
* Copyright 2017-2020, 2023 NXP
*
* SPDX-License-Identifier: BSD-3-Clause
*/

/**
 @file
 @brief FreeRTOS specific Network service implementation
 @details
*/

#include "genavb/ether.h"

#include "os/net.h"
#include "os/sys_types.h"

#include "net_logical_port.h"
#include "net_rx.h"
#include "net_port.h"
#include "ptp.h"
#include "mrp.h"
#include "avtp.h"
#include "hsr.h"
#include "l2.h"

struct net_rx_ctx net_rx_ctx;
static struct generic_rx_hdlr other_rx_hdlr[CFG_ENDPOINT_NUM];

int other_socket_bind(struct net_socket *sock, struct net_address *addr)
{
	return generic_bind(other_rx_hdlr, sock, addr->port, CFG_ENDPOINT_NUM);
}

void other_socket_unbind(struct net_socket *sock)
{
	generic_unbind(other_rx_hdlr, sock, CFG_ENDPOINT_NUM);
}

int net_rx_slow(struct net_rx_ctx *net, struct logical_port *port, struct net_rx_desc *desc, struct net_rx_stats *stats)
{
	struct net_socket *sock;
	int rc;

	if (!port)
		goto drop;

	if (port->is_bridge)
		goto drop;

	sock = other_rx_hdlr[port->id].sock;
	if (!sock)
		goto drop;

	rc = socket_rx(net, sock, desc, stats);
	if (rc >= 0)
		return AVB_NET_RX_SLOW;
	else
		return rc;

drop:
	return net_rx_drop(net, desc, stats);
}

int net_rx_drop(struct net_rx_ctx *net, struct net_rx_desc *desc, struct net_rx_stats *stats)
{
	stats->dropped++;
	net_rx_free(desc);

	return AVB_NET_RX_DROP;
}

static inline int vlan_rx(struct net_rx_ctx *net, struct logical_port *port, struct net_rx_desc *desc, void *hdr)
{
	struct vlanhdr *vlan = hdr;
	unsigned int ether_type = vlan->type;

	desc->vid = VLAN_VID(vlan);

	desc->l3_offset = desc->l2_offset + sizeof(struct eth_hdr) + sizeof(struct vlanhdr);
	if (likely(ether_type == htons(ETHERTYPE_AVTP)))
		return avtp_rx(net, port, desc, vlan + 1, 1);

	return l2_rx(net, port, desc);
}

int eth_rx(struct net_rx_ctx *net, struct net_rx_desc *desc, struct net_port *phys_port)
{
	struct logical_port *port = physical_to_logical_port(phys_port);
	struct eth_hdr *ethhdr = (struct eth_hdr *)((uint8_t *)desc + desc->l2_offset);
	unsigned int ether_type = ethhdr->type;

	desc->port = phys_port->index;

	if (likely(ether_type == htons(ETHERTYPE_VLAN)))
		return vlan_rx(net, port, desc, ethhdr + 1);

	desc->vid = VLAN_VID_NONE;
	desc->ethertype = ntohs(ether_type);
	desc->l3_offset = desc->l2_offset + sizeof(struct eth_hdr);

	switch (desc->ethertype) {
	case ETHERTYPE_PTP:
		return ptp_rx(net, port, desc, ethhdr + 1);
		break;
	case ETHERTYPE_AVTP:
		return avtp_rx(net, port, desc, ethhdr + 1, 0);
		break;
	case ETHERTYPE_MSRP:
	case ETHERTYPE_MVRP:
	case ETHERTYPE_MMRP:
		return mrp_rx(net, port, desc, ethhdr + 1);
		break;
	case ETHERTYPE_HSR_SUPERVISION:
		return hsr_rx(net, port, desc);
		break;

	default:
		break;
	}

	return l2_rx(net, port, desc);
}

void net_rx_flush(struct net_rx_ctx *net, struct net_port *port)
{
}
