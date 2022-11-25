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

#ifndef _NET_RX_H_
#define _NET_RX_H_

#include "net_port.h"

#include "genavb/net_types.h"

struct net_rx_stats {
	unsigned int rx;
	unsigned int dropped;
	unsigned int slow;
	unsigned int slow_dropped;
};

struct generic_rx_hdlr {
	struct net_socket *sock;
};

struct ptype_handler {
	struct net_rx_stats stats[CFG_PORTS];
};


extern struct ptype_handler ptype_hdlr[PTYPE_MAX];
extern raw_spinlock_t ptype_lock;

int net_rx_slow(struct eth_avb *eth, struct net_rx_desc *desc, struct net_rx_stats *stats);
int net_rx_drop(struct eth_avb *eth, struct net_rx_desc *desc, struct net_rx_stats *stats);

int net_rx_thread(struct net_drv *drv);
int net_tx_thread(struct eth_avb *eth);
int net_tx_available_thread(struct eth_avb *eth);

int eth_rx(struct eth_avb *eth, struct net_rx_desc *desc);

#endif /* _NET_RX_H_ */
