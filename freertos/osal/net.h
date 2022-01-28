/*
* Copyright 2018-2020 NXP
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
 @brief FreeRTOS specific NET implementation
 @details
*/
#ifndef _FREERTOS_OSAL_NET_H_
#define _FREERTOS_OSAL_NET_H_

#include "common/net_types.h"

#define DEFAULT_NET_DATA_SIZE		1600

struct net_tx {
	void (*func_tx_ts)(struct net_tx *, uint64_t ts, unsigned int private);

	void *socket;

	unsigned int port_id;
};

struct net_rx {
	void (*func)(struct net_rx *, struct net_rx_desc *);
	void (*func_multi)(struct net_rx *, struct net_rx_desc **, unsigned int);

	void *socket;

	unsigned int batch;
};

int net_rx_set_callback(struct net_rx*, void (*callback)(void *), void *data);
int net_rx_enable_callback(struct net_rx *rx);

#endif /* _FREERTOS_OSAL_NET_H_ */
