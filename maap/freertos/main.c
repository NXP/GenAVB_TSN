/*
* Copyright 2022 NXP
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
 @brief NXP MAAP FreeRTOS specific code
 @details Setups FreeRTOS task for NXP MAAP stack component. Implements main loop and event handling.
 */

#include <string.h>
#include <stdlib.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "common/ipc.h"
#include "common/log.h"
#include "common/types.h"

#include "os/config.h"
#include "os/sys_types.h"
#include "os/log.h"
#include "os/clock.h"
#include "os/sys_types.h"
#include "os/net.h"
#include "os/ipc.h"
#include "os/timer.h"

#include "maap/maap_entry.h"
#include "maap/config.h"

#define MAAP_TASK_NAME		"MAAP Stack"

const struct maap_config maap_default_config = {
	.log_level = maap_CFG_LOG,

	.port_max = CFG_EP_DEFAULT_NUM_PORTS,

	.logical_port_list = CFG_EP_LOGICAL_PORT_LIST,
};

/**
 *
 */
__init static void process_config(struct maap_config *maap_cfg)
{
}

/**
 *
 */
static void maap_task(void *pvParameters)
{
	struct maap_config *maap_cfg = pvParameters;
	struct maap_ctx *maap;
	QueueHandle_t event_queue_h;

	os_log(LOG_INIT, "maap task init\n");

	event_queue_h = xQueueCreate(MAAP_CFG_EVENT_QUEUE_LENGTH, sizeof(struct event));
	if (!event_queue_h) {
		os_log(LOG_ERR, "xQueueCreate() failed\n");
		goto err_queue_create;
	}
	/**
	 * MAAP Config
	 */
	process_config(maap_cfg);

	/**
	 * MAAP Init
	 */
	maap = maap_init(maap_cfg, (unsigned long)event_queue_h);
	if (!maap)
		goto err_maap_init;

	os_log(LOG_INIT, "started\n");

	/*
	* Main event loop
	*/
	while (1) {
		BaseType_t handle;
		struct event e;

		handle = xQueueReceive(event_queue_h, &e, pdMS_TO_TICKS(10000));
		if (!handle) {
			continue;
		}

		switch (e.type) {
		case EVENT_TYPE_NET_RX:
			net_rx((struct net_rx *)e.data);
			break;

		case EVENT_TYPE_TIMER:
			os_timer_process((struct os_timer *)e.data);
			break;

		case EVENT_TYPE_NET_TX_TS:
			net_tx_ts_process((struct net_tx *)e.data);
			break;

		case EVENT_TYPE_IPC:
			ipc_rx((struct ipc_rx *)e.data);
			break;

		default:
			os_log(LOG_ERR, "xQueueReceive(): invalid event type(%u)\n", e.type);
			break;
		}
	}

	maap_exit(maap);

err_maap_init:
	vQueueDelete(event_queue_h);

err_queue_create:
	os_log(LOG_INIT, "maap task exited\n");

	vTaskDelete(NULL);
}

__init TaskHandle_t maap_task_init(struct maap_config *maap_cfg)
{
	TaskHandle_t task;
	BaseType_t rc;

	rc = xTaskCreate(maap_task, MAAP_TASK_NAME, MAAP_CFG_STACK_DEPTH, maap_cfg, MAAP_CFG_PRIORITY, &task);
	if (rc != pdPASS) {
		os_log(LOG_ERR, "xTaskCreate(%s) failed\n", MAAP_TASK_NAME);
		goto err;
	}

	os_log(LOG_INIT, "maap main completed\n");

	return task;

err:
	return NULL;
}

__exit void maap_task_exit(TaskHandle_t task)
{
}
