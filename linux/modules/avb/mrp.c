/*
 * MRP management functions
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

#include "genavb/qos.h"

#include "mrp.h"
#include "net_bridge.h"
#include "net_socket.h"
#include "net_logical_port.h"

static struct generic_rx_hdlr mrp_rx_hdlr[CFG_LOGICAL_NUM_PORT];

void mrp_socket_unbind(struct net_socket *sock)
{
	generic_unbind(mrp_rx_hdlr, sock, CFG_LOGICAL_NUM_PORT);
}

int mrp_socket_bind(struct net_socket *sock, struct net_address *addr)
{
	return generic_bind(mrp_rx_hdlr, sock, addr->port, CFG_LOGICAL_NUM_PORT);
}

void mrp_socket_disconnect(struct logical_port *port, struct net_socket *sock)
{
	struct eth_avb *eth = port->eth;

	sock->tx_func = NULL;

	qos_queue_disconnect(eth->qos, sock->qos_queue);
	sock->qos_queue = NULL;
}

static int mrp_tx(struct net_socket *sock, struct logical_port *port, unsigned long *desc_array, unsigned int *n)
{
	struct avb_tx_desc *desc;
	int rc = 0, i;

	for (i = 0; i < *n; i++) {
		desc = (void *)desc_array[i];

		desc->common.flags = 0;

#if defined(CONFIG_GENAVB_HYBRID) || defined(CONFIG_GENAVB_BRIDGE)
		if (bridge_xmit(port, desc, PTYPE_MRP) < 0) {
			rc = -EIO;
			break;
		}
#else
		if ((rc = socket_qos_enqueue(sock, sock->qos_queue, desc)) < 0)
			break;
#endif
	}

	*n = i;

	return rc;
}
int mrp_socket_connect(struct logical_port *port, struct net_socket *sock, struct net_address *addr)
{
	struct eth_avb *eth = port->eth;

	/* Both SRP and PTP packets need to be strictly serialized when transmitted to network
	through the SJA1105 Host interface (for proper SJA1105 HW route management processing)
	So here SRP protocol is defined as 'connected' and assigned to the same high priority queue as PTP
	*/
	sock->qos_queue = qos_queue_connect(eth->qos, MRP_DEFAULT_PRIORITY, &sock->queue, 0);
	if (!sock->qos_queue)
		goto err;

	sock->tx_func = mrp_tx;

	return 0;
err:
	return -1;
}

#if defined(CONFIG_GENAVB_HYBRID) || defined(CONFIG_GENAVB_BRIDGE)
int __mrp_rx(struct logical_port *port, struct net_rx_desc *desc)
{
	struct eth_avb *eth = port->eth;
	struct net_rx_stats *stats = &ptype_hdlr[PTYPE_MRP].stats[eth->port];
	struct net_socket *nsock[SOCKET_MAX_RX];
	struct net_socket *sock;
	unsigned long flags;
	unsigned int n = 0;
	int rc;

	raw_spin_lock_irqsave(&ptype_lock, flags);

	desc->port = port->id;

	sock = mrp_rx_hdlr[port->id].sock;
	if (!sock)
		goto slow;

	rc = net_rx(eth, sock, desc, stats);

	generic_rx_wakeup(sock, nsock, &n);

	raw_spin_unlock_irqrestore(&ptype_lock, flags);

	if (n) {
		wake_up(&nsock[0]->wait);
		clear_bit(SOCKET_ATOMIC_FLAGS_BUSY, &nsock[0]->atomic_flags);
	}

	return rc;

slow:
	raw_spin_unlock_irqrestore(&ptype_lock, flags);

	return net_rx_slow(eth, desc, stats);
}

int mrp_rx(struct eth_avb *eth, struct net_rx_desc *desc, void *hdr)
{
	struct net_rx_stats *stats = &ptype_hdlr[PTYPE_MRP].stats[desc->port];
	int rc = AVB_NET_RX_OK;

	if (bridge_receive(eth, desc, PTYPE_MRP) < 0)
		rc = net_rx_drop(eth, desc, stats);

	return rc;
}

#else
int mrp_rx(struct eth_avb *eth, struct net_rx_desc *desc, void *hdr)
{
	struct net_rx_stats *stats = &ptype_hdlr[PTYPE_MRP].stats[desc->port];
	struct net_socket *sock;
	struct logical_port *port = physical_to_logical_port(eth);
	unsigned long flags;
	int rc;

	raw_spin_lock_irqsave(&ptype_lock, flags);

	sock = mrp_rx_hdlr[port->id].sock;
	if (!sock)
		goto slow;

	rc = net_rx(eth, sock, desc, stats);

	raw_spin_unlock_irqrestore(&ptype_lock, flags);

	return rc;

slow:
	raw_spin_unlock_irqrestore(&ptype_lock, flags);

	return net_rx_slow(eth, desc, stats);
}
#endif

