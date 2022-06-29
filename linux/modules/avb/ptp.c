/*
 * PTP management functions
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
#include <linux/sched.h>
#include <linux/fec.h>

#include "net_bridge.h"
#include "ptp.h"
#include "net_socket.h"
#include "net_logical_port.h"

static struct generic_rx_hdlr ptp_rx_hdlr[CFG_LOGICAL_NUM_PORT];

void ptp_socket_unbind(struct net_socket *sock)
{
	generic_unbind(ptp_rx_hdlr, sock, CFG_LOGICAL_NUM_PORT);
}

int ptp_socket_bind(struct net_socket *sock, struct net_address *addr)
{
	if (addr->u.ptp.version != 2)
		return -1;

	return generic_bind(ptp_rx_hdlr, sock, addr->port, CFG_LOGICAL_NUM_PORT);
}

void ptp_socket_disconnect(struct logical_port *port, struct net_socket *sock)
{
	struct eth_avb *eth = port->eth;

	sock->tx_func = NULL;

	/* FIXME flush transmit queue */
	if (sock->flags & SOCKET_FLAGS_TX_TS_ENABLED) {
		/* FIXME receive timestamp queue */
		sock->flags &= ~SOCKET_FLAGS_TX_TS_ENABLED;
		port->net_sock = NULL;

		qos_queue_disconnect(eth->qos, sock->qos_queue);
		sock->qos_queue = NULL;
	}
}


