/*
 * MRP management functions
 * Copyright 2015 Freescale Semiconductor, Inc.
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef _MRP_H_
#define _MRP_H_

#include "net_rx.h"
#include "netdrv.h"

void mrp_socket_unbind(struct net_socket *sock);
int mrp_socket_bind(struct net_socket *sock, struct net_address *addr);
void mrp_socket_disconnect(struct logical_port *port, struct net_socket *sock);
int mrp_socket_connect(struct logical_port *port, struct net_socket *sock, struct net_address *addr);
int mrp_rx(struct eth_avb *eth, struct net_rx_desc *desc, void *hdr);
int __mrp_rx(struct logical_port *port, struct net_rx_desc *desc);

#endif /* _MRP_H_ */
