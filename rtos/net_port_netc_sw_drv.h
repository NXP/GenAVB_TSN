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

#ifndef _RTOS_NET_PORT_NETC_SW_DRV_H_
#define _RTOS_NET_PORT_NETC_SW_DRV_H_

#if CFG_NUM_NETC_SW

#include "fsl_netc.h"
#include "fsl_netc_switch.h"

#include "hw_clock.h"
#include "net_bridge.h"
#include "net_port_netc_psfp.h"
#include "net_port_netc_stream_identification.h"

#ifdef CONFIG_GENAVB_TSN_DSA
#include "net_port_dsa.h"
#endif

#define NUM_TX_TS 32

struct tx_ts_info_t {
	uint8_t state;
	uint16_t age;
	uint16_t tx_ts_id;
	uint32_t timestamp;
	struct net_tx_desc *desc;
	struct net_port *port;
};

#define ETT_MAX_ENTRIES 384
#define ISEQGT_MAX_ENTRIES 384
#define ESEQRT_MAX_ENTRIES 384

#define NUM_IPF_ENTRIES 4

/* FRER */
#define SEQG_MAX_ENTRIES ISEQGT_MAX_ENTRIES
#define SEQR_MAX_ENTRIES ESEQRT_MAX_ENTRIES

struct netc_sw_seqg {
	uint16_t *stream_handle;
	uint16_t stream_n;
	uint8_t programmed : 1;
};

struct netc_sw_seqr {
	uint16_t *stream_handle;
	uint16_t stream_n;
	uint8_t port_map;
};

struct netc_sw_seqi {
	uint16_t stream_handle[SI_MAX_ENTRIES];
	uint16_t stream_n;
	uint8_t tag : 4;
	uint8_t active : 1;
};

#define NUM_TC		8	/* Number of traffic classes, priorities and buffer pools */

struct netc_sw_drv {
	NETC_ENETC_Type *base;
	bool use_masquerade;
	u8 st_enabled;
	u8 fp_enabled;
	u8 mlo;
	swt_handle_t handle;
	ep_handle_t *ep_handle;
	struct tx_ts_info_t tx_ts[NUM_TX_TS];
	uint32_t ipf_entries_id[NUM_IPF_ENTRIES];
	uint16_t num_ipf_entries;
	uint16_t tc_bytes[NUM_TC];
	uint8_t ports;	/* total enabled ports */
	struct net_port *port[CFG_NUM_NETC_SW_PORTS];
	struct netc_psfp psfp;
	struct netc_si si;
	struct netc_sw_seqg seqg_table[SEQG_MAX_ENTRIES];
	struct netc_sw_seqr seqr_table[SEQR_MAX_ENTRIES];
	struct netc_sw_seqi seqi_table[CFG_NUM_NETC_SW_PORTS];
	rtos_atomic_t *fm_table;
	unsigned int fm_table_entries;
#ifdef CONFIG_GENAVB_TSN_DSA
	uint8_t dsa_cpu_port;
#endif
};

#define NULL_ENTRY_ID	(0xFFFFFFFF)

/* Egress Treatment Base group for vlan untagged */
#define NETC_UNTAGGED_VLAN_ETT_BASE 0

/* Ingress Port Filter entry priority */
#define NETC_IPF_PRECEDENCE_MAX		0xffff
#define NETC_IPF_PRECEDENCE_DEFAULT	0x8000
#define NETC_IPF_PRECEDENCE_MIN		0x0000

extern uint8_t netc_sw_port_to_net_port[CFG_NUM_NETC_SW_PORTS];
extern const uint8_t port_to_netc_sw_port[CFG_BR_DEFAULT_NUM_PORTS];

int netc_sw_vft_find_entry(struct netc_sw_drv *drv, uint16_t vid, uint32_t *entry_id, netc_tb_vf_cfge_t *cfge);
int netc_sw_add_or_update_et_table_entry(struct netc_sw_drv *drv, uint32_t et_eid, void (*update_entry)(struct netc_sw_drv *drv, netc_tb_et_config_t *entry, void *data), void *data);
int netc_sw_delete_et_table_entry(struct netc_sw_drv *drv, uint32_t et_eid);
uint8_t netc_sw_dsa_cpu_port(struct netc_sw_drv *drv);

static inline uint32_t netc_sw_get_stream_et_eid(uint32_t stream_handle, unsigned int port_index)
{
	return NETC_UNTAGGED_VLAN_ETT_BASE + CFG_NUM_NETC_SW_PORTS + (stream_handle * CFG_NUM_NETC_SW_PORTS) + port_index;
}

#endif

#endif /* _RTOS_NET_PORT_NETC_SW_DRV_H_ */

