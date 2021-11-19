/*
* Copyright 2020 NXP
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
 @brief Linux specific Network service implementation
 @details
*/

#ifndef _LINUX_NET_H_
#define _LINUX_NET_H_

#include "os/sys_types.h"
#include "osal/net.h"
#include "os_config.h"

struct net_ops_cb {
	void (*net_exit)(void);
	int (*net_rx_init)(struct net_rx *, struct net_address *, void (*func)(struct net_rx *, struct net_rx_desc *), unsigned long);
	int (*net_rx_init_multi)(struct net_rx *, struct net_address *, void (*func)(struct net_rx *, struct net_rx_desc **, unsigned int), unsigned int, unsigned int, unsigned long);
	void (*net_rx_exit)(struct net_rx *);
	struct net_rx_desc * (*__net_rx)(struct net_rx *);
	void (*net_rx)(struct net_rx *);
	void (*net_rx_multi)(struct net_rx *);

	int (*net_tx_init)(struct net_tx *, struct net_address *);
	void (*net_tx_exit)(struct net_tx *);
	int (*net_tx)(struct net_tx *, struct net_tx_desc *);
	int (*net_tx_multi)(struct net_tx *, struct net_tx_desc **, unsigned int);

	int (*net_tx_ts_get)(struct net_tx *, uint64_t *, unsigned int *);
	int (*net_tx_ts_init)(struct net_tx *, struct net_address *, void (*func)(struct net_tx *, uint64_t, unsigned int), unsigned long);
	int (*net_tx_ts_exit)(struct net_tx *);

	int (*net_add_multi)(struct net_rx *, unsigned int, const unsigned char *);
	int (*net_del_multi)(struct net_rx *, unsigned int, const unsigned char *);

	struct net_tx_desc * (*net_tx_alloc)(unsigned int);
	int (*net_tx_alloc_multi)(struct net_tx_desc **, unsigned int, unsigned int);
	struct net_tx_desc * (*net_tx_clone)(struct net_tx_desc *);
	void (*net_tx_free)(struct net_tx_desc *);
	void (*net_rx_free)(struct net_rx_desc *);
	void (*net_free_multi)(void **, unsigned int);

	unsigned int (*net_tx_available)(struct net_tx *);
	int (*net_port_status)(struct net_tx *, unsigned int, bool *, bool *, unsigned int *);
	int (*net_port_sr_config)(unsigned int, uint8_t *);
};

int net_init(struct os_net_config *config, struct os_xdp_config *xdp_config);
void net_exit(void);

int net_dflt_port_status(struct net_tx *tx, unsigned int port_id, bool *up, bool *point_to_point, unsigned int *rate);
int net_std_port_status(unsigned int port_id, bool *up, bool *point_to_point, unsigned int *rate);
int net_std_add_multi(struct net_rx *rx, unsigned int port_id, const unsigned char *hw_addr);
int net_std_del_multi(struct net_rx *rx, unsigned int port_id, const unsigned char *hw_addr);
int net_port_sr_config(unsigned int port_id, uint8_t *sr_class);
void net_std_rx_parser(struct net_rx *rx, struct net_rx_desc *desc);

#endif /* _LINUX_NET_H_ */
