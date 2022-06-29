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


#include "net_bridge.h"

#if defined(CONFIG_GENAVB_HYBRID) || defined(CONFIG_GENAVB_BRIDGE)

#if defined (CONFIG_SJA1105)
#include "sja1105.h"
#endif


/** bridge_add_multi -
 * \return 0 on success or negative value on failure
 * \param port
 * \param addr
 */
int bridge_add_multi(struct logical_port *port, struct net_mc_address *addr)
{
#if defined (CONFIG_SJA1105)
	return sja1105_add_multi(port, addr);
#else
	return -1;
#endif
}


/** bridge_del_multi -
 * \return 0 on success or negative value on failure
 * \param port
 * \param addr
 */
int bridge_del_multi(struct logical_port *port, struct net_mc_address *addr)
{
#if defined (CONFIG_SJA1105)
	return sja1105_del_multi(port, addr);
#else
	return -1;
#endif
}


/** bridge_xmit - transmit a packet through a bridge port
 * \return 0 on success or negative value on failure
 * \param port
 * \param desc
 * \param frame_type
 */
int bridge_xmit(struct logical_port *port, struct avb_tx_desc *desc, u8 frame_type)
{
	if (port->is_bridge)
		desc->common.flags |= AVB_TX_FLAG_AED_B;
	else
		desc->common.flags |= AVB_TX_FLAG_AED_E;

#if defined (CONFIG_SJA1105)
	/* the logical ethernet port is passed to the HAL. Required to later get the timestamp redirected
	to the correct handler (i.e. one of the gptp SW ports). */
	return sja1105_tx_from_host(port, desc, frame_type);
#else
	return -1;
#endif
}


/** bridge_avb_receive - receive a packet from a bridge port
 * \return 0 on success or negative value on failure
 * \param eth
 * \param desc
 * \param frame_type
 */
int bridge_receive(struct eth_avb *eth, struct net_rx_desc *desc, u8 frame_type)
{
#if defined (CONFIG_SJA1105)
	return sja1105_rx_from_network(eth, desc, frame_type);
#else
	return -1;
#endif
}

#else

int bridge_add_multi(struct logical_port *port, struct net_mc_address *addr) { return 0;}
int bridge_del_multi(struct logical_port *port, struct net_mc_address *addr){ return 0;}

#endif

