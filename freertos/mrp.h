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
 @brief MRP management functions
 @details
*/

#ifndef _FREERTOS_MRP_H_
#define _FREERTOS_MRP_H_

#include "net_socket.h"
#include "net_rx.h"

void mrp_socket_unbind(struct net_socket *sock);
int mrp_socket_bind(struct net_socket *sock, struct net_address *addr);

void mrp_socket_disconnect(struct logical_port *port, struct net_socket *sock);
int mrp_socket_connect(struct logical_port *port, struct net_socket *sock, struct net_address *addr);

int mrp_rx(struct net_rx_ctx *net, struct net_port *port, struct net_rx_desc *desc, void *hdr);

#endif /* _FREERTOS_MRP_H_ */
