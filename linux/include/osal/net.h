/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2021, 2023-2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
#include "common/ipc.h"

#define DEFAULT_NET_DATA_SIZE 1600

struct net_rx;
struct net_tx;

struct net_tx_ops_cb {
	int (*net_tx_init)(struct net_tx *tx, struct net_address *addr);
	void (*net_tx_exit)(struct net_tx *tx);

	int (*net_tx)(struct net_tx *tx, struct net_tx_desc *desc);
	int (*net_tx_multi)(struct net_tx *tx, struct net_tx_desc **desc, unsigned int n);

	struct net_tx_desc * (*net_tx_alloc)(unsigned int size);
	int (*net_tx_alloc_multi)(struct net_tx_desc **desc, unsigned int n, unsigned int size);
	struct net_tx_desc * (*net_tx_clone)(struct net_tx_desc *desc);

	int (*net_tx_ts_get)(struct net_tx *tx, uint64_t *ts, unsigned int *priv);
	int (*net_tx_ts_init)(struct net_tx *tx, struct net_address *addr, void (*func)(struct net_tx *tx, uint64_t ts, unsigned int ts_info), unsigned long priv);
	int (*net_tx_ts_exit)(struct net_tx *tx);

	unsigned int (*net_tx_available)(struct net_tx *tx);
	int (*net_port_status)(struct net_tx *tx, unsigned int port_id, bool *up, bool *point_to_point, uint64_t *rate);
	unsigned int (*net_port_mtu_get)(unsigned int port_id);
};

struct net_rx_ops_cb {
	int (*net_rx_init)(struct net_rx *rx, struct net_address *addr, void (*func)(struct net_rx *rx, struct net_rx_desc *desc), unsigned long priv);
	int (*net_rx_init_multi)(struct net_rx *rx, struct net_address *addr, void (*func)(struct net_rx *rx, struct net_rx_desc **desc, unsigned int n), unsigned int rx_batch, unsigned int rx_latency, unsigned long priv);

	void (*net_rx_exit)(struct net_rx *rx);
	void (*net_rx)(struct net_rx *rx);
	int (*__net_rx_multi)(struct net_rx *rx, struct net_rx_desc **desc, unsigned int n);
	void (*net_rx_multi)(struct net_rx *rx);

	int (*net_add_multi)(struct net_rx *rx, unsigned int port_id, const unsigned char *hw_addr);
	int (*net_del_multi)(struct net_rx *rx, unsigned int port_id, const unsigned char *hw_addr);
};

struct net_tx {
	int fd;
	int epoll_fd;
	int port_id;
	void (*func_tx_ts)(struct net_tx *tx, uint64_t ts, unsigned int private);
	struct linux_epoll_data epoll_data;
	u8 eth_src[6];
	os_clock_id_t clock_domain; /* clock domain to which hw timestamps must be converted */
	void *priv;
	unsigned int pool_type;
	struct net_tx_ops_cb net_ops;
	struct ipc_tx ipc_tx; /* used for sending network buffers over IPC */
};

struct net_rx {
	int fd;
	int epoll_fd;
	int port_id;
	void (*func)(struct net_rx *rx, struct net_rx_desc *desc);
	void (*func_multi)(struct net_rx *rx, struct net_rx_desc **desc, unsigned int n);
	struct linux_epoll_data epoll_data;
	unsigned int batch;
	os_clock_id_t clock_domain; /* clock domain to which hw timestamps must be converted */
	void *priv;
	unsigned int pool_type;
	struct net_rx_ops_cb net_ops;
	struct ipc_rx ipc_rx; /* used for sending network buffers over IPC */
};

struct net_mem_ops_cb {
	void (*net_tx_free)(struct net_tx_desc *desc);

	void (*net_rx_free)(struct net_rx_desc *desc);

	void (*net_free_multi)(void **desc, unsigned int n);
};

#endif /* _LINUX_OSAL_NET_H_ */
