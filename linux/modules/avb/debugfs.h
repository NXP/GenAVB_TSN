/*
 * AVB debugfs driver
 * Copyright 2014-2015 Freescale Semiconductor, Inc.
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

#ifndef _DEBUGFS_H_
#define _DEBUGFS_H_

#include "dmadrv.h"

struct dentry *avb_debugfs_init(void);
void avb_debugfs_exit(struct dentry *dentry);

void net_rx_debugfs_init(struct eth_avb *eth, struct dentry *avb_dentry);

void net_qos_debugfs_init(struct net_qos *net, struct dentry *avb_dentry);

struct dentry *mclock_debugfs_init(struct dentry *avb_dentry);

struct dentry *mclock_gen_ptp_debugfs_init(struct mclock_drv *drv, struct mclock_gen_ptp_stats *stats, int port);
void mclock_gen_ptp_debugfs_exit(struct dentry *mclock_dentry);
void mclock_rec_pll_debugfs_init(struct mclock_drv *drv, struct mclock_rec_pll *rec, int domain);
void mclock_dma_debugfs_init(struct mclock_dma_stats *stats, struct dentry *mclock_dentry, char *dma_name, int domain);


void hw_timer_debugfs_init(struct hw_timer *timer, struct dentry *avb_dentry);

#if defined(CONFIG_HYBRID) || defined(CONFIG_BRIDGE)
void switch_debugfs_init(struct switch_drv *drv, struct dentry *avb_dentry);
#endif

#endif /* _DEBUGFS_H_ */
