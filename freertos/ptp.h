/*
* Copyright 2017, 2020 NXP
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
 @brief PTP management functions
 @details
*/

#ifndef _FREERTOS_PTP_H_
#define _FREERTOS_PTP_H_

#include "genavb/ptp.h"

#include "net_socket.h"
#include "net_rx.h"
#include "net_tx.h"

void ptp_socket_unbind(struct net_socket *sock);
int ptp_socket_bind(struct net_socket *sock, struct net_address *addr);

void ptp_socket_disconnect(struct logical_port *port, struct net_socket *sock);
int ptp_socket_connect(struct logical_port *port, struct net_socket *sock, struct net_address *addr);

void ptp_tx_ts(struct net_port *phys_port, u64 ts, u32 priv);
int ptp_rx(struct net_rx_ctx *net, struct net_port *port, struct net_rx_desc *desc, void *hdr);

#endif /* _FREERTOS_PTP_H_ */
