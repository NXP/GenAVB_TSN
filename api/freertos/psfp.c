/*
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "genavb/psfp.h"

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

int genavb_stream_gate_update(uint32_t index, struct genavb_stream_gate_instance *instance)
{
	int rc;

	rc = stream_gate_update(index, instance);

	return rc;
}

int genavb_stream_gate_delete(uint32_t index)
{
	int rc;

	rc = stream_gate_delete(index);

	return rc;
}

int genavb_stream_gate_read(uint32_t index, struct genavb_stream_gate_instance *instance)
{
	int rc;

	rc = stream_gate_read(index, instance);

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