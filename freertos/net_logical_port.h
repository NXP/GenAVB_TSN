/*
* Copyright 2018 NXP
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

/**
 @file
 @brief AVB logical port functions
 @details

 Copyright 2018 NXP
 All Rights Reserved.
*/

#ifndef _NET_LOGICAL_PORT_H_
#define _NET_LOGICAL_PORT_H_

#include "config.h"

struct logical_port {
	bool is_bridge;

	union {
		struct {
			unsigned int port;
		} bridge;
	};

	struct net_port *phys; /* pointer to physical interface */

	struct net_socket *ptp_sock;

	unsigned int id;
};

static inline bool logical_port_valid(unsigned int port)
{
	return (port < CFG_LOGICAL_NUM_PORT);
}

struct logical_port *__logical_port_get(unsigned int port);
struct logical_port *logical_port_get(unsigned int port);
bool logical_port_is_bridge(unsigned int port);
bool logical_port_is_endpoint(unsigned int port);
unsigned int logical_port_endpoint_id(unsigned int port);
unsigned int logical_port_bridge_id(unsigned int port);
unsigned int logical_port_to_bridge_port(unsigned int port);
void logical_port_init();

#endif /* _NET_LOGICAL_PORT_H_ */
