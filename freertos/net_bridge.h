/*
* Copyright 2022 NXP
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

#ifndef _FREERTOS_NET_BRIDGE_H_
#define _FREERTOS_NET_BRIDGE_H_

#include "os/net.h"
#include "genavb/net_types.h"

#include "net_port.h"

struct net_bridge;
struct net_rx_ctx;

#define BR_RX_FRAME_ERROR	-1
#define BR_RX_FRAME_EMPTY	0
#define BR_RX_FRAME_SUCCESS	1
#define BR_RX_FRAME_EGRESS_TS	2

struct net_bridge_drv_ops {
	int (*get_rx_frame_size)(struct net_bridge *bridge, uint32_t *length);
	int (*read_frame)(struct net_bridge *bridge, uint8_t *data, uint32_t length, uint8_t *port_index, uint64_t *ts);
	int (*read_egress_ts_frame)(struct net_bridge *bridge);
	int (*send_frame)(struct net_bridge *bridge, unsigned int port_index, uint8_t *data, uint32_t length, struct net_tx_desc *desc, uint8_t priority, bool need_ts);
	void (*tx_cleanup)(struct net_bridge *bridge);
	int (*init)(struct net_bridge *bridge);
	void (*exit)(struct net_bridge *bridge);
};

struct net_bridge_stats {
	unsigned int rx_err;
	unsigned int rx_alloc_err;
	unsigned int tx_ts_err;
};

struct net_bridge {
	unsigned int index;
	net_driver_type_t drv_type;
	unsigned int drv_index;
	void *drv;
	struct net_bridge_drv_ops drv_ops;
	struct net_bridge_stats stats;

	StaticSemaphore_t mutex_buffer;
	SemaphoreHandle_t mutex;
};

extern struct net_bridge bridges[CFG_BRIDGE_NUM];

int bridge_init(void);
void bridge_exit(void);
unsigned int bridge_rx(struct net_rx_ctx *net, struct net_bridge *bridge, unsigned int n);
int bridge_port_tx(struct net_port *port, uint8_t priority, struct net_tx_desc *desc);
void bridge_tx_cleanup(struct net_bridge *bridge);
#endif /* _FREERTOS_NET_BRIDGE_H_ */