static int ptp_tx(struct net_socket *sock, struct logical_port *port, unsigned long *desc_array, unsigned int *n)
{
	struct avb_tx_desc *desc;
	unsigned int flags;
	int rc = 0, i;

	for (i = 0; i < *n; i++) {
		desc = (void *)desc_array[i];

		flags = desc->common.flags;

		desc->common.flags = 0;

		if (flags & NET_TX_FLAGS_HW_TS) {
			if (!(sock->flags & SOCKET_FLAGS_TX_TS_ENABLED)) {
				rc = -EINVAL;
				break;
			}

			desc->common.flags |= AVB_TX_FLAG_HW_TS;
		}

#if defined(CONFIG_GENAVB_HYBRID) || defined(CONFIG_GENAVB_BRIDGE)
		if (bridge_xmit(port, desc, PTYPE_PTP) < 0) {
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

int ptp_socket_connect(struct logical_port *port, struct net_socket *sock, struct net_address *addr)
{
	struct eth_avb *eth = port->eth;

	//pr_info("%s: eth=%p qos=%p sock=%p port=%d eth->net_sock=%p\n", __func__, eth, eth->qos, sock, addr->port, eth->net_sock);

	if (port->net_sock)
		goto err;

	sock->qos_queue = qos_queue_connect(eth->qos, addr->priority, &sock->queue, 0);
	if (!sock->qos_queue)
		goto err;

	sock->tx_func = ptp_tx;
	sock->flags |= SOCKET_FLAGS_TX_TS_ENABLED;
	port->net_sock = sock;

	return 0;

err:
	return -1;
}

void ptp_tx_ts_wakeup(struct logical_port *port)
{
	struct eth_avb *eth = port->eth;
	struct net_socket *sock;
	unsigned long flags;

	raw_spin_lock_irqsave(&eth->lock, flags);

	sock = port->net_sock;

	/* check hw ts flag was set for this packet */
	if (sock && (sock->flags & SOCKET_FLAGS_TX_TS_ENABLED)) {
		/* enqueue ts to upper layer */
		if (queue_pending(&sock->tx_ts_queue)) {
			raw_spin_unlock_irqrestore(&eth->lock, flags);

			wake_up(&sock->wait);
		} else
			raw_spin_unlock_irqrestore(&eth->lock, flags);
	} else
		raw_spin_unlock_irqrestore(&eth->lock, flags);

}

static int __ptp_tx_ts(struct logical_port *port, struct net_tx_desc *desc)
{
	struct eth_avb *eth = port->eth;
	struct net_socket *sock;
	int rc = 0;

	//pr_info("%s: port %d ts %lu private 0%08x\n", __func__, eth->port, desc->ts, desc->private);

	sock = port->net_sock;

	/* check hw ts flag was set for this packet */
	if (sock && (sock->flags & SOCKET_FLAGS_TX_TS_ENABLED)) {

		/* enqueue ts to upper layer */
		if (queue_enqueue(&sock->tx_ts_queue, (unsigned long)desc) < 0) {
			pool_dma_free(eth->buf_pool, desc);
		} else {
			rc = AVB_WAKE_THREAD;
		}
	}

	return rc;
}

int ptp_tx_ts_lock(struct logical_port *port, struct net_tx_desc *desc)
{
	raw_spinlock_t *lock = &port->eth->lock;
	unsigned long flags;
	int rc = 0;

	raw_spin_lock_irqsave(lock, flags);

	rc = __ptp_tx_ts(port, desc);

	raw_spin_unlock_irqrestore(lock, flags);

	return rc;
}

int ptp_tx_ts(struct logical_port *port, struct net_tx_desc *desc)
{
	int rc = 0;

	rc = __ptp_tx_ts(port, desc);

	return rc;
}

#if defined(CONFIG_GENAVB_HYBRID) || defined(CONFIG_GENAVB_BRIDGE)
int __ptp_rx(struct logical_port *port, struct net_rx_desc *desc)
{
	struct eth_avb *eth = port->eth;
	struct net_socket *sock;
	struct net_rx_stats *stats = &ptype_hdlr[PTYPE_PTP].stats[eth->port];
	int rc;
	struct net_socket *nsock[SOCKET_MAX_RX];
	unsigned long flags;
	unsigned int n = 0;

	raw_spin_lock_irqsave(&ptype_lock, flags);

	desc->port = port->id;

	sock = ptp_rx_hdlr[port->id].sock;
	if (!sock)
		goto slow_unlock;

	rc = net_rx(eth, sock, desc, stats);

	generic_rx_wakeup(sock, nsock, &n);

	raw_spin_unlock_irqrestore(&ptype_lock, flags);

	if (n) {
		wake_up(&nsock[0]->wait);
		clear_bit(SOCKET_ATOMIC_FLAGS_BUSY, &nsock[0]->atomic_flags);
	}

	return rc;

slow_unlock:
	raw_spin_unlock_irqrestore(&ptype_lock, flags);

	return net_rx_slow(eth, desc, stats);
}

int ptp_rx(struct eth_avb *eth, struct net_rx_desc *desc, void *hdr)
{
	struct ptp_hdr *ptp = hdr;
	struct net_rx_stats *stats = &ptype_hdlr[PTYPE_PTP].stats[eth->port];
	int rc = AVB_NET_RX_OK;

	if (ptp->version_ptp != 0x2)
		goto slow;

	if ((ptp->transport_specific != 0x1) && (ptp->transport_specific != 0x2))
		goto slow;

	if (bridge_receive(eth, desc, PTYPE_PTP) < 0)
		rc = net_rx_drop(eth, desc, stats);

	return rc;

slow:
	return net_rx_slow(eth, desc, stats);
}

#else
int ptp_rx(struct eth_avb *eth, struct net_rx_desc *desc, void *hdr)
{
	struct ptp_hdr *ptp = hdr;
	struct net_rx_stats *stats = &ptype_hdlr[PTYPE_PTP].stats[desc->port];
	struct net_socket *sock;
	struct logical_port *port = physical_to_logical_port(eth);
	unsigned long flags;
	int rc;

	if (ptp->version_ptp != 0x2)
		goto slow;

	if ((ptp->transport_specific != 0x1) && (ptp->transport_specific != 0x2))
		goto slow;

	raw_spin_lock_irqsave(&ptype_lock, flags);

	sock = ptp_rx_hdlr[port->id].sock;
	if (!sock)
		goto slow_unlock;

	rc = net_rx(eth, sock, desc, stats);

	raw_spin_unlock_irqrestore(&ptype_lock, flags);

	return rc;

slow_unlock:
	raw_spin_unlock_irqrestore(&ptype_lock, flags);

slow:
	return net_rx_slow(eth, desc, stats);
}
#endif


