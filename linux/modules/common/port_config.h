/*
 * AVB logical port configuration
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

#ifndef _PORT_CONFIG_H_
#define _PORT_CONFIG_H_

#ifdef __KERNEL__
#include <linux/if.h>
#else
#include <net/if.h>
#define IFNAMSIZ IF_NAMESIZE
#endif

enum logical_port_type {
	LOGICAL_PORT_TYPE_PHYSICAL,
	LOGICAL_PORT_TYPE_BRIDGE
};


struct logical_port_config {
	unsigned int type;
	char name[IFNAMSIZ];	/* Linux interface name */
	bool enabled;
	union {
		struct {
			unsigned int id;
			unsigned int port;
		} bridge;

		struct {
			unsigned int id;
		} physical;
	};
};

#ifdef __KERNEL__

#define CFG_LOGICAL_NUM_PORT	2
#define CFG_PHYSICAL_NUM_PORT	2

#define CFG_PORTS	CFG_PHYSICAL_NUM_PORT

struct physical_port_config {
	char name[IFNAMSIZ];
};

#define LOGICAL_PORT_CONFIG	\
{	\
	[0] = {	\
		.type = LOGICAL_PORT_TYPE_PHYSICAL,	\
		.name = "eth0",	\
		.enabled = true,	\
		.physical.id = 0,	\
	},	\
	[1] = {	\
		.type = LOGICAL_PORT_TYPE_PHYSICAL,	\
		.name = "eth1",	\
		.enabled = true,	\
		.physical.id = 1,	\
	},	\
}

#define PHYSICAL_PORT_CONFIG	\
{	\
	[0] = {	\
		.name = "eth0",	\
	},	\
	[1] = {	\
		.name = "eth1",	\
	},	\
}
#endif /* __KERNEL__ */
#endif /* _PORT_CONFIG_H_ */
