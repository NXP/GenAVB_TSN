/*
* Copyright 2022 NXP
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
 @brief FreeRTOS specific Network service implementation
 @details
*/

#ifndef _FREERTOS_NET_PORT_NETC_SW_H_
#define _FREERTOS_NET_PORT_NETC_SW_H_

#include "net_port.h"
#include "net_bridge.h"

int netc_sw_init(struct net_bridge *sw);
int netc_sw_port_init(struct net_port *port);
void *netc_sw_get_handle(struct net_port *port);
void *netc_sw_get_port_handle(struct net_port *port);

#endif /* _FREERTOS_NET_PORT_NETC_SW_H_ */

