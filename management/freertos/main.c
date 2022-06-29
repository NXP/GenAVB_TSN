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
#include "event_groups.h"
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
#include "management/config.h"

#define MANAGEMENT_TASK_NAME		"Management Stack"
#define MANAGEMENT_TASK_SUCCESS		(1 << 0)
#define MANAGEMENT_TASK_ERROR		(1 << 1)

struct management_ctx {
	EventGroupHandle_t event_group_handle;
	StaticEventGroup_t event_group;
	struct management_config *management_cfg;
	QueueHandle_t event_queue_h;
	TaskHandle_t task_h;
	void *management;
};

const struct management_config management_default_config = {
	.log_level = management_CFG_LOG,
	.is_bridge = 0,
	.port_max = CFG_EP_DEFAULT_NUM_PORTS,
	.logical_port_list = CFG_EP_LOGICAL_PORT_LIST,
};

static void management_task(void *pvParameters)
{
	struct management_ctx *ctx = pvParameters;
	struct management_config *management_cfg = ctx->management_cfg;
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

	ctx->event_queue_h = event_queue_h;

	management = management_init(management_cfg, (unsigned long)event_queue_h);
	if (!management)
		goto err_management_init;

	ctx->management = management;

	xEventGroupSetBits(ctx->event_group_handle, MANAGEMENT_TASK_SUCCESS);

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

	/* Not reached */

err_queue_create:
	vQueueDelete(event_queue_h);

err_management_init:
	os_log(LOG_INIT, "management task exited\n");

	xEventGroupSetBits(ctx->event_group_handle, MANAGEMENT_TASK_ERROR);

	vTaskDelete(NULL);
}

__init void *management_task_init(struct management_config *management_cfg)
{
	struct management_ctx *ctx;
	BaseType_t rc;
	EventBits_t bits;

	ctx = pvPortMalloc(sizeof(struct management_ctx));
	if (!ctx)
		goto err_ctx_alloc;

	ctx->event_group_handle = xEventGroupCreateStatic(&ctx->event_group);
	if (!ctx->event_group_handle)
		goto err_event_group;

	ctx->management_cfg = management_cfg;

	rc = xTaskCreate(management_task, MANAGEMENT_TASK_NAME, MANAGEMENT_CFG_STACK_DEPTH, ctx, MANAGEMENT_CFG_PRIORITY, &ctx->task_h);
	if (rc != pdPASS) {
		os_log(LOG_ERR, "xTaskCreate(%s) failed\n", MANAGEMENT_TASK_NAME);
		goto err_task_create;
	}

	bits = xEventGroupWaitBits(ctx->event_group_handle, MANAGEMENT_TASK_SUCCESS | MANAGEMENT_TASK_ERROR, pdFALSE, pdFALSE, portMAX_DELAY);
	if (bits & MANAGEMENT_TASK_ERROR)
		goto err_event_group;

	os_log(LOG_INIT, "management main completed\n");

	return ctx;

err_event_group:
err_task_create:
	vPortFree(ctx);

err_ctx_alloc:
	return NULL;
}

__exit void management_task_exit(void *handle)
{
	struct management_ctx *ctx = handle;

	vTaskDelete(ctx->task_h);

	management_exit(ctx->management);

	vQueueDelete(ctx->event_queue_h);

	vPortFree(ctx);
}
