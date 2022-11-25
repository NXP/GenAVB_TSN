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

#ifndef _FREERTOS_NET_PORT_ENETC_EP_H_
#define _FREERTOS_NET_PORT_ENETC_EP_H_

#include "net_port.h"

int enetc_ep_init(struct net_port *port);
void *enetc_ep_get_handle(struct net_port *port);
void *enetc_ep_get_port_handle(struct net_port *port);
void *enetc_ep_get_pseudo_port_handle(struct net_port *port);

#endif /* _FREERTOS_NET_PORT_ENETC_EP_H_ */
