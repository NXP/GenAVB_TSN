/*
* Copyright 2023-2025 NXP
*
* SPDX-License-Identifier: BSD-3-Clause
*/

/**
 @file
 @brief RTOS specific Network service implementation
 @details
*/

#ifndef _RTOS_NET_PORT_NETC_STREAM_IDENTIFICATION_H_
#define _RTOS_NET_PORT_NETC_STREAM_IDENTIFICATION_H_

#if CFG_NUM_NETC_SW
#include "fsl_netc_switch.h"
#endif
#include "net_bridge.h"

/* Stream Identification */
#define IST_MAX_ENTRIES 384

#define SI_MAX_HANDLE	IST_MAX_ENTRIES
#define SI_MAX_ENTRIES	IST_MAX_ENTRIES
#define NUM_PORT CFG_NUM_NETC_SW_PORTS

struct netc_si_entry {
	struct {
		uint32_t eid[2]; /* ISI or IPF Table entry id returned by hardware */
	} port[NUM_PORT];

	uint16_t handle;
	uint8_t used	: 1;
	uint8_t use_isi	: 1;
	uint8_t type	: 3;
	uint8_t vlan_type : 3;
	uint8_t port_mask;
};

struct netc_si {
    struct netc_si_entry entry[SI_MAX_ENTRIES];
    uint16_t total_ports;
	union {
#if CFG_NUM_NETC_SW
    struct netc_sw_drv *sw;
#endif
	} drv;
	union {
#if CFG_NUM_NETC_SW
		swt_handle_t *handle;
#endif
	} hw;
	struct net_port **port;
};

int netc_si_init(struct net_si_ops *si_ops);
int netc_si_update_sf_ref(void *drv, uint32_t handle);
void netc_si_free_fm_eid(void *sw_drv, uint32_t eid);
int netc_si_update_frer_ref(void *sw_drv, uint32_t handle);
int netc_si_update_et_rtag(void *sw_drv, uint32_t stream_handle, unsigned int port_index, bool del_tag);

#endif /* _RTOS_NET_PORT_NETC_STREAM_IDENTIFICATION_H_ */

