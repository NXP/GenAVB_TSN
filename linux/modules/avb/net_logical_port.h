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

#ifndef _NET_LOGICAL_PORT_H_
#define _NET_LOGICAL_PORT_H_

#include "port_config.h"

#ifdef __KERNEL__

struct logical_port {
	unsigned int is_bridge;
	int ifindex;

	union {
		struct {
			unsigned int port;
		} bridge;
	};

	struct eth_avb *eth; /* pointer to physical interface */

	struct net_socket *net_sock;

	unsigned int id;
};

struct avb_drv;

struct logical_port *__logical_port_get(struct avb_drv *avb, unsigned int port);
struct logical_port *logical_port_get(struct avb_drv *avb, unsigned int port);
int logical_port_init(struct avb_drv *avb);

char *physical_port_name(unsigned int port);

extern struct logical_port_config port_config[];

#if defined(CONFIG_HYBRID) || defined(CONFIG_BRIDGE)
struct logical_port *bridge_to_logical_port(unsigned int bridge_id, unsigned int bridge_port);
int logical_port_to_bridge(struct logical_port *port, unsigned int *bridge_id, unsigned int *bridge_port);
int physical_to_bridge(struct eth_avb *eth);
#endif

static inline bool logical_port_valid(unsigned int port)
{
	return ((port < CFG_LOGICAL_NUM_PORT) && port_config[port].enabled);
}

#endif /* __KERNEL__ */
#endif /* _NET_LOGICAL_PORT_H_ */
