/*
* Copyright 2018, 2020 NXP
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
 \file control.c
 \brief control public API for freertos
 \details
 \copyright Copyright 2018, 2020 NXP
*/

#include <string.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "common/ipc.h"

#include "api/control.h"

int avb_ipc_receive_sync(struct ipc_rx const *rx, unsigned int *msg_type, void *msg, unsigned int *msg_len, int timeout)
{
	QueueHandle_t event_handle = (QueueHandle_t)rx->priv;
	struct event e;
	int rc;

	if (event_handle) {
		if (xQueueReceive(event_handle, &e, pdMS_TO_TICKS(timeout)) != pdTRUE)
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

			vTaskDelay(pdMS_TO_TICKS(wait_interval));
			timeout -= wait_interval;
		}
	}

	return rc;
}

int genavb_control_set_callback(struct genavb_control_handle *handle, int (*callback)(void *), void *data)
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

int genavb_control_open(struct genavb_handle const *genavb, struct genavb_control_handle **control, genavb_control_id_t id)
{
	int rc;

	/*
	* allocate new stream
	*/

	if (id >= GENAVB_CTRL_ID_MAX) {
		rc = (-GENAVB_ERR_CTRL_INIT);
		goto err_invalid_id;
	}

	*control = pvPortMalloc(sizeof(struct genavb_control_handle));
	if (!*control) {
		rc = (-GENAVB_ERR_NO_MEMORY);
		goto err_alloc;
	}
	memset((*control), 0, sizeof(struct genavb_control_handle));

	(*control)->id = id;

	(*control)->event_queue_handle = xQueueCreateStatic(AVB_CONTROL_EVENT_QUEUE_LENGTH, sizeof(struct event), (*control)->event_queue_buffer, &(*control)->event_queue);
	if (!(*control)->event_queue_handle) {
		rc = (-GENAVB_ERR_NO_MEMORY);
		goto err_event_queue;
	}

	if (ipc_rx_init_no_notify(&(*control)->rx, ipc_id[id][CTRL_RX]) < 0) {
		rc = (-GENAVB_ERR_CTRL_INIT);
		goto err_control_rx_init;
	}

	if (ipc_tx_init(&(*control)->tx, ipc_id[id][CTRL_TX]) < 0) {
		rc = (-GENAVB_ERR_CTRL_INIT);
		goto err_control_tx_init;
	}

	if (ipc_id[id][CTRL_RX_SYNC] != IPC_ID_NONE) {
		if (ipc_rx_init(&(*control)->rx_sync, ipc_id[id][CTRL_RX_SYNC], NULL, (unsigned long)(*control)->event_queue_handle) < 0) {
			rc = (-GENAVB_ERR_CTRL_INIT);
			goto err_control_rx_sync_init;
		}
	}

	switch (id) {
	case GENAVB_CTRL_AVDECC_MEDIA_STACK:
	case GENAVB_CTRL_AVDECC_CONTROLLED:
		if (send_heartbeat(&(*control)->tx, &(*control)->rx, 0) < 0) {
			rc = -GENAVB_ERR_STACK_NOT_READY;
			goto err_tx_heartbeat;
		}

		break;

	case GENAVB_CTRL_AVDECC_CONTROLLER:
		if (send_heartbeat(&(*control)->tx, &(*control)->rx_sync, IPC_FLAGS_AVB_MSG_SYNC) < 0) {
			rc = -GENAVB_ERR_STACK_NOT_READY;
			goto err_tx_heartbeat;
		}

		break;

	case GENAVB_CTRL_MSRP:
	case GENAVB_CTRL_MVRP:
	case GENAVB_CTRL_GPTP:
	case GENAVB_CTRL_CLOCK_DOMAIN:
		if (ipc_tx_connect(&(*control)->tx, &(*control)->rx) < 0) {
			rc = (-GENAVB_ERR_CTRL_INIT);
			goto err_connect;
		}

		if (ipc_tx_connect(&(*control)->tx, &(*control)->rx_sync) < 0) {
			rc = (-GENAVB_ERR_CTRL_INIT);
			goto err_connect;
		}

		break;

	default:
		break;
	}

	return GENAVB_SUCCESS;

err_connect:
err_tx_heartbeat:
	if (ipc_id[id][CTRL_RX_SYNC] != IPC_ID_NONE)
		ipc_rx_exit(&(*control)->rx_sync);

err_control_rx_sync_init:
	ipc_tx_exit(&(*control)->tx);

err_control_tx_init:
	ipc_rx_exit(&(*control)->rx);

err_control_rx_init:
err_event_queue:
	vPortFree(*control);
	*control = NULL;

err_alloc:
err_invalid_id:
	return rc;
}
