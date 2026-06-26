/*
 * Copyright 2018-2019, 2021, 2023, 2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief MRP management functions
 @details
*/

#ifndef _RTOS_MRP_H_
#define _RTOS_MRP_H_

#include "net_socket.h"
#include "net_rx.h"

#if defined(CONFIG_GENAVB_TSN_SRP)
void mrp_socket_unbind(struct net_socket *sock);
int mrp_socket_bind(struct net_socket *sock, struct net_address *addr);

void mrp_socket_disconnect(struct logical_port *port, struct net_socket *sock);
int mrp_socket_connect(struct logical_port *port, struct net_socket *sock, struct net_address *addr);

int mrp_rx(struct net_rx_ctx *net, struct logical_port *port, struct net_rx_desc *desc, void *hdr);
#else
static inline void mrp_socket_unbind(struct net_socket *sock) { }
static inline int mrp_socket_bind(struct net_socket *sock, struct net_address *addr) { return -1; }
static inline void mrp_socket_disconnect(struct logical_port *port, struct net_socket *sock) { }
static inline int mrp_socket_connect(struct logical_port *port, struct net_socket *sock, struct net_address *addr) { return -1; }
#endif

#endif /* _RTOS_MRP_H_ */
