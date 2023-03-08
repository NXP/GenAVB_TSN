/*
* Copyright 2018, 2020-2021, 2023 NXP
*
* SPDX-License-Identifier: BSD-3-Clause
*/

/**
 @file
 @brief SRP freeRTOS specific code
 @details Setups freeRTOS thread for SRP stack component. Implements SRP main loop and event handling.
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
#include "os/net.h"
#include "os/ipc.h"
#include "os/timer.h"

#include "srp/srp_entry.h"
#include "srp/config.h"

#define SRP_TASK_NAME		"SRP Stack"
#define SRP_TASK_SUCCESS	(1 << 0)
#define SRP_TASK_ERROR		(1 << 1)

struct srp_ctx {
	EventGroupHandle_t event_group_handle;
	StaticEventGroup_t event_group;
	struct srp_config *srp_cfg;
	QueueHandle_t event_queue_h;
	TaskHandle_t task_h;
	void *srp;
	bool started;
};

const struct srp_config srp_default_config = {
	.log_level = srp_CFG_LOG,

	.is_bridge = 0,

	.port_max = CFG_EP_DEFAULT_NUM_PORTS,

	.logical_port_list = CFG_EP_LOGICAL_PORT_LIST,

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
	struct srp_ctx *ctx = pvParameters;
	struct srp_config *srp_cfg = ctx->srp_cfg;
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

	ctx->event_queue_h = event_queue_h;

	process_config(srp_cfg);

	srp = srp_init(srp_cfg, (unsigned long)event_queue_h);
	if (!srp)
		goto err_srp_init;

	ctx->srp = srp;

	xEventGroupSetBits(ctx->event_group_handle, SRP_TASK_SUCCESS);

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

	/* Not reached */

err_srp_init:
	vQueueDelete(event_queue_h);

err_queue_create:
	os_log(LOG_INIT, "srp task exited\n");

	xEventGroupSetBits(ctx->event_group_handle, SRP_TASK_ERROR);

	vTaskDelete(NULL);
}

__init void *srp_task_init(struct srp_config *srp_cfg)
{
	struct srp_ctx *ctx;
	BaseType_t rc;
	EventBits_t bits;

	ctx = pvPortMalloc(sizeof(struct srp_ctx));
	if (!ctx)
		goto err_ctx_alloc;

	ctx->event_group_handle = xEventGroupCreateStatic(&ctx->event_group);
	if (!ctx->event_group_handle)
		goto err_event_group;

	ctx->srp_cfg = srp_cfg;

	rc = xTaskCreate(srp_task, SRP_TASK_NAME, SRP_CFG_STACK_DEPTH, ctx, SRP_CFG_PRIORITY, &ctx->task_h);
	if (rc != pdPASS) {
		os_log(LOG_ERR, "xTaskCreate(%s) failed\n", SRP_TASK_NAME);
		goto err_task_create;
	}

	bits = xEventGroupWaitBits(ctx->event_group_handle, SRP_TASK_SUCCESS | SRP_TASK_ERROR, pdFALSE, pdFALSE, portMAX_DELAY);
	if (bits & SRP_TASK_ERROR)
		goto err_event_group;

	os_log(LOG_INIT, "srp main completed\n");

	return ctx;

err_event_group:
err_task_create:
	vPortFree(ctx);

err_ctx_alloc:
	return NULL;
}

__exit void srp_task_exit(void *handle)
{
	struct srp_ctx *ctx = handle;

	vTaskDelete(ctx->task_h);

	srp_exit(ctx->srp);

	vQueueDelete(ctx->event_queue_h);

	vPortFree(ctx);
}
