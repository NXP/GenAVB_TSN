/*
 * Copyright 2018-2021, 2023, 2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief AVTP management functions
 @details
*/

#ifndef _RTOS_AVTP_H_
#define _RTOS_AVTP_H_

#include "genavb/avtp.h"
#include "genavb/sr_class.h"

#include "net_socket.h"
#include "net_rx.h"
#include "net_tx.h"

#if defined(CONFIG_GENAVB_TSN_AVTP)
void avtp_socket_unbind(struct net_socket *sock);
int avtp_socket_bind(struct net_socket *sock, struct net_address *addr);

void avtp_socket_disconnect(struct logical_port *port, struct net_socket *sock);
int avtp_socket_connect(struct logical_port *port, struct net_socket *sock, struct net_address *addr);

int avtp_rx(struct net_rx_ctx *net, struct logical_port *port, struct net_rx_desc *desc, void *hdr, unsigned int is_vlan);

const struct net_rx_stats *avtp_maap_stats(unsigned int port);
const struct net_rx_stats *avtp_avdecc_stats(unsigned int port);

#elif defined(CONFIG_GENAVB_TSN_AVDECC) || defined(CONFIG_GENAVB_TSN_MAAP)
#error AVDECC/MAAP enabled without AVTP
#else
static inline void avtp_socket_unbind(struct net_socket *sock) { }
static inline int avtp_socket_bind(struct net_socket *sock, struct net_address *addr) { return -1; }
static inline void avtp_socket_disconnect(struct logical_port *port, struct net_socket *sock) { }
static inline int avtp_socket_connect(struct logical_port *port, struct net_socket *sock, struct net_address *addr) { return -1; }
#endif

#endif /* _RTOS_AVTP_H_ */
