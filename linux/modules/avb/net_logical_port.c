/*
 * AVB logical port functions
 * Copyright 2018, 2023 NXP
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
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
