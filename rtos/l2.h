/*
 * Copyright 2018-2019, 2021, 2023, 2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief Packet socket management functions
 @details
*/

#ifndef _RTOS_PACKET_H_
#define _RTOS_PACKET_H_

#include "net_socket.h"
#include "net_rx.h"

#if defined(CONFIG_GENAVB_TSN_SOCKET)
void l2_socket_unbind(struct net_socket *sock);
int l2_socket_bind(struct net_socket *sock, struct net_address *addr);

void l2_socket_disconnect(struct logical_port *port, struct net_socket *sock);
int l2_socket_connect(struct logical_port *port, struct net_socket *sock, struct net_address *addr);

int l2_rx(struct net_rx_ctx *net, struct logical_port *port, struct net_rx_desc *desc);
#else
static inline void l2_socket_unbind(struct net_socket *sock) { }
static inline int l2_socket_bind(struct net_socket *sock, struct net_address *addr) { return -1; }
static inline void l2_socket_disconnect(struct logical_port *port, struct net_socket *sock) { }
static inline int l2_socket_connect(struct logical_port *port, struct net_socket *sock, struct net_address *addr) { return -1; }

static inline int l2_rx(struct net_rx_ctx *net, struct logical_port *port, struct net_rx_desc *desc)
{
	return net_rx_slow(net, port, desc, &net->ptype_hdlr[PTYPE_INST_OTHER].stats[desc->port]);
}
#endif

#endif /* _RTOS_PACKET_H_ */
