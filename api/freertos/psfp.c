/*
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "genavb/psfp.h"
#include "genavb/error.h"
#include "api/clock.h"


int genavb_stream_filter_update(uint32_t index, struct genavb_stream_filter_instance *instance)
{
	int rc;

	rc = stream_filter_update(index, instance);

	return rc;
}

int genavb_stream_filter_delete(uint32_t index)
{
	int rc;

	rc = stream_filter_delete(index);

	return rc;
}

int genavb_stream_filter_read(uint32_t index, struct genavb_stream_filter_instance *instance)
{
	int rc;

	rc = stream_filter_read(index, instance);

	return rc;
}

unsigned int genavb_stream_filter_get_max_entries(void)
{
	return stream_filter_get_max_entries();
}

int genavb_stream_gate_update(uint32_t index, genavb_clock_id_t clk_id, struct genavb_stream_gate_instance *instance)
{
	os_clock_id_t os_clk_id;
	int rc;

	if (clk_id >= GENAVB_CLOCK_MAX) {
		rc = -GENAVB_ERR_CLOCK;
		goto err;
	}

	os_clk_id = genavb_clock_to_os_clock(clk_id);
	if (os_clk_id >= OS_CLOCK_MAX) {
		rc = -GENAVB_ERR_CLOCK;
		goto err;
	}

	rc = stream_gate_update(index, os_clk_id, instance);

err:
	return rc;
}

int genavb_stream_gate_delete(uint32_t index)
{
	int rc;

	rc = stream_gate_delete(index);

	return rc;
}

int genavb_stream_gate_read(uint32_t index, genavb_sg_config_type_t type, struct genavb_stream_gate_instance *instance)
{
	int rc;

	rc = stream_gate_read(index, type, instance);

	return rc;
}

unsigned int genavb_stream_gate_get_max_entries(void)
{
	return stream_gate_get_max_entries();
}

unsigned int genavb_stream_gate_control_get_max_entries(void)
{
	return stream_gate_control_get_max_entries();
}