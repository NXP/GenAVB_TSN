/*
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief PSFP OS abstraction
 @details
*/

#ifndef _OS_PSFP_H_
#define _OS_PSFP_H_

#include "os/clock.h"
#include "genavb/psfp.h"

int stream_filter_update(uint32_t index, struct genavb_stream_filter_instance *instance);
int stream_filter_delete(uint32_t index);
int stream_filter_read(uint32_t index, struct genavb_stream_filter_instance *instance);
unsigned int stream_filter_get_max_entries(void);

int stream_gate_update(uint32_t index, os_clock_id_t clk_id, struct genavb_stream_gate_instance *instance);
int stream_gate_delete(uint32_t index);
int stream_gate_read(uint32_t index, genavb_sg_config_type_t type, struct genavb_stream_gate_instance *instance);
unsigned int stream_gate_get_max_entries(void);
unsigned int stream_gate_control_get_max_entries(void);

int flow_meter_update(uint32_t index, struct genavb_flow_meter_instance *instance);
int flow_meter_delete(uint32_t index);
int flow_meter_read(uint32_t index, struct genavb_flow_meter_instance *instance);
unsigned int flow_meter_get_max_entries(void);

#endif /* _OS_PSFP_H_ */
