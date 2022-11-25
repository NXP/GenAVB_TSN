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
 @brief Packet socket management functions
 @details
*/

#ifndef _FREERTOS_PACKET_H_
#define _FREERTOS_PACKET_H_

#include "net_socket.h"
#include "net_rx.h"
#include "slist.h"

#define L2_HASH	16

struct l2_rx_hdlr {
	struct slist_head sock_list_head[L2_HASH];
};

void l2_socket_unbind(struct net_socket *sock);
int l2_socket_bind(struct net_socket *sock, struct net_address *addr);

void l2_socket_disconnect(struct logical_port *port, struct net_socket *sock);
int l2_socket_connect(struct logical_port *port, struct net_socket *sock, struct net_address *addr);

int l2_rx(struct net_rx_ctx *net, struct net_port *port, struct net_rx_desc *desc);

static inline unsigned int l2_hash(void *dst_mac, uint16_t vlan_id)
{
	u8 *data = dst_mac;

	return (data[0] ^ data[1] ^ data[2] ^ data[3] ^ data[4] ^ data[5]
		^ ((vlan_id >> 8) & 0xFF) ^ (vlan_id & 0xFF))
		& (L2_HASH - 1);
}

#endif /* _FREERTOS_PACKET_H_ */


