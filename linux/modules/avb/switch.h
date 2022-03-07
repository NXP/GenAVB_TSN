/*
* Switch interface
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


#ifndef _SWITCH_H_
#define _SWITCH_H_

#ifdef __KERNEL__

#include "queue.h"
#include "net_port.h"

#define NUM_PORT	5 /* 4 connected to external EPs + 1 connected to local EP (via host port) */

struct switch_stats {
	/* Buffering counters */
	unsigned long rx_queue_lvl;
	unsigned long rx_queue_full;
	unsigned long rx_enqueue_count;
	unsigned long rx_dequeue_count;

	unsigned long tx_queue_ep_count;
	unsigned long tx_queue_ep_lvl;
	unsigned long tx_queue_ep_full;

	unsigned long tx_queue_sw_count;
	unsigned long tx_queue_sw_lvl;
	unsigned long tx_queue_sw_full;

	/* Rx counters */
	unsigned long rx_from_network;
	unsigned long rx_from_network_err;
	unsigned long rx_from_network_ptp;
	unsigned long rx_from_network_srp;
	unsigned long rx_from_network_meta;

	unsigned long rx_to_host_ep;
	unsigned long rx_to_host_ep_ptp;
	unsigned long rx_to_host_ep_srp;
	unsigned long rx_to_host_ep_meta;

	unsigned long rx_to_host_sw[NUM_PORT];
	unsigned long rx_to_host_sw_srp[NUM_PORT];
	unsigned long rx_to_host_sw_ptp[NUM_PORT];
	unsigned long rx_to_host_sw_meta;

	/* Tx counters */
	unsigned long tx_from_host_ep;
	unsigned long tx_from_host_ep_ptp;
	unsigned long tx_from_host_ep_srp;
	unsigned long tx_from_host_ep_err;

	unsigned long tx_from_host_sw[NUM_PORT];
	unsigned long tx_from_host_sw_ptp[NUM_PORT];
	unsigned long tx_from_host_sw_srp[NUM_PORT];
	unsigned long tx_from_host_sw_err[NUM_PORT];

	unsigned long tx_to_network_ep_ptp;
	unsigned long tx_to_network_ep_ptp_err;
	unsigned long tx_to_network_ep_srp;
	unsigned long tx_to_network_ep_srp_err;

	unsigned long tx_to_network_sw_ptp[NUM_PORT];
	unsigned long tx_to_network_sw_ptp_err[NUM_PORT];
	unsigned long tx_to_network_sw_srp[NUM_PORT];
	unsigned long tx_to_network_sw_srp_err[NUM_PORT];

	/* Memory allocations counters */
	unsigned long num_tx_desc_alloc;
	unsigned long num_rx_desc_alloc;
	unsigned long num_desc_free;
	unsigned long num_desc_free_rx;
	unsigned long num_desc_free_rx_ep;
	unsigned long num_desc_free_rx_sw;
	unsigned long num_desc_free_tx;
	unsigned long num_alloc_err;

	/* Switch's egress timestamp counters */
	unsigned long num_egress_ts[NUM_PORT];
	unsigned long num_egress_ts_alloc[NUM_PORT];
	unsigned long num_egress_ts_free[NUM_PORT];
	unsigned long num_egress_ts_alloc_err[NUM_PORT];
	unsigned long num_egress_ts_store_err[NUM_PORT];
	unsigned long num_egress_ts_lookup_err;
	unsigned long egress_ts_queue_full;
	unsigned long egress_ts_queue_lvl;
	unsigned long egress_ts_queue_count;
	unsigned long egress_ts_dequeue_count;

	/* Miscs errors*/
	unsigned long num_sja_tick_err;
};


struct switch_drv {
	struct queue rx_queue;
	unsigned long rx_queue_extra_storage[CFG_RX_EXTRA_ENTRIES]; /* Must follow rx_queue member */
	struct queue tx_queue_ep;
	struct queue tx_queue_sw;

	struct port_qos *qos;
	struct queue tx_queue;
	struct qos_queue *tx_qos_queue;

	struct queue egress_ts_queue;

	struct mutex lock;
	u8 ready;

	struct switch_stats stats;
	struct task_struct *kthread;
	struct pool buf_pool;
	void *buf_baseaddr;

	struct cdev cdev;
	dev_t devno;
};


int switch_init(struct switch_drv *drv, struct net_qos *qos, struct dentry *avb_dentry);
void switch_exit(struct switch_drv *drv);

#endif /* __KERNEL__ */

#endif /* _SWITCH_H_ */

