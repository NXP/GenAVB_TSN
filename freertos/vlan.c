/*
 * Copyright 2022 NXP
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

int vlan_update(uint16_t vid, struct genavb_vlan_port_map *map)
{
	struct logical_port *port;
	int rc = -1;

	if ((vid == 0) || (vid > 4095))
		goto err;

	port = logical_port_get(map->port_id);
	if (!port)
		goto err;

	if (port->phys->drv_ops.vlan_update)
		rc = port->phys->drv_ops.vlan_update(port->phys, vid, map);

err:
	return rc;
}

int vlan_delete(uint16_t vid)
{
	struct net_bridge *bridge = bridge_get(DEFAULT_BRIDGE_ID);
	int rc = -1;

	if (bridge->drv_ops.vlan_delete)
		rc = bridge->drv_ops.vlan_delete(bridge, vid);

	return rc;
}

int vlan_read(uint16_t vid, bool *dynamic, struct genavb_vlan_port_map *map)
{
	struct net_bridge *bridge = bridge_get(DEFAULT_BRIDGE_ID);
	int rc = -1;

	if (bridge->drv_ops.vlan_read)
		rc = bridge->drv_ops.vlan_read(bridge, vid, dynamic, map);

	return rc;
}

int vlan_dump(uint32_t *token, uint16_t *vid, bool *dynamic, struct genavb_vlan_port_map *map)
{
	struct net_bridge *bridge = bridge_get(DEFAULT_BRIDGE_ID);
	int rc = -1;

	if (bridge->drv_ops.vlan_dump)
		rc = bridge->drv_ops.vlan_dump(bridge, token, vid, dynamic, map);

	return rc;
}
#else
int vlan_update(uint16_t vid, struct genavb_vlan_port_map *map) { return -1; }
int vlan_delete(uint16_t vid) { return -1; }
int vlan_read(uint16_t vid, bool *dynamic, struct genavb_vlan_port_map *map) { return -1; }
int vlan_dump(uint32_t *token, uint16_t *vid, bool *dynamic, struct genavb_vlan_port_map *map) { return -1; }
#endif
