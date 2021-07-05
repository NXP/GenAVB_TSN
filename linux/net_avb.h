/*
* Copyright 2019-2021 NXP
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
 @brief Linux AVB network service implementation
 @details
*/
#ifndef _LINUX_NET_AVB_H_
#define _LINUX_NET_AVB_H_

#include "os/net.h"

int net_avb_rx_init(struct net_rx *rx, struct net_address *addr, void (*func)(struct net_rx *, struct net_rx_desc *), unsigned long epoll_fd);
int net_avb_rx_init_multi(struct net_rx *rx, struct net_address *addr, void (*func)(struct net_rx *, struct net_rx_desc **, unsigned int), unsigned int packets, unsigned int time, unsigned long epoll_fd);
void net_avb_rx_exit(struct net_rx *rx);
struct net_rx_desc *__net_avb_rx(struct net_rx *rx);
void net_avb_rx(struct net_rx *rx);

int net_avb_tx_init(struct net_tx *tx, struct net_address *addr);
void net_avb_tx_exit(struct net_tx *tx);
int net_avb_tx(struct net_tx *tx, struct net_tx_desc *desc);
int net_avb_tx_ts_get(struct net_tx *tx, uint64_t *ts, unsigned int *private);
int net_avb_tx_ts_init(struct net_tx *tx, struct net_address *addr, void (*func)(struct net_tx *, uint64_t, unsigned int), unsigned long priv);
int net_avb_tx_ts_exit(struct net_tx *tx);
unsigned int net_avb_tx_available(struct net_tx *tx);

int net_avb_port_status(struct net_tx *tx, unsigned int port_id, bool *up, bool *point_to_point, unsigned int *rate);
int net_avb_add_multi(struct net_rx *rx, unsigned int port_id, const unsigned char *hw_addr);
int net_avb_del_multi(struct net_rx *rx, unsigned int port_id, const unsigned char *hw_addr);
int net_avb_port_sr_config(unsigned int port_id, uint8_t *sr_class);

#endif /* _LINUX_NET_AVB_H_ */
