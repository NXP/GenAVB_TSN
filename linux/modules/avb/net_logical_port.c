/*
 * AVB logical port functions
 * Copyright 2018 NXP
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
#include <linux/kernel.h>
#include <linux/netdevice.h>

#include "avbdrv.h"
#include "net_logical_port.h"
#include "net_port.h"

struct logical_port_config port_config[CFG_LOGICAL_NUM_PORT] = LOGICAL_PORT_CONFIG;

static struct physical_port_config phys_port_config[CFG_PHYSICAL_NUM_PORT] = PHYSICAL_PORT_CONFIG;

struct logical_port *__logical_port_get(struct avb_drv *avb, unsigned int port)
{
	return &avb->logical_port[port];
}

struct logical_port *logical_port_get(struct avb_drv *avb, unsigned int port)
{
	if (!logical_port_valid(port))
		return NULL;

	return __logical_port_get(avb, port);
}

int logical_port_init(struct avb_drv *avb)
{
	struct logical_port *port;
	struct net_device *ndev;
	int i;

	for (i = 0; i < CFG_LOGICAL_NUM_PORT; i++) {
		port = logical_port_get(avb, i);
		if (!port)
			continue;

		port->id = i;

		ndev = dev_get_by_name(&init_net, port_config[i].name);
		if (ndev) {
			port->ifindex = ndev->ifindex;
			dev_put(ndev);
		} else {
			port->ifindex = -1;
		}

		if (port_config[i].type == LOGICAL_PORT_TYPE_PHYSICAL) {
			port->eth = &avb->eth[port_config[i].physical.id];

			port->eth->logical_port = port;
		}
	}

	return 0;
}

char *physical_port_name(unsigned int port)
{
	return phys_port_config[port].name;
}

