/*
 * Copyright 2022 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "genavb/vlan.h"


int genavb_vlan_update(uint16_t vid, struct genavb_vlan_port_map *map)
{
	int rc;

	rc = vlan_update(vid, false, map);

	return rc;
}

int genavb_vlan_delete(uint16_t vid)
{
	int rc;

	rc = vlan_delete(vid, false);

	return rc;
}

int genavb_vlan_read(uint16_t vid, bool *dynamic, struct genavb_vlan_port_map *map)
{
	int rc;

	rc = vlan_read(vid, dynamic, map);

	return rc;
}

int genavb_vlan_dump(uint32_t *token, uint16_t *vid, bool *dynamic, struct genavb_vlan_port_map *map)
{
	int rc;

	rc = vlan_dump(token, vid, dynamic, map);

	return rc;
}
