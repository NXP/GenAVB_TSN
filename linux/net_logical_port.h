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

#ifndef _LINUX_NET_LOGICAL_PORT_H_
#define _LINUX_NET_LOGICAL_PORT_H_

#include "os_config.h"

bool logical_port_valid(unsigned int port_id);
const char *logical_port_name(unsigned int port_id);
const char *logical_port_bridge_name(unsigned int port_id);
bool logical_port_is_endpoint(unsigned int port_id);
bool logical_port_is_bridge(unsigned int port_id);
unsigned int logical_port_endpoint_id(unsigned int port_id);
unsigned int logical_port_bridge_id(unsigned int port_id);
unsigned int logical_port_max(void);
unsigned int logical_port_to_bridge_port(unsigned int port_id);
void logical_port_init(struct os_logical_port_config *config);

#endif /* _LINUX_NET_LOGICAL_PORT_H_ */
