/*
* Copyright 2020 NXP
*
* NXP Confidential. This software is owned or controlled by NXP and may only
* be used strictly in accordance with the applicable license terms.  By expressly
* accepting such terms or by downloading, installing, activating and/or otherwise
* using the software, you are agreeing that you have read, and that you agree to
* comply with and are bound by, such license terms.  If you do not agree to be
* bound by the applicable license terms, then you may not retain, install, activate
* or otherwise use the software.
*/

#include <stdbool.h>

#include "genavb/helpers.h"

#include "common/log.h"

#include "modules/port_config.h"

#include "net_logical_port.h"

static struct logical_port_config port_config[CFG_MAX_LOGICAL_PORTS] = {0, };
static struct {
	char name[IFNAMSIZ];
} bridge_config[CFG_MAX_BRIDGES];

bool logical_port_valid(unsigned int port_id)
{
	return ((port_id < CFG_MAX_LOGICAL_PORTS) && port_config[port_id].enabled);
}

const char *logical_port_name(unsigned int port_id)
{
	return port_config[port_id].name;
}

const char *logical_port_bridge_name(unsigned int port_id)
{
	return bridge_config[port_config[port_id].bridge.id].name;
}

bool logical_port_is_bridge(unsigned int port_id)
{
	return (port_config[port_id].type == LOGICAL_PORT_TYPE_BRIDGE);
}

bool logical_port_is_endpoint(unsigned int port_id)
{
	return (port_config[port_id].type == LOGICAL_PORT_TYPE_PHYSICAL);
}

unsigned int logical_port_endpoint_id(unsigned int port_id)
{
	return port_config[port_id].physical.id;
}

unsigned int logical_port_bridge_id(unsigned int port_id)
{
	return port_config[port_id].bridge.id;
}

unsigned int logical_port_max(void)
{
	return CFG_MAX_LOGICAL_PORTS;
}

unsigned int logical_port_to_bridge_port(unsigned int port_id)
{
	return port_config[port_id].bridge.port;
}

static void logical_port_get_config(struct logical_port_config *port, const char *name)
{
	if (!strlen(name) || !strcmp(name, "off")) {
		port->enabled = false;
	} else {
		h_strncpy(port->name, name, IF_NAMESIZE);
		port->enabled = true;
	}
}

void logical_port_init(struct os_logical_port_config *config)
{
	int i, j;

	for (i = 0; i < CFG_MAX_ENDPOINTS; i++) {
		port_config[i].type = LOGICAL_PORT_TYPE_PHYSICAL;
		port_config[i].physical.id = i;

		logical_port_get_config(&port_config[i], config->endpoint[i]);

		if (port_config[i].enabled)
			os_log(LOG_INIT, "logical_port(%d) enabled, endpoint(%u, %s)\n", i, logical_port_endpoint_id(i), logical_port_name(i));
		else
			os_log(LOG_INIT, "logical_port(%d) disabled\n", i);
	}

	for (i = 0; i < CFG_MAX_BRIDGES; i++) {
		/* Get the bridge interface name */
		if (strlen(config->bridge[i]) && strcmp(config->bridge[i], "off"))
			h_strncpy(bridge_config[i].name, config->bridge[i], IF_NAMESIZE);

		for (j = 0; j < CFG_MAX_NUM_PORT; j++) {
			unsigned int port_id = CFG_MAX_ENDPOINTS + i * CFG_MAX_NUM_PORT + j;

			port_config[port_id].type = LOGICAL_PORT_TYPE_BRIDGE;
			port_config[port_id].bridge.id = i;
			port_config[port_id].bridge.port = j;

			logical_port_get_config(&port_config[port_id], config->bridge_ports[i][j]);

			if (port_config[port_id].enabled)
				os_log(LOG_INIT, "logical_port(%d) enabled, bridge(%s, %u, %u, %s)\n", port_id, bridge_config[i].name, logical_port_bridge_id(port_id),
				       logical_port_to_bridge_port(port_id), logical_port_name(port_id));
			else
				os_log(LOG_INIT, "logical_port(%d) disabled\n", port_id);
		}
	}
}
