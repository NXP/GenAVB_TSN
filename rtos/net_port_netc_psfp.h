/*
* Copyright 2023-2025 NXP
*
* SPDX-License-Identifier: BSD-3-Clause
*/

/**
 @file
 @brief RTOS specific Per Stream Filtering and Policing implementation
 @details
*/

#ifndef _RTOS_NET_PORT_NETC_PSFP_H_
#define _RTOS_NET_PORT_NETC_PSFP_H_

#include "fsl_netc_switch.h"
#include "net_bridge.h"

/* Per Stream Filtering and Policing */
#define NUM_STREAM_FILTER_ENTRIES 128

#define NUM_STREAM_GATES_ENTRIES 32

struct netc_sg {
	uint32_t prev_admin_eid;
	uint32_t admin_eid;
};

struct netc_stream_filter {
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

struct netc_sw_drv;

struct netc_psfp {
	struct netc_stream_filter stream_filters[NUM_STREAM_FILTER_ENTRIES];
	uint16_t max_stream_filters_instances;
	struct netc_sg sg_table[NUM_STREAM_GATES_ENTRIES];
	union {
		swt_handle_t *handle;
	} hw;
	struct netc_sw_drv *si;
};

int netc_sw_psfp_init(struct net_psfp_ops *psfp_ops);
void netc_sw_psfp_exit(struct net_psfp_ops *psfp_ops);
int netc_sw_psfp_get_eid(void *drv, uint32_t handle, uint32_t *rp_eid, uint32_t *sg_eid, uint32_t *isc_eid, bool *sf_enable, uint16_t *msdu);
#endif /* _RTOS_NET_PORT_NETC_PSFP_H_ */
