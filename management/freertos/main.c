/*
* Copyright 2019-2021 NXP
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
 @brief Management freeRTOS specific code
 @details Setups freeRTOS thread for Management stack component. Implements Management main loop and event handling.
 */

#include <string.h>
#include <stdlib.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "common/ipc.h"
#include "common/log.h"

#include "os/config.h"
#include "os/sys_types.h"
#include "os/log.h"
#include "os/sys_types.h"
#include "os/ipc.h"
#include "os/timer.h"

#include "management/management_entry.h"

#define MANAGEMENT_TASK_NAME		"Management Stack"

const struct management_config management_default_config = {
	.log_level = management_CFG_LOG,
	.is_bridge = 0,
	.port_max = CFG_EP_DEFAULT_NUM_PORTS,
	.logical_port_list = CFG_EP_LOGICAL_PORT_LIST,
};

static void management_task(void *pvParameters)
{
	struct management_config *management_cfg = pvParameters;
	void *management;
	QueueHandle_t event_queue_h;

	os_log(LOG_INIT, "management task started\n");

	/*
	 * Main event queue creation
	 */
	event_queue_h = xQueueCreate(MANAGEMENT_CFG_EVENT_QUEUE_LENGTH, sizeof(struct event));
	if (!event_queue_h) {
		os_log(LOG_ERR, "xQueueCreate() failed\n");
		goto err_queue_create;
	}

	management = management_init(management_cfg, (unsigned long)event_queue_h);
	if (!management)
		goto err_management_init;

	os_log(LOG_INIT, "started\n");

	/*
	 * Main event loop
	 */
	while (1) {
		struct event e;

		if (xQueueReceive(event_queue_h, &e, pdMS_TO_TICKS(10000)) != pdTRUE)
			continue;

		switch (e.type) {
		case EVENT_TYPE_TIMER:
			os_timer_process((struct os_timer *)e.data);
			break;

		case EVENT_TYPE_IPC:
			ipc_rx((struct ipc_rx *)e.data);
			break;

		default:
			os_log(LOG_ERR, "xQueueReceive(): invalid event type(%u)\n", e.type);
			break;
		}
	}

	management_exit(management);

err_management_init:
	vQueueDelete(event_queue_h);

err_queue_create:
	os_log(LOG_INIT, "management task exited\n");

	vTaskDelete(NULL);
}

__init TaskHandle_t management_task_init(struct management_config *management_cfg)
{
	TaskHandle_t task;
	BaseType_t rc;

	rc = xTaskCreate(management_task, MANAGEMENT_TASK_NAME, MANAGEMENT_CFG_STACK_DEPTH, management_cfg, MANAGEMENT_CFG_PRIORITY, &task);
	if (rc != pdPASS) {
		os_log(LOG_ERR, "xTaskCreate(%s) failed\n", MANAGEMENT_TASK_NAME);
		goto err;
	}

	os_log(LOG_INIT, "management main completed\n");

	return task;

err:
	return NULL;
}

__exit void management_task_exit(TaskHandle_t task)
{
}
