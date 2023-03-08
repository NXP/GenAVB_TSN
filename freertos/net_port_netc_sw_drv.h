/*
* Copyright 2023 NXP
*
* SPDX-License-Identifier: BSD-3-Clause
*/

/**
 @file
 @brief FreeRTOS specific Network service implementation
 @details
*/

#ifndef _FREERTOS_NET_PORT_NETC_SW_DRV_H_
#define _FREERTOS_NET_PORT_NETC_SW_DRV_H_

#if CFG_NUM_NETC_SW

#include "fsl_netc.h"
#include "fsl_netc_switch.h"

#include "hw_clock.h"

#define NUM_TX_TS 32

struct tx_ts_info_t {
	uint8_t state;
	uint16_t age;
	uint16_t tx_ts_id;
	uint32_t timestamp;
	struct net_tx_desc *desc;
	struct net_port *port;
};

#define NUM_IPF_ENTRIES 2
#define NUM_TC		8	/* Number of traffic classes, priorities and buffer pools */

struct netc_sw_drv {
	NETC_ENETC_Type *base;
	u8 refcount;
	bool use_masquerade;
	u8 st_enabled;
	swt_handle_t handle;
	swt_config_t config;
	swt_transfer_config_t tx_rx_config;
	ep_handle_t *ep_handle;
	struct hw_clock clock;
	os_clock_id_t clock_local;
	void *timer_handle;
	struct tx_ts_info_t tx_ts[NUM_TX_TS];
	uint32_t ipf_entries_id[NUM_IPF_ENTRIES];
	uint16_t num_ipf_entries;
	uint16_t tc_words_log2[NUM_TC];
	uint8_t word_size;
	uint8_t ports;	/* total enabled ports */
	struct net_port *port[CFG_NUM_NETC_SW_PORTS];
};

#define get_drv(sw) ((struct netc_sw_drv *)sw->drv)

#define NULL_ENTRY_ID	(0xFFFFFFFF)

extern const uint8_t netc_sw_port_to_net_port[CFG_BR_DEFAULT_NUM_PORTS];

#endif

#endif /* _FREERTOS_NET_PORT_NETC_SW_DRV_H_ */

