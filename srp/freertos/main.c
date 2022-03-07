/*
* Copyright 2018, 2020-2021 NXP
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
 @brief SRP freeRTOS specific code
 @details Setups freeRTOS thread for SRP stack component. Implements SRP main loop and event handling.
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
#include "os/net.h"
#include "os/ipc.h"
#include "os/timer.h"

#include "srp/srp_entry.h"
#include "srp/config.h"

#define SRP_TASK_NAME		"SRP Stack"

const struct srp_config srp_default_config = {
	.log_level = srp_CFG_LOG,

	.is_bridge = 0,

	.port_max = CFG_EP_DEFAULT_NUM_PORTS,

#ifdef CONFIG_MANAGEMENT
	.management_enabled = 1,
#else
	.management_enabled = 0,
#endif

	.msrp_cfg = {
		.is_bridge = 0,
		.flags = 0,
		.port_max = CFG_EP_DEFAULT_NUM_PORTS,
		.logical_port_list = CFG_EP_LOGICAL_PORT_LIST,
		.enabled = 1,
	},
	.mvrp_cfg = {
		.is_bridge = 0,
		.port_max = CFG_EP_DEFAULT_NUM_PORTS,
		.logical_port_list = CFG_EP_LOGICAL_PORT_LIST,
	},
};

__init static void process_section_msrp(struct msrp_config *cfg)
{
	cfg->flags = 0;
}

__init static void process_config(struct srp_config *cfg)
{
	process_section_msrp(&cfg->msrp_cfg);
}

static void srp_task(void *pvParameters)
{
	struct srp_config *srp_cfg = pvParameters;
	void *srp;
	QueueHandle_t event_queue_h;

	os_log(LOG_INIT, "srp task started\n");

	/*
	 * Main event queue creation
	 */
	event_queue_h = xQueueCreate(SRP_CFG_EVENT_QUEUE_LENGTH, sizeof(struct event));
	if (!event_queue_h) {
		os_log(LOG_ERR, "xQueueCreate() failed\n");
		goto err_queue_create;
	}

	process_config(srp_cfg);

	srp = srp_init(srp_cfg, (unsigned long)event_queue_h);
	if (!srp)
		goto err_srp_init;

	os_log(LOG_INIT, "started\n");

	/*
	 * Main event loop
	 */
	while (1) {
		struct event e;

		if (xQueueReceive(event_queue_h, &e, pdMS_TO_TICKS(10000)) != pdTRUE)
			continue;

		switch (e.type) {
		case EVENT_TYPE_NET_RX:
			net_rx((struct net_rx *)e.data);
			break;

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

	srp_exit(srp);

err_srp_init:
	vQueueDelete(event_queue_h);

err_queue_create:
	os_log(LOG_INIT, "srp task exited\n");

	vTaskDelete(NULL);
}

__init TaskHandle_t srp_task_init(struct srp_config *srp_cfg)
{
	TaskHandle_t task;
	BaseType_t rc;

	rc = xTaskCreate(srp_task, SRP_TASK_NAME, SRP_CFG_STACK_DEPTH, srp_cfg, SRP_CFG_PRIORITY, &task);
	if (rc != pdPASS) {
		os_log(LOG_ERR, "xTaskCreate(%s) failed\n", SRP_TASK_NAME);
		goto err;
	}

	os_log(LOG_INIT, "srp main completed\n");

	return task;

err:
	return NULL;
}

__exit void srp_task_exit(TaskHandle_t task)
{
}
