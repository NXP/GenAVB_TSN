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


#define ETT_MAX_ENTRIES 384
#define ISEQGT_MAX_ENTRIES 384
#define ESEQRT_MAX_ENTRIES 384
#define IST_MAX_ENTRIES 384

/* Stream Identification */
#define SI_MAX_HANDLE	IST_MAX_ENTRIES
#define SI_MAX_ENTRIES	IST_MAX_ENTRIES

struct netc_sw_si {
	struct {
		uint32_t eid[2]; /* ISI or IPF Table entry id returned by hardware */
	} port[CFG_NUM_NETC_SW_PORTS];

	uint16_t handle;
	uint8_t used	: 1;
	uint8_t use_isi	: 1;
	uint8_t type	: 3;
	uint8_t vlan_type : 3;
	uint8_t port_mask;
};


/* Per Stream Filtering and Policing */
#define NUM_STREAM_FILTER_ENTRIES 128

struct netc_sw_stream_filter {
	uint32_t sf_eid;
	uint32_t isc_eid;
	uint32_t stream_handle;
	uint32_t stream_gate_ref;
	uint32_t flow_meter_ref;
	uint16_t max_sdu_size;
	uint8_t priority_spec;
	uint8_t used : 1;
	uint8_t stream_blocked_due_to_oversize_frame_enabled : 1;
	uint8_t stream_blocked_due_to_oversize_frame : 1; 
	uint8_t flow_meter_enable : 1;
};

#define NUM_IPF_ENTRIES 2

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
	struct netc_sw_si si[SI_MAX_ENTRIES];
	struct netc_sw_stream_filter stream_filters[NUM_STREAM_FILTER_ENTRIES];
	uint16_t max_stream_filters_instances;
	struct netc_sw_seqg seqg_table[SEQG_MAX_ENTRIES];
	struct netc_sw_seqr seqr_table[SEQR_MAX_ENTRIES];
	struct netc_sw_seqi seqi_table[CFG_NUM_NETC_SW_PORTS];
};

#define get_drv(sw) ((struct netc_sw_drv *)sw->drv)

#define NULL_ENTRY_ID	(0xFFFFFFFF)

#define NETC_FM_EID(SQTA, VUDA) ((1 << 13) | ((SQTA) << 2) | (VUDA))
#define NETC_FM_DEL_VLAN_TAG NETC_FM_EID(0, 2)
#define NETC_FM_DEL_VLAN_SQ_TAG NETC_FM_EID(1, 2)

extern uint8_t netc_sw_port_to_net_port[CFG_NUM_NETC_SW_PORTS];
extern const uint8_t port_to_netc_sw_port[CFG_BR_DEFAULT_NUM_PORTS];

int netc_sw_vft_find_entry(struct netc_sw_drv *drv, uint16_t vid, uint32_t *entry_id, netc_tb_vf_cfge_t *cfge);
int netc_sw_add_or_update_et_table_entry(struct netc_sw_drv *drv, uint32_t et_eid, void (*update_entry)(netc_tb_et_config_t *entry, void *data), void *data);

#endif

#endif /* _FREERTOS_NET_PORT_NETC_SW_DRV_H_ */

