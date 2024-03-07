/*
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 \file
 \brief GenAVB public API
 \details 802.1Q PSFP definitions.

 \copyright Copyright 2023 NXP
*/
#ifndef _GENAVB_PUBLIC_PSFP_H_
#define _GENAVB_PUBLIC_PSFP_H_

#include "genavb/types.h"

#define GENAVB_STREAM_HANDLE_WILDCARD (0xFFFFFFFF)
#define GENAVB_PRIORITY_SPEC_WILDCARD (0xFF)

/**
 * \ingroup psfp
 * PSFP Stream Filter Instance
 *  IEEE 802.1Qcw-D15 - 48.2.11 ieee802-dot1q-psfp-bridge YANG module
 */

struct genavb_stream_filter_instance {
	uint32_t stream_filter_instance_id;
	uint32_t stream_handle;
	uint8_t priority_spec;
	uint32_t max_sdu_size;
	bool stream_blocked_due_to_oversize_frame_enabled;
	bool stream_blocked_due_to_oversize_frame; 
	uint32_t stream_gate_ref;
	uint32_t flow_meter_ref;
	bool flow_meter_enable;

	uint64_t matching_frames_count;
	uint64_t passing_frames_count;
	uint64_t not_passing_frames_count;
	uint64_t red_frames_count;
	uint64_t passing_sdu_count;
	uint64_t not_passing_sdu_count;
};

/**
 * \ingroup psfp
 * PSFP Stream Gate Instance
 *  IEEE 802.1Qcw-D15 - 48.2.11 ieee802-dot1q-psfp-bridge YANG module
 */

/**
 * \ingroup psfp
 * Stream gate configuration type
 */
typedef enum {
	GENAVB_SG_OPER,
	GENAVB_SG_ADMIN
} genavb_sg_config_type_t;

/**
 * \ingroup psfp
 * Stream gate operations
 * 802.1Qci-2017 - Table 8.7 Stream Gate operations
 */
typedef enum {
	GENAVB_SG_SET_GATE_AND_IPV,    /**< SetGateAndIPV */
} genavb_sg_operations_t;

struct genavb_stream_gate_control_entry {
	uint8_t operation_name;
	uint8_t gate_state_value;
	uint8_t ipv_spec;
	uint32_t time_interval_value;
	uint32_t interval_octet_max;
};

struct genavb_stream_gate_instance {
	uint32_t stream_gate_instance_id;
	bool gate_enable;
	uint32_t cycle_time_p;
	uint32_t cycle_time_q;
	uint32_t cycle_time_extension;
	uint64_t base_time;
	unsigned int list_length;
	struct genavb_stream_gate_control_entry *control_list;
};

/**
 * \ingroup psfp
 * Flow meter 
 * IEEE 802.1Qcw-D15 - 48.2.11 ieee802-dot1q-psfp-bridge YANG module
 */

struct genavb_flow_meter_instance {
	uint32_t flow_meter_instance_id;
	uint64_t committed_information_rate;
	uint32_t committed_burst_size;
	uint64_t excess_information_rate;
	uint32_t excess_burst_size;
	uint8_t coupling_flag;
	uint8_t color_mode;
	bool drop_on_yellow;
	bool mark_all_frames_red_enable;
	bool mark_all_frames_red;
};

/* OS specific headers */
#include "os/psfp.h"

#endif /* _GENAVB_PUBLIC_PSFP_H_ */
