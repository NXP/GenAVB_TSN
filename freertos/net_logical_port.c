/*
* Copyright 2018, 2020 NXP
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
 @brief AVB logical port functions
 @details
*/

#include "net_logical_port.h"
#include "net_port.h"

static struct logical_port_config port_config[CFG_LOGICAL_NUM_PORT] = LOGICAL_PORT_CONFIG;

static struct logical_port logical_port[CFG_LOGICAL_NUM_PORT];

struct logical_port *__logical_port_get(unsigned int port)
{
	return &logical_port[port];
}

struct logical_port *logical_port_get(unsigned int port)
{
	if (!logical_port_valid(port))
		return NULL;

	return __logical_port_get(port);
}

bool logical_port_is_bridge(unsigned int port)
{
	return (port_config[port].type == LOGICAL_PORT_TYPE_BRIDGE);
}

bool logical_port_is_endpoint(unsigned int port)
{
	return (port_config[port].type == LOGICAL_PORT_TYPE_PHYSICAL);
}

unsigned int logical_port_endpoint_id(unsigned int port)
{
	return port_config[port].physical.id;
}

unsigned int logical_port_bridge_id(unsigned int port)
{
	return port_config[port].bridge.id;
}

__init void logical_port_init(void)
{
	struct logical_port *port;
	int i;

	for (i = 0; i < CFG_LOGICAL_NUM_PORT; i++) {
		port = __logical_port_get(i);

		port->id = i;

		if (port_config[i].type == LOGICAL_PORT_TYPE_PHYSICAL) {
			port->is_bridge = 0;

			port->phys = &ports[port_config[i].physical.id];

			port->phys->logical_port = port;
		}
	}
}
