/*
 * Copyright 2018-2021, 2023-2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 \file control.c
 \brief control public API for rtos
 \details
*/

#include <string.h>

#include "rtos_abstraction_layer.h"

#include "common/ipc.h"

#include "api/control.h"

int avb_ipc_receive_sync(struct ipc_rx const *rx, unsigned int *msg_type, void *msg, unsigned int *msg_len, int timeout)
{
	rtos_mqueue_t *event_handle = (rtos_mqueue_t *)rx->priv;
	struct event e;
	int rc;

	if (event_handle) {
		if (rtos_mqueue_receive(event_handle, &e, RTOS_MS_TO_TICKS(timeout)) < 0)
			rc = -GENAVB_ERR_CTRL_TIMEOUT;
		else
			rc = avb_ipc_receive(rx, msg_type, msg, msg_len);
	} else {
		int wait_interval = 10;

		if (timeout >= 1000)
			wait_interval = 100;

		while ((rc = avb_ipc_receive(rx, msg_type, msg, msg_len)) == -GENAVB_ERR_CTRL_RX) {

			if (timeout <= 0) {
				rc = -GENAVB_ERR_CTRL_TIMEOUT;
				break;
			}

			if (wait_interval > timeout)
				wait_interval = timeout;

			rtos_sleep(RTOS_MS_TO_TICKS(wait_interval));
			timeout -= wait_interval;
		}
	}

	return rc;
}

int genavb_control_set_callback(struct genavb_control_handle *handle, int (*callback)(void *data), void *data)
{
	int rc;

	if (ipc_rx_set_callback(&handle->rx, callback, data) < 0) {
		rc = -GENAVB_ERR_CTRL_INIT;
		goto err;
	}

	return GENAVB_SUCCESS;

err:
	return rc;
}

int genavb_control_enable_callback(struct genavb_control_handle *handle)
{
	int rc;

	if (ipc_rx_enable_callback(&handle->rx) < 0) {
		rc = -GENAVB_ERR_CTRL_INIT;
		goto err;
	}

	return GENAVB_SUCCESS;

err:
	return rc;
}

/* Platform-specific initialization */
int control_init(struct genavb_control_handle *control, genavb_control_id_t id)
{
	if (rtos_mqueue_init(&control->event_queue, AVB_CONTROL_EVENT_QUEUE_LENGTH, 
			     sizeof(struct event), control->event_queue_buffer) < 0) {
		return -GENAVB_ERR_NO_MEMORY;
	}

	return GENAVB_SUCCESS;
}

void *control_priv_data(struct genavb_control_handle *control)
{
	return &(control)->event_queue;
}
