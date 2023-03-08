/*
* Copyright 2022-2023 NXP
*
* SPDX-License-Identifier: BSD-3-Clause
*/

/**
 @file
 @brief FreeRTOS specific Network service implementation
 @details
*/

#include "common/log.h"

#include "os/net.h"

#include "net_bridge.h"
#include "net_port.h"
#include "net_rx.h"
#include "net_logical_port.h"

#if CFG_BRIDGE_NUM

#include "net_port_netc_sw.h"
#include "hw_clock.h"

struct net_bridge bridges[CFG_BRIDGE_NUM] = {
	[0] = {
		.index = 0,
		.drv_index = BOARD_NET_BRIDGE0_DRV_INDEX,
		.drv_type = BOARD_NET_BRIDGE0_DRV_TYPE,
	},
};

struct net_bridge *bridge_get(unsigned int bridge_id)
{
	if (bridge_id > (CFG_BRIDGE_NUM - 1))
		return NULL;
	else
		return &bridges[bridge_id];
}

static inline void bridge_tx_lock(struct net_bridge *bridge)
{
	xSemaphoreTake(bridge->mutex, portMAX_DELAY);
}

static inline void bridge_tx_unlock(struct net_bridge *bridge)
{
	xSemaphoreGive(bridge->mutex);
}

void bridge_tx_cleanup(struct net_bridge *bridge)
{
	bridge_tx_lock(bridge);

	bridge->drv_ops.tx_cleanup(bridge);

	bridge_tx_unlock(bridge);
}

int bridge_port_tx(struct net_port *port, uint8_t priority, struct net_tx_desc *desc)
{
	struct net_bridge *bridge = &bridges[logical_port_bridge_id(port->logical_port->id)];
	int rc;

	bridge_tx_lock(bridge);

	if (port->up) {

		port->stats[0].tx++;
		rc = bridge->drv_ops.send_frame(bridge, port->logical_port->bridge.port, (uint8_t *)desc + desc->l2_offset,
					      desc->len, desc, priority, desc->flags & NET_TX_FLAGS_HW_TS);
		if (rc < 0) {
			port->stats[0].tx_err++;
			rc = -1;
		}
	} else {
		rc = -1;
	}

	bridge_tx_unlock(bridge);

	/* Free descriptor now only if buffer is copied and successfully transmitted */
	if (!rc)
		net_tx_free(desc);

	return rc;
}

unsigned int bridge_rx(struct net_rx_ctx *net, struct net_bridge *bridge, unsigned int n)
{
	uint32_t length;
	uint64_t ts64;
	uint8_t port_index;
	struct net_rx_desc *desc;
	struct net_port *port;
	unsigned int read = 0;
	int rc;

	while (read < n) {
		rc = bridge->drv_ops.get_rx_frame_size(bridge, &length);
		switch (rc) {
		case BR_RX_FRAME_SUCCESS:
			desc = net_rx_alloc(length);
			if (!desc) {
				bridge->stats.rx_alloc_err++;
				break;
			}

			if (!bridge->drv_ops.read_frame(bridge, (uint8_t *)desc + desc->l2_offset, length, &port_index, &ts64)) {
				port = &ports[port_index];

				desc->len = length;
				desc->ts64 = hw_clock_cycles_to_time(port->hw_clock, ts64) - port->rx_tstamp_latency;
				desc->ts = (uint32_t)desc->ts64;

				port->stats[0].rx++;
				eth_rx(net, desc, port);
			} else {
				bridge->stats.rx_err++;
				net_rx_free(desc);
			}
			break;

		case BR_RX_FRAME_EGRESS_TS:
			bridge_tx_lock(bridge);

			if (bridge->drv_ops.read_egress_ts_frame(bridge) < 0)
				bridge->stats.tx_ts_err++;

			bridge_tx_unlock(bridge);
			break;

		case BR_RX_FRAME_ERROR:
			bridge->drv_ops.read_frame(bridge, NULL, 0, NULL, NULL);
			break;

		case BR_RX_FRAME_EMPTY:
		default:
			break;
		}

		read++;

		xSemaphoreGive(net->mutex);
		/* Explicit preemption point, to allow socket synchronous receive */
		xSemaphoreTake(net->mutex, portMAX_DELAY);
	}

	return read;
}

static int __bridge_init(struct net_bridge *bridge)
{
	os_log(LOG_INIT, "bridge(%u): %p\n", bridge->index, bridge);

	switch (bridge->drv_type) {
	case NETC_SW_t:
		bridge->drv_ops.init = netc_sw_init;
		break;

	default:
		os_log(LOG_ERR, "bridge(%d) invalid driver type: %u\n", bridge->index, bridge->drv_type);
		goto err_type;
		break;
	}

	memset(&bridge->stats, 0, sizeof(bridge->stats));

	bridge->mutex = xSemaphoreCreateMutexStatic(&bridge->mutex_buffer);
	if (!bridge->mutex) {
		os_log(LOG_ERR, "xSemaphoreCreateMutexStatic() bridge(%u) failed\n", bridge->index);
		goto err_semaphore_create;
	}

	if (bridge->drv_ops.init(bridge) < 0)
		goto err_drv_init;

	return 0;

err_drv_init:
err_semaphore_create:
err_type:
	return -1;
}

__init int bridge_init(void)
{
	int i;

	for (i = 0; i < CFG_BRIDGE_NUM; i++) {
		if (__bridge_init(&bridges[i]) < 0)
			goto err;
	}

	return 0;

err:
	while (--i >= 0) {
		if (bridges[i].drv)
			bridges[i].drv_ops.exit(&bridges[i]);
	}

	return -1;
}

__exit void bridge_exit(void)
{
	int i;

	for (i = 0; i < CFG_BRIDGE_NUM; i++) {
		if (bridges[i].drv)
			bridges[i].drv_ops.exit(&bridges[i]);
	}
}

#else
struct net_bridge *bridge_get(unsigned int bridge_id) { return 0; }
void bridge_tx_cleanup(struct net_bridge *bridge) {};
int bridge_port_tx(struct net_port *port, uint8_t priority, struct net_tx_desc *desc) { return 0; }
unsigned int bridge_rx(struct net_rx_ctx *net, struct net_bridge *bridge, unsigned int n) { return 0; }
__init int bridge_init(void) { return 0; }
__exit void bridge_exit(void) {}
#endif /* CFG_BRIDGE_NUM */
