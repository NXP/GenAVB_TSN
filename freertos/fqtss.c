/*
* Copyright 2020, 2023 NXP
*
* SPDX-License-Identifier: BSD-3-Clause
*/

/**
 @file
 @brief FreeRTOS specific FQTSS service implementation
 @details
*/

#include "FreeRTOS.h"

#include "common/log.h"
#include "common/net.h"

#include "net_socket.h"
#include "net_tx.h"

static struct net_socket *socket = NULL;

int fqtss_set_oper_idle_slope(unsigned int port_id, uint8_t traffic_class, unsigned int idle_slope)
{
	struct logical_port *port;
	int rc = -1;

	port = logical_port_get(port_id);
	if (!port)
		goto err;

	if (port->phys->drv_ops.set_tx_idle_slope)
		rc = port->phys->drv_ops.set_tx_idle_slope(port->phys, idle_slope, traffic_class);

err:
	return rc;
}

static int net_sr_config(unsigned int port_id, void *stream_id, uint16_t vlan_id, uint8_t priority, unsigned int idle_slope)
{
	struct net_sr_config sr_config;
	struct event e;
	int rc = 0;

	sr_config.port = port_id;
	sr_config.vlan_id = vlan_id;
	sr_config.priority = priority;
	copy_64(&sr_config.stream_id, stream_id);
	sr_config.idle_slope = idle_slope;

	socket->handle_data = &sr_config;

	e.type = EVENT_SR_CONFIG;
	e.data = socket;

	if (xQueueSend(net_tx_ctx.queue_handle, &e, 0) != pdPASS) {
		os_log(LOG_ERR, "xQueueSend failed\n");
		rc = -1;
		goto err;
	}

	rc = xEventGroupWaitBits(socket->event_group_handle, SOCKET_SUCCESS | SOCKET_ERROR, pdTRUE, pdFALSE, pdMS_TO_TICKS(1));

	if (!(rc & SOCKET_SUCCESS)) {
		rc = -1;
		goto err;
	}

	rc = 0;

err:
	return rc;
}

int fqtss_stream_add(unsigned int port_id, void *stream_id, uint16_t vlan_id, uint8_t priority, unsigned int idle_slope)
{
	struct logical_port *port = logical_port_get(port_id);

	if (!port || port->is_bridge)
		return -1;

	return net_sr_config(port_id, stream_id, vlan_id, priority, idle_slope);
}

int fqtss_stream_remove(unsigned int port_id, void *stream_id, uint16_t vlan_id, uint8_t priority, unsigned int idle_slope)
{
	struct logical_port *port = logical_port_get(port_id);

	if (!port || port->is_bridge)
		return -1;

	return net_sr_config(port_id, stream_id, vlan_id, priority, 0);
}

__init int fqtss_init(void)
{
	socket = socket_open(NULL, NULL, 0);
	if (!socket) {
		os_log(LOG_ERR, "socket_open() failed\n");
		goto err_open;
	}

	return 0;

err_open:
	return -1;
}

__exit void fqtss_exit(void)
{
	socket_close(socket);
	socket = NULL;
}
