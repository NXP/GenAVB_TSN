/*
* Copyright 2023 NXP
*
* SPDX-License-Identifier: BSD-3-Clause
*/

/**
 @file
 @brief FreeRTOS specific PSFP service implementation
 @details
*/

#include "os/psfp.h"
#include "common/log.h"
#include "net_bridge.h"


#if CFG_BRIDGE_NUM
int stream_filter_update(uint32_t index, struct genavb_stream_filter_instance *instance)
{
	struct net_bridge *bridge = bridge_get(DEFAULT_BRIDGE_ID);
	int rc = -1;

	xSemaphoreTake(bridge->mutex, portMAX_DELAY);

	if (bridge->drv_ops.stream_filter_update)
		rc = bridge->drv_ops.stream_filter_update(bridge, index, instance);

	xSemaphoreGive(bridge->mutex);

	return rc;
}

int stream_filter_delete(uint32_t index)
{
	struct net_bridge *bridge = bridge_get(DEFAULT_BRIDGE_ID);
	int rc = -1;

	xSemaphoreTake(bridge->mutex, portMAX_DELAY);

	if (bridge->drv_ops.stream_filter_delete)
		rc = bridge->drv_ops.stream_filter_delete(bridge, index);

	xSemaphoreGive(bridge->mutex);

	return rc;
}

int stream_filter_read(uint32_t index, struct genavb_stream_filter_instance *instance)
{
	struct net_bridge *bridge = bridge_get(DEFAULT_BRIDGE_ID);
	int rc = -1;

	xSemaphoreTake(bridge->mutex, portMAX_DELAY);

	if (bridge->drv_ops.stream_filter_read)
		rc = bridge->drv_ops.stream_filter_read(bridge, index, instance);

	xSemaphoreGive(bridge->mutex);

	return rc;
}

unsigned int stream_filter_get_max_entries(void)
{
	struct net_bridge *bridge = bridge_get(DEFAULT_BRIDGE_ID);
	unsigned int max_entries = 0;

	xSemaphoreTake(bridge->mutex, portMAX_DELAY);

	if (bridge->drv_ops.stream_filter_get_max_entries)
		max_entries =  bridge->drv_ops.stream_filter_get_max_entries(bridge);

	xSemaphoreGive(bridge->mutex);

	return max_entries;
}

int stream_gate_update(uint32_t index, struct genavb_stream_gate_instance *instance)
{
	struct net_bridge *bridge = bridge_get(DEFAULT_BRIDGE_ID);
	int rc = -1;

	xSemaphoreTake(bridge->mutex, portMAX_DELAY);

	if (bridge->drv_ops.stream_gate_update)
		rc = bridge->drv_ops.stream_gate_update(bridge, index, instance);

	xSemaphoreGive(bridge->mutex);

	return rc;
}

int stream_gate_delete(uint32_t index)
{
	struct net_bridge *bridge = bridge_get(DEFAULT_BRIDGE_ID);
	int rc = -1;

	xSemaphoreTake(bridge->mutex, portMAX_DELAY);

	if (bridge->drv_ops.stream_gate_delete)
		rc = bridge->drv_ops.stream_gate_delete(bridge, index);

	xSemaphoreGive(bridge->mutex);

	return rc;
}

int stream_gate_read(uint32_t index, struct genavb_stream_gate_instance *instance)
{
	struct net_bridge *bridge = bridge_get(DEFAULT_BRIDGE_ID);
	int rc = -1;

	xSemaphoreTake(bridge->mutex, portMAX_DELAY);

	if (bridge->drv_ops.stream_gate_read)
		rc = bridge->drv_ops.stream_gate_read(bridge, index, instance);

	xSemaphoreGive(bridge->mutex);

	return rc;
}

unsigned int stream_gate_get_max_entries(void)
{
	struct net_bridge *bridge = bridge_get(DEFAULT_BRIDGE_ID);
	unsigned int max_entries = 0;

	xSemaphoreTake(bridge->mutex, portMAX_DELAY);

	if (bridge->drv_ops.stream_gate_get_max_entries)
		max_entries = bridge->drv_ops.stream_gate_get_max_entries(bridge);

	xSemaphoreGive(bridge->mutex);

	return max_entries;
}

unsigned int stream_gate_control_get_max_entries(void)
{
	struct net_bridge *bridge = bridge_get(DEFAULT_BRIDGE_ID);
	unsigned int max_entries = 0;

	xSemaphoreTake(bridge->mutex, portMAX_DELAY);

	if (bridge->drv_ops.stream_gate_control_get_max_entries)
		max_entries = bridge->drv_ops.stream_gate_control_get_max_entries(bridge);

	xSemaphoreGive(bridge->mutex);

	return max_entries;
}
#else
int stream_filter_update(uint32_t index, struct genavb_stream_filter_instance *instance) { return -1; }
int stream_filter_delete(uint32_t index) { return -1; }
int stream_filter_read(uint32_t index, struct genavb_stream_filter_instance *instance) { return -1; }
unsigned int stream_filter_get_max_entries(void) { return 0; }

int stream_gate_update(uint32_t index, struct genavb_stream_gate_instance *instance) { return -1; }
int stream_gate_delete(uint32_t index) { return -1; }
int stream_gate_read(uint32_t index, struct genavb_stream_gate_instance *instance) { return -1; }
unsigned int stream_gate_get_max_entries(void) { return 0; }
unsigned int stream_gate_control_get_max_entries(void) { return 0; }
#endif
