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

#ifndef _PTP_H_
#define _PTP_H_

#include "genavb/ptp.h"

#include "net_rx.h"
#include "netdrv.h"

void ptp_socket_unbind(struct net_socket *sock);
int ptp_socket_bind(struct net_socket *sock, struct net_address *addr);
void ptp_socket_disconnect(struct logical_port *port, struct net_socket *sock);
int ptp_socket_connect(struct logical_port *port, struct net_socket *sock, struct net_address *addr);
void ptp_tx_ts_wakeup(struct logical_port *port);
int ptp_tx_ts(struct logical_port *port, struct net_tx_desc *desc);
int ptp_rx(struct eth_avb *eth, struct net_rx_desc *desc, void *hdr);
int __ptp_rx(struct logical_port *port, struct net_rx_desc *desc);

#endif /* _PTP_H_ */
