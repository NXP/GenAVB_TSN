/*
* Copyright 2022 NXP
*
* SPDX-License-Identifier: BSD-3-Clause
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

#define DEFAULT_BRIDGE_ID 0

struct net_bridge_drv_ops {
	int (*vlan_read)(struct net_bridge *bridge, uint16_t vid, bool *dynamic, struct genavb_vlan_port_map *map);
	int (*vlan_delete)(struct net_bridge *bridge, uint16_t vid);
	int (*vlan_dump)(struct net_bridge *bridge, uint32_t *token, uint16_t *vid, bool *dynamic, struct genavb_vlan_port_map *map);
	int (*fdb_delete)(struct net_bridge *bridge, uint8_t *address, uint16_t vid, bool dynamic);
	int (*fdb_dump)(struct net_bridge *bridge, uint32_t *token, uint8_t *address, uint16_t *vid, bool *dynamic, struct genavb_fdb_port_map *map, genavb_fdb_status_t *status);
	int (*fdb_read)(struct net_bridge *bridge, uint8_t *address, uint16_t vid, bool *dynamic, struct genavb_fdb_port_map *map, genavb_fdb_status_t *status);
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
struct net_bridge *bridge_get(unsigned int bridge_id);
#endif /* _FREERTOS_NET_BRIDGE_H_ */
