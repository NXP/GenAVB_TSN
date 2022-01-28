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
 @brief MRP management functions
 @details
*/

#include "genavb/qos.h"

#include "mrp.h"
#include "net_port.h"
#include "net_logical_port.h"
#include "clock.h"
#include "net_tx.h"
#include "bit.h"

#include "os/net.h"

static struct generic_rx_hdlr mrp_rx_hdlr[CFG_LOGICAL_NUM_PORT];

static int mrp_tx(struct net_socket *sock, struct logical_port *port, unsigned long *addr, unsigned int *n);

void mrp_socket_unbind(struct net_socket *sock)
{
	generic_unbind(mrp_rx_hdlr, sock);
}

int mrp_socket_bind(struct net_socket *sock, struct net_address *addr)
{
	return generic_bind(mrp_rx_hdlr, sock, addr->port);
}

void mrp_socket_disconnect(struct logical_port *port, struct net_socket *sock)
{
	struct net_port *net_port = port->phys;

	qos_queue_disconnect(net_port->qos, sock->tx.qos_queue);

	sock->tx.func = NULL;
}

int mrp_socket_connect(struct logical_port *port, struct net_socket *sock, struct net_address *addr)
{
	struct net_port *net_port = port->phys;

	if (port->is_bridge)
		goto err;

	/* Both SRP and PTP packets need to be strictly serialized when transmitted to network
	through the SJA1105 Host interface (for proper SJA1105 HW route management processing)
	So here SRP protocol is defined as 'connected' and assigned to the same high priority queue as PTP
	*/
	sock->tx.qos_queue = qos_queue_connect(net_port->qos, MRP_DEFAULT_PRIORITY, &sock->tx.queue, 0);
	if (!sock->tx.qos_queue)
		goto err;

	sock->tx.func = mrp_tx;

	return 0;

err:
	return -1;
}

int mrp_rx(struct net_rx_ctx *net, struct net_port *phys_port, struct net_rx_desc *desc, void *hdr)
{
	struct net_rx_stats *stats = &net->ptype_hdlr[PTYPE_MRP].stats[desc->port];
	struct net_socket *sock;
	struct logical_port *port = physical_to_logical_port(phys_port);
	uint64_t gptp_time = 0;
	int rc;

	if (!port)
		goto slow;

	sock = mrp_rx_hdlr[port->id].sock;
	if (!sock)
		goto slow;

	clock_time_from_hw(phys_port->clock_gptp, desc->ts64, &gptp_time);
	desc->ts = gptp_time;
	desc->ts64 = gptp_time;

	rc = socket_rx(net, sock, desc, stats);

	return rc;

slow:
	return net_rx_slow(net, phys_port, desc, stats);
}

static int mrp_tx(struct net_socket *sock, struct logical_port *port, unsigned long *addr, unsigned int *n)
{
	struct net_tx_desc *desc;
	unsigned int flags;
	int rc = 0, i;

	for (i = 0; i < *n; i++) {
		desc = (struct net_tx_desc *)addr[i];

		flags = desc->flags;

		desc->flags = 0;

		if (flags & NET_TX_FLAGS_TS)
			desc->flags |= NET_TX_FLAGS_TS;

		if ((rc = socket_qos_enqueue(sock, port, sock->tx.qos_queue, desc)) < 0)
			break;
	}

	*n = i;

	return rc;
}
