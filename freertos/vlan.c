/*
 * Copyright 2022-2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief FreeRTOS specific VLAN registration service implementation
 @details
*/

#include "os/vlan.h"
#include "common/log.h"
#include "net_bridge.h"
#include "net_logical_port.h"

#if CFG_BRIDGE_NUM

int vlan_update(uint16_t vid, bool dynamic, struct genavb_vlan_port_map *map)
{
	struct net_bridge *bridge = bridge_get(DEFAULT_BRIDGE_ID);
	struct logical_port *port;
	int rc = -1;

	if ((vid == 0) || (vid > 4095))
		goto err;

	if (dynamic) {
		if (map->control == GENAVB_VLAN_ADMIN_CONTROL_FORBIDDEN ||
		    map->control == GENAVB_VLAN_ADMIN_CONTROL_FIXED ||
		    map->control == GENAVB_VLAN_ADMIN_CONTROL_NORMAL)
			goto err;
	} else {
		if (map->control == GENAVB_VLAN_ADMIN_CONTROL_REGISTERED ||
		    map->control == GENAVB_VLAN_ADMIN_CONTROL_NOT_REGISTERED)
			goto err;
	}

	if (!(logical_port_valid(map->port_id) && logical_port_is_bridge(map->port_id) && (logical_port_bridge_id(map->port_id) == DEFAULT_BRIDGE_ID)))
		goto err;

	port = __logical_port_get(map->port_id);

	xSemaphoreTake(bridge->mutex, portMAX_DELAY);

	if (port->phys->drv_ops.vlan_update)
		rc = port->phys->drv_ops.vlan_update(port->phys, vid, dynamic, map);

	xSemaphoreGive(bridge->mutex);
err:
	return rc;
}

int vlan_delete(uint16_t vid, bool dynamic)
{
	struct net_bridge *bridge = bridge_get(DEFAULT_BRIDGE_ID);
	int rc = -1;

	xSemaphoreTake(bridge->mutex, portMAX_DELAY);

	if (bridge->drv_ops.vlan_delete)
		rc = bridge->drv_ops.vlan_delete(bridge, vid, dynamic);

	xSemaphoreGive(bridge->mutex);

	return rc;
}

int vlan_read(uint16_t vid, bool *dynamic, struct genavb_vlan_port_map *map)
{
	struct net_bridge *bridge = bridge_get(DEFAULT_BRIDGE_ID);
	int rc = -1;

	xSemaphoreTake(bridge->mutex, portMAX_DELAY);

	if (bridge->drv_ops.vlan_read)
		rc = bridge->drv_ops.vlan_read(bridge, vid, dynamic, map);

	xSemaphoreGive(bridge->mutex);

	return rc;
}

int vlan_dump(uint32_t *token, uint16_t *vid, bool *dynamic, struct genavb_vlan_port_map *map)
{
	struct net_bridge *bridge = bridge_get(DEFAULT_BRIDGE_ID);
	int rc = -1;

	xSemaphoreTake(bridge->mutex, portMAX_DELAY);

	if (bridge->drv_ops.vlan_dump)
		rc = bridge->drv_ops.vlan_dump(bridge, token, vid, dynamic, map);

	xSemaphoreGive(bridge->mutex);

	return rc;
}
#else
int vlan_update(uint16_t vid, bool dynamic, struct genavb_vlan_port_map *map) { return -1; }
int vlan_delete(uint16_t vid, bool dynamic) { return -1; }
int vlan_read(uint16_t vid, bool *dynamic, struct genavb_vlan_port_map *map) { return -1; }
int vlan_dump(uint32_t *token, uint16_t *vid, bool *dynamic, struct genavb_vlan_port_map *map) { return -1; }
#endif
