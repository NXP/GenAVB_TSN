/*
 * Copyright 2021-2023, 2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief RTOS specific FDB service implementation
 @details
*/

#include "config.h"

#include "genavb/error.h"

#include "os/fdb.h"
#include "common/log.h"
#include "net_bridge.h"
#include "net_logical_port.h"


#if CFG_BRIDGE_NUM
int fdb_update(unsigned int port_id, uint8_t *address, uint16_t vid, bool dynamic, genavb_fdb_port_control_t control)
{
	struct net_bridge *bridge = bridge_get(DEFAULT_BRIDGE_ID);
	struct logical_port *port;
	int rc = -GENAVB_ERR_FDB_NOT_SUPPORTED;

	if ((vid == 0) || (vid > 4095)) {
		rc = -GENAVB_ERR_FDB_INVALID_VID;
		goto out;
	}

	if (!(logical_port_valid(port_id) && logical_port_is_bridge(port_id) && (logical_port_bridge_id(port_id) == DEFAULT_BRIDGE_ID))) {
		rc = -GENAVB_ERR_FDB_INVALID_PORT;
		goto out;
	}

	port = __logical_port_get(port_id);

	rtos_mutex_lock(&bridge->mutex, RTOS_WAIT_FOREVER);

	if (port->phys->drv_ops.fdb_update)
		rc = port->phys->drv_ops.fdb_update(port->phys, address, vid, dynamic, control);

	rtos_mutex_unlock(&bridge->mutex);

out:
	return rc;
}

int fdb_delete(uint8_t *address, uint16_t vid, bool dynamic)
{
	struct net_bridge *bridge = bridge_get(DEFAULT_BRIDGE_ID);
	int rc = -GENAVB_ERR_FDB_NOT_SUPPORTED;

	rtos_mutex_lock(&bridge->mutex, RTOS_WAIT_FOREVER);

	if (bridge->drv_ops.fdb_delete)
		rc = bridge->drv_ops.fdb_delete(bridge, address, vid, dynamic);

	rtos_mutex_unlock(&bridge->mutex);

	return rc;
}

int fdb_read(uint8_t *address, uint16_t vid, bool *dynamic, struct genavb_fdb_port_map *map, genavb_fdb_status_t *status)
{
	struct net_bridge *bridge = bridge_get(DEFAULT_BRIDGE_ID);
	int rc = -GENAVB_ERR_FDB_NOT_SUPPORTED;

	rtos_mutex_lock(&bridge->mutex, RTOS_WAIT_FOREVER);

	if (bridge->drv_ops.fdb_read)
		rc = bridge->drv_ops.fdb_read(bridge, address, vid, dynamic, map, status);

	rtos_mutex_unlock(&bridge->mutex);

	return rc;
}

int fdb_dump(uint32_t *token, uint8_t *address, uint16_t *vid, bool *dynamic, struct genavb_fdb_port_map *map, genavb_fdb_status_t *status)
{
	struct net_bridge *bridge = bridge_get(DEFAULT_BRIDGE_ID);
	int rc = -GENAVB_ERR_FDB_NOT_SUPPORTED;

	rtos_mutex_lock(&bridge->mutex, RTOS_WAIT_FOREVER);

	if (bridge->drv_ops.fdb_dump)
		rc = bridge->drv_ops.fdb_dump(bridge, token, address, vid, dynamic, map, status);

	rtos_mutex_unlock(&bridge->mutex);

	return rc;
}
#else
int fdb_update(unsigned int port_id, uint8_t *address, uint16_t vid, bool dynamic, genavb_fdb_port_control_t control) { return -1; }
int fdb_delete(uint8_t *address, uint16_t vid, bool dynamic) { return -1; }
int fdb_read(uint8_t *address, uint16_t vid, bool *dynamic, struct genavb_fdb_port_map *map, genavb_fdb_status_t *status) { return -1; }
int fdb_dump(uint32_t *token, uint8_t *address, uint16_t *vid, bool *dynamic, struct genavb_fdb_port_map *map, genavb_fdb_status_t *status) { return -1; }
#endif
