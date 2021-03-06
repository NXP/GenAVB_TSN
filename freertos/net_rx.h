/*
* Copyright 2017-2020 NXP
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
 @brief FreeRTOS specific Network service implementation
 @details
*/

#ifndef _FREERTOS_NET_RX_H_
#define _FREERTOS_NET_RX_H_

#include "genavb/config.h"
#include "genavb/net_types.h"
#include "net_port.h"
#include "slist.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "os/sys_types.h"
#include "os/timer.h"

#define NET_RX_PERIOD		125000	/* 125us */
#define NET_RX_PACKETS		20	/* 100Mbit/s * 125us ~= 18.6 small packets,
					   1Gbit/s * 125us ~= 10 big packets */

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

struct net_rx_ctx {
	struct ptype_handler ptype_hdlr[PTYPE_MAX];

	StaticSemaphore_t mutex_buffer;
	SemaphoreHandle_t mutex;

	TaskHandle_t handle;

	/* Periodic timer */
	struct net_rx_timer {
		struct os_timer handle;
		unsigned int period;
		void *data;
	} timer[CFG_PORTS];

	struct slist_head polling_list;
	unsigned int time;

	/* Event Queue */
	StaticQueue_t queue;
	uint8_t queue_buffer[NET_RX_EVENT_QUEUE_LENGTH * sizeof(struct event)];
	QueueHandle_t queue_handle;
};

extern struct net_rx_ctx net_rx_ctx;

struct net_rx_desc *net_rx_alloc(unsigned int size);
int net_rx_slow(struct net_rx_ctx *net, struct net_port *port, struct net_rx_desc *desc, struct net_rx_stats *stats);
int net_rx_drop(struct net_rx_ctx *net, struct net_rx_desc *desc, struct net_rx_stats *stats);
int eth_rx(struct net_rx_ctx *net, struct net_rx_desc *desc, struct net_port *port);
void net_rx_flush(struct net_rx_ctx *net, struct net_port *port);
int other_socket_bind(struct net_socket *sock, struct net_address *addr);
void other_socket_unbind(struct net_socket *sock);

#endif /* _FREERTOS_NET_RX_H_ */
