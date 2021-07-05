/*
* Copyright 2017-2021 NXP
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
 @brief Linux specific NET implementation
 @details
*/

#ifndef _LINUX_OSAL_NET_H_
#define _LINUX_OSAL_NET_H_

#include "common/net_types.h"
#include "epoll.h"
#include "os/clock.h"
#include "os/string.h"

#define DEFAULT_NET_DATA_SIZE 1600

struct net_tx {
	int fd;
	int epoll_fd;
	int port_id;
	void (*func_tx_ts)(struct net_tx *, uint64_t ts, unsigned int private);
	struct linux_epoll_data epoll_data;
	u8 eth_src[6];
	os_clock_id_t clock_domain; /* clock domain to which hw timestamps must be converted */
};

struct net_rx {
	int fd;
	int epoll_fd;
	int port_id;
	void (*func)(struct net_rx *, struct net_rx_desc *);
	void (*func_multi)(struct net_rx *, struct net_rx_desc **, unsigned int);
	struct linux_epoll_data epoll_data;
	unsigned int batch;
	bool is_ptp;
	os_clock_id_t clock_domain; /* clock domain to which hw timestamps must be converted */
};

#endif /* _LINUX_OSAL_NET_H_ */
