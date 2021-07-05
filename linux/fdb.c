/*
* Copyright 2020-2021 NXP
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
 @brief Linux specific FDB service implementation
 @details
*/
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>

#include "os/fdb.h"

#include "net_logical_port.h"

#if defined (CONFIG_NET_STD)
#include "fdb_std.h"
#else
#include "fdb_sja.h"
#endif

#include "net_logical_port.h"

int fdb_dynamic_vlan_create(u16 vid, struct fdb_port_map *map, unsigned int n)
{
	return 0;
}

int fdb_dynamic_vlan_delete(u16 vid)
{
	return 0;
}

int fdb_dynamic_vlan_read(u16 vid, struct fdb_port_map *map, unsigned int *n)
{
	return -1;
}

int fdb_dynamic_reservation_create(u8 *mac, u16 vid, struct fdb_port_map *map, unsigned int n)
{
	int i, rc = 0;
	bool add;

	for (i = 0 ; i < n ; i++) {
		switch (map[i].control) {
		case FDB_PORT_CONTROL_FORWARDING:
			add = true;
			break;
		case FDB_PORT_CONTROL_FILTERING:
			add = false;
			break;
		default:
			rc = -1;
			continue;
		}

		if (bridge_rtnetlink(mac, vid, map[i].port_id, add) < 0)
			rc = -1;
	}

	return rc;
}

int fdb_dynamic_reservation_delete(u8 *mac, u16 vid)
{
	int i, rc = 0;

	for (i = 0; i < logical_port_max(); i++) {
		if (!logical_port_valid(i))
			continue;

		if (!logical_port_is_bridge(i))
			continue;

		if (bridge_rtnetlink(mac, vid, i, false) < 0)
			rc = -1;
	}

	return rc;
}

int fdb_dynamic_reservation_read(u8 *mac, u16 vid, struct fdb_port_map *map, unsigned int *n)
{
	return -1;
}
