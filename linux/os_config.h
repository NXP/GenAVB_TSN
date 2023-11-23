/*
* Copyright 2020, 2023 NXP
*
* SPDX-License-Identifier: BSD-3-Clause
*/

#ifndef _LINUX_OS_CONFIG_H_
#define _LINUX_OS_CONFIG_H_

#include "genavb/config.h"

typedef enum {
	NET_AVB = 1,
	NET_STD,
	NET_XDP,
} network_mode_t;

struct os_config {
	struct os_logical_port_config {
		char endpoint[CFG_MAX_ENDPOINTS][32];
		char bridge[CFG_MAX_BRIDGES][32];
		char bridge_ports[CFG_MAX_BRIDGES][CFG_MAX_NUM_PORT][32];
	} logical_port_config;

	struct os_clock_config {
		char endpoint_gptp[CFG_MAX_GPTP_DOMAINS][CFG_MAX_ENDPOINTS][32];
		char endpoint_local[CFG_MAX_ENDPOINTS][32];
		char bridge_gptp[CFG_MAX_GPTP_DOMAINS][CFG_MAX_BRIDGES][32];
		char bridge_local[CFG_MAX_BRIDGES][32];
	} clock_config;

	struct os_net_config {
		network_mode_t net_mode;
	} net_config;

	struct os_xdp_config {
		int endpoint_queue_rx[CFG_MAX_ENDPOINTS];
		int endpoint_queue_tx[CFG_MAX_ENDPOINTS];
	} xdp_config;
};

int os_config_get(struct os_config *config);

#endif /* _LINUX_OS_CONFIG_H_ */
