/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef _AVB_DSA_H_
#define _AVB_DSA_H_

#include <linux/netdevice.h>

#include "port_config.h"
#include "net_logical_port.h"


#if (defined(CFG_DSA_TAG_SUPPORT) && CFG_DSA_TAG_SUPPORT)
#include <linux/fec.h>

#include "genavb/net_types.h"
#include "net_port.h"

void dsa_tx_add_tag(struct net_device *ndev, struct avb_tx_desc *desc);
int dsa_rx(struct eth_avb *eth, struct net_rx_desc *desc, void *hdr);
void dsa_set_port(struct logical_port *port, struct net_device *ndev);

#else

static inline void dsa_set_port(struct logical_port *port, struct net_device *ndev)
{
	return;
}
#endif /* defined(CFG_DSA_TAG_SUPPORT) && CFG_DSA_TAG_SUPPORT */

#endif /* _AVB_DSA_H_ */
