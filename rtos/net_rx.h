/*
 * Copyright 2017-2024, 2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief RTOS specific Network service implementation
 @details
*/

#ifndef _RTOS_NET_RX_H_
#define _RTOS_NET_RX_H_

#include "genavb/config.h"
#include "genavb/net_types.h"
#include "config.h"
#include "net_port.h"
#include "slist.h"

#include "rtos_abstraction_layer.h"

#include "os/sys_types.h"
#include "os/timer.h"

#define NET_RX_TX_PERIOD		125000	/* 125us */

#define NET_RX_BRIDGE_PACKETS   8

#define AVB_NET_RX_WAKE		1
#define AVB_NET_RX_OK		0
#define AVB_NET_RX_SLOW		-1
#define AVB_NET_RX_DROP		-2

#define NET_RX_EVENT_QUEUE_LENGTH	16

struct generic_rx_hdlr {
	struct net_socket *sock;
};

struct net_rx_stats {
	unsigned int rx;
	unsigned int dropped;
	unsigned int slow;
	unsigned int slow_dropped;
};

struct ptype_handler {
	struct net_rx_stats stats[CFG_PORTS];
};

/*
 * Compact ptype instance IDs: sequential indices into ptype_hdlr[], gated by
 * compile options. Protocol-specific instances are only allocated when the
 * corresponding CONFIG is enabled; when off, callers fall back to
 * PTYPE_INST_OTHER for stats accounting.
 */
enum {
#if defined(CONFIG_GENAVB_TSN_SRP)
	PTYPE_INST_MRP,
#endif
#if defined(CONFIG_GENAVB_TSN_GPTP) || defined(CONFIG_GENAVB_TSN_SOCKET)
	PTYPE_INST_PTP,
#endif
#if defined(CONFIG_GENAVB_TSN_HSR)
	PTYPE_INST_HSR,
#endif
#if defined(CONFIG_GENAVB_TSN_AVTP)
	PTYPE_INST_AVTP,
#endif
#if defined(CONFIG_GENAVB_TSN_SOCKET)
	PTYPE_INST_L2,
#endif
	PTYPE_INST_OTHER,
	PTYPE_INST_MAX
};

struct net_rx_ctx {
	struct ptype_handler ptype_hdlr[PTYPE_INST_MAX];

	rtos_mutex_t mutex;

	rtos_thread_t task;

	/* Periodic timer */
	struct net_rx_timer {
		struct os_timer handle;
		unsigned int period;
	} timer;

	struct slist_head polling_list;
	unsigned int time;

	/* Event Queue */
	rtos_mqueue_t queue;
	uint8_t queue_buffer[NET_RX_EVENT_QUEUE_LENGTH * sizeof(struct event)];
};

extern struct net_rx_ctx net_rx_ctx;

struct net_rx_desc *net_pool_rx_alloc(unsigned int size);
int net_rx_slow(struct net_rx_ctx *net, struct logical_port *port, struct net_rx_desc *desc, struct net_rx_stats *stats);
int net_rx_drop(struct net_rx_ctx *net, struct net_rx_desc *desc, struct net_rx_stats *stats);
int eth_rx(struct net_rx_ctx *net, struct net_rx_desc *desc, struct net_port *port);
void net_rx_flush(struct net_rx_ctx *net, struct net_port *port);
int other_socket_bind(struct net_socket *sock, struct net_address *addr);
void other_socket_unbind(struct net_socket *sock);

#endif /* _RTOS_NET_RX_H_ */
