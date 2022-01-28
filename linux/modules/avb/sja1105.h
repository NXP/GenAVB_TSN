/*
* SJA1105 glue layer functions
* Copyright 2016 Freescale Semiconductor, Inc.
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/


#ifndef _SJA1105_H_
#define _SJA1105_H_

#if defined (CONFIG_SJA1105)

#define NEW_API 1

#ifdef __KERNEL__

#include <sja1105_fops.h>

#include <linux/cdev.h>
#include <linux/string.h>
#include <linux/fec.h>

#include "switch.h"
#include "net_port.h"
#include "net_logical_port.h"
#include "pool.h"

#define NUM_TS_SLOT	(NUM_PORT*2)
#define UNUSED_TS_SLOT	0xFFFF

#define SJA_BUF_ORDER	8
#define SJA_BUF_SIZE	(1 << SJA_BUF_ORDER)
#define SJA_BUF_COUNT	(512)
#define SJA_BUF_POOL_PAGES	((SJA_BUF_COUNT * SJA_BUF_SIZE + PAGE_SIZE - 1) / PAGE_SIZE)
#define SJA_BUF_POOL_SIZE	(SJA_BUF_POOL_PAGES * PAGE_SIZE)

#define SJA_SRP_FRAME	(1 << 2)
#define SJA_PTP_FRAME	(1 << 1)
#define SJA_META_FRAME	(1 << 0)

#define SJA_MAC_SRC_PORT_BYTE  (3)
#define SJA_MAC_SWITCH_ID_BYTE (4)

#define EXT_PORT_RX_COMP_DELAY_NS (240)
#define EXT_PORT_TX_COMP_DELAY_NS (780)

#define LPBK_PORT_RX_COMP_DELAY_NS (0)
#define LPBK_PORT_TX_COMP_DELAY_NS (0)

#define FLAG_TS_RECEIVED 	(1 << 0)
#define FLAG_DESC_DONE		(1 << 1)

struct sja1105_ts_desc {
	u16 flags;
	u16 port;
	u16 slot;
	u16 index;
	u64 timestamp;
	void *data1;
	void *data2;
};

int sja1105_open(struct switch_drv *drv);
int sja1105_close(struct switch_drv *drv);
int sja1105_enable(struct switch_drv *drv);
int sja1105_disable(struct switch_drv *drv);
int sja1105_tx_from_host(struct logical_port *port, struct avb_tx_desc *desc, u8 frame_type);
int sja1105_rx_from_network(struct eth_avb *eth, struct net_rx_desc *desc, u8 frame_type);
u16 sja1105_rx_to_hal_done(void *sja_desc);
int sja1105_metaframe_rx(struct eth_avb *eth, struct net_rx_desc *desc);
//void sja1105_desc_free(struct pool *pool, unsigned long sja_desc);
void sja1105_desc_free(void *data, unsigned long sja_desc);
int sja1105_egress_ts_done(struct switch_drv *drv, struct eth_avb *eth, struct net_tx_desc *desc);
int sja1105_add_multi(struct logical_port *port, struct net_mc_address *addr);
int sja1105_del_multi(struct logical_port *port, struct net_mc_address *addr);



#endif /* __KERNEL__ */

#endif /* SJA1105 */

#endif /* _SJA1105_H_ */

