/*
* Copyright 2020 NXP
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
 @brief FreeRTOS specific QoS implementation
 @details
*/

#include "common/log.h"
#include "os/clock.h"
#include "clock.h"

#include "net_port.h"
#include "net_logical_port.h"

int qos_st_set_admin_config(unsigned int port_id, os_clock_id_t clk_id,
			    struct genavb_st_config *config)
{
	struct logical_port *port;
	u64 cycle_time, now;

	port = logical_port_get(port_id);
	if (!port)
		return -1;

	if (!config->enable)
		goto set_config;

	if (clock_to_hw_clock(clk_id) != clock_to_hw_clock(port->phys->clock_local)) {
		os_log(LOG_ERR, "clock id: %u has no common hw clock with port(%u)\n",
		       clk_id, port_id);
		goto err;
	}

	if (!clock_has_hw_adjust_ratio(clk_id)) {
		os_log(LOG_ERR, "clock id: %u hw clock ratio is not adjusted\n",
		       clk_id, port_id);
		goto err;
	}

	if (!config->cycle_time_p)
		goto err;

	cycle_time = ((uint64_t)NSECS_PER_SEC * config->cycle_time_p) / config->cycle_time_q;
	if (((uint64_t)cycle_time * config->cycle_time_q) !=
	    ((uint64_t)NSECS_PER_SEC * config->cycle_time_p)) {
		os_log(LOG_ERR, "only integer cycle time is supported\n");
		goto err;
	}

	/* Check and update base time.
	 * IEEE 802.1Qbv doesn't have any restrictions for base time value.
	 * If it's in the past, the schedule should start at (baseTime + N * cycleTime),
	 * with N being the minimum integer for which the time is in the future.
	 * However, some MAC don't support values in the past and this can be handled
	 * in software.
	 */
	if (os_clock_gettime64(clk_id, &now) < 0) {
		os_log(LOG_ERR, "os_clock_gettime64() error\n");
		goto err;
	}

	if (config->base_time < now) {
		uint64_t n;

		n = (now - config->base_time) / cycle_time;
		config->base_time = config->base_time + (n + 1) * cycle_time;
	}

	if (clock_time_to_cycles(clk_id, config->base_time, &config->base_time) < 0) {
		os_log(LOG_ERR, "clock_time_to_cycles error\n");
		goto err;
	}

set_config:
	return port_set_st_config(port->phys, config);

err:
	return -1;
}

int qos_st_get_config(unsigned int port_id, genavb_st_config_type_t type,
		      struct genavb_st_config *config, unsigned int list_length)
{
	struct logical_port *port;

	port = logical_port_get(port_id);
	if (!port)
		return -1;

	/* FIXME convert base time, keep track of current config in SW */
	return port_get_st_config(port->phys, type, config, list_length);
}

int qos_fp_set(unsigned int port_id, unsigned int type, struct genavb_fp_config *config)
{
	struct logical_port *port;

	port = logical_port_get(port_id);
	if (!port)
		return -1;

	return port_set_fp(port->phys, type, config);
}

int qos_fp_get(unsigned int port_id, unsigned int type, struct genavb_fp_config *config)
{
	struct logical_port *port;

	port = logical_port_get(port_id);
	if (!port)
		return -1;

	return port_get_fp(port->phys, type, config);
}
