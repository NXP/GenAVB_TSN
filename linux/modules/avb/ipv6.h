/*
 * AVB 1733 (ipv6) management functions
 * Copyright 2015 Freescale Semiconductor, Inc.
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

#ifndef _IPV6_H_
#define _IPV6_H_

#include <linux/ipv6.h>
#include "net_port.h"
#include "net_socket.h"
#include "netdrv.h"

void ipv6_socket_unbind(struct net_socket *sock);
int ipv6_socket_bind(struct net_socket *sock, struct net_address *addr);
void ipv6_socket_disconnect(struct logical_port *port, struct net_socket *sock);
int ipv6_socket_connect(struct logical_port *port, struct net_socket *sock, struct net_address *addr);

struct ipv6_rx_hdlr {

};


#ifndef CFG_AVB_IPV6_1733
static inline int ipv6_rx(struct eth_avb *eth, struct net_rx_desc *desc, void *hdr, unsigned int is_vlan)
{
	return -1;
}
#else
int ipv6_rx(struct eth_avb *eth, struct net_rx_desc *desc, void *hdr, unsigned int is_vlan);
#endif

#endif /* _IPV6_H_ */
