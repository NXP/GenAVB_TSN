/*
 * AVB ethernet bridge functions
 * Copyright 2018 NXP
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


#ifndef _NET_BRIDGE_H_
#define _NET_BRIDGE_H_

#include "genavb/net_types.h"
#include "net_logical_port.h"
#include "net_port.h"
#include "avbdrv.h"


int bridge_add_multi(struct logical_port *port, struct net_mc_address *addr);
int bridge_del_multi(struct logical_port *port, struct net_mc_address *addr);
int bridge_xmit(struct logical_port *port, struct avb_tx_desc *desc, u8 frame_type);
int bridge_receive(struct eth_avb *eth, struct net_rx_desc *rx_desc, u8 frame_type);

#endif /* _NET_BRIDGE_H_ */
