/*
 * Copyright 2023, 2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief RTOS specific stream identification implementation
 @details
*/

#include "os/stream_identification.h"
#include "common/log.h"
#include "net_logical_port.h"
#include "net_bridge.h"

#if CFG_BRIDGE_NUM

int stream_identity_update(uint32_t index, struct genavb_stream_identity *entry)
{
	struct net_bridge *bridge = bridge_get(DEFAULT_BRIDGE_ID);
	struct net_si_ops *ops = &bridge->drv_ops.si;
	int rc = -1;
	int i;

	if (!entry->port)
		goto out;

	if (!entry->port_n)
		goto out;

	for (i = 0; i < entry->port_n; i++) {
		unsigned int port_id = entry->port[i].id;
		if (!(logical_port_valid(port_id) && logical_port_is_bridge(port_id) && (logical_port_bridge_id(port_id) == DEFAULT_BRIDGE_ID))) {
			os_log(LOG_ERR, "invalid logical_port(%u)\n", port_id);
			goto out;
		}
	}

	rtos_mutex_lock(&bridge->mutex, RTOS_WAIT_FOREVER);

	if (ops->si_update)
		rc = ops->si_update(ops, index, entry);

	rtos_mutex_unlock(&bridge->mutex);
out:
	return rc;
}

int stream_identity_delete(uint32_t index)
{
	struct net_bridge *bridge = bridge_get(DEFAULT_BRIDGE_ID);
	struct net_si_ops *ops = &bridge->drv_ops.si;
	int rc = -1;

	rtos_mutex_lock(&bridge->mutex, RTOS_WAIT_FOREVER);

	if (ops->si_delete)
		rc = ops->si_delete(ops, index);

	rtos_mutex_unlock(&bridge->mutex);

	return rc;
}

int stream_identity_read(uint32_t index, struct genavb_stream_identity *entry)
{
	struct net_bridge *bridge = bridge_get(DEFAULT_BRIDGE_ID);
	struct net_si_ops *ops = &bridge->drv_ops.si;
	int rc = -1;

	rtos_mutex_lock(&bridge->mutex, RTOS_WAIT_FOREVER);

	if (ops->si_read)
		rc = ops->si_read(ops, index, entry);

	rtos_mutex_unlock(&bridge->mutex);

	return rc;
}

#else
int stream_identity_update(uint32_t index, struct genavb_stream_identity *entry) { return -1; }
int stream_identity_delete(uint32_t index) { return -1; }
int stream_identity_read(uint32_t index, struct genavb_stream_identity *entry) { return -1; }
#endif
