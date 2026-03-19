/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "dsa.h"

#if (defined(CFG_DSA_TAG_SUPPORT) && CFG_DSA_TAG_SUPPORT)

#include <linux/if_ether.h>
#include <net/dsa.h>

#include "genavb/ether.h"

#include "mrp.h"
#include "avtp.h"

/* This function sets the DSA port index of a given network device
 * either DSA master or slave port. The DSA port index generally maps to the
 * hardware port index and is used to map a packet to a logical port
 * depending on DSA tag information.
 *
 * Must be called with the reference to ndev held (when using a non-NULL ndev argument).
 */
void dsa_set_port(struct logical_port *port, struct net_device *ndev)
{
	struct dsa_port *dp;

	if (!port)
		goto out;

	port->dsa_port_index = -1;

	if (!ndev)
		goto out;

	/* netdev_uses_dsa() return true on DSA master only */
	if (netdev_uses_dsa(ndev))
		dp = ndev->dsa_ptr;
	else if (dsa_slave_dev_check(ndev))
		dp = dsa_port_from_netdev(ndev);
	else
		goto out;

	port->dsa_port_index = dp->index;
	pr_info("%s: logical port(%u) device(%s) has DSA port index(%d)\n",
		__func__, port->id,  netdev_name(ndev), port->dsa_port_index);

out:
	return;
}

/* This function inserts/populates the appropriate DSA tag in the
 * avb transmit descriptor and updates the desc->common.len accrodingly.
 */
void dsa_tx_add_tag(struct net_device *ndev, struct avb_tx_desc *desc)
{
	/* If network device does not use DSA (either dsa master port in hybrid
	 * mode or slave in standalone endpoint mode), exit without any further processing. */
	if (!ndev || !(netdev_uses_dsa(ndev) || dsa_slave_dev_check(ndev))) {
		return;
	}

	/* Memmove and insert the DSA tag part then exit */
}

/* This function return the logical port associated to a hardware source
 * port (DSA port index) if found. Otherwise it returns -1.
 */
static __attribute__((unused)) int dsa_get_logical_port(struct eth_avb *eth, unsigned int source_port)
{
	int i, rc = -1;

	if (eth->num_logical_ports == 1) { /* Return the only logical port mapped to the physical interface */
		rc = eth->logical_port[0]->id;
	} else {
		for ( i = 0; i < eth->num_logical_ports; i++) {
			if (eth->logical_port[i]->dsa_port_index == source_port) {
				rc = eth->logical_port[i]->id;
				break;
			}
		}
	}

	return rc;
}

/* This function parses the net rx descriptor with the DSA tag, update the descriptor fields (l3_offset/ethertype/port/vid)
 * then calls the right ethertype handler (avtp_rx(), mrp_rx() ...) otherwise net_rx_slow() to pass the packet
 * to the Linux networking stack.
 */
int dsa_rx(struct eth_avb *eth, struct net_rx_desc *desc, void* hdr)
{
	return net_rx_slow(eth, desc, &ptype_hdlr[PTYPE_OTHER].stats[desc->port]);
}
#endif /* # (defined(CFG_DSA_TAG_SUPPORT) && CFG_DSA_TAG_SUPPORT) */
