/*
* Copyright 2023 NXP
*
* SPDX-License-Identifier: BSD-3-Clause
*/

/**
 @file
 @brief NXP HSR FreeRTOS specific code
 @details Setups FreeRTOS task for NXP HSR stack component. Implements main loop and event handling.
 */

#include <string.h>
#include <stdlib.h>

#include "rtos_abstraction_layer.h"

#include "common/ipc.h"
#include "common/log.h"

#include "os/sys_types.h"
#include "os/config.h"
#include "os/timer.h"
#include "os/log.h"
#include "os/net.h"

#include "hsr/hsr.h"

#define HSR_TASK_NAME		"HSR Stack"
#define HSR_TASK_SUCCESS	(1 << 0)
#define HSR_TASK_ERROR		(1 << 1)

struct hsr_ctx {
	EventGroupHandle_t event_group_handle;
	StaticEventGroup_t event_group;
	QueueHandle_t event_queue_h;
	TaskHandle_t task_h;
	struct hsr_config *hsr_cfg;
	void *hsr;
	bool started;
};

const struct hsr_config hsr_default_config = {
	.hsr_enabled = 0,
	.port_max = 7,
	.hsr_port[0] = {
		.logical_port = 0,
		.type = HSR_UNUSED_PORT,
	},
	.hsr_port[1] = {
		.logical_port = 1,
		.type = HSR_HOST_PORT,
	},
	.hsr_port[2] = {
		.logical_port = 2,
		.type = HSR_RING_PORT,
	},
	.hsr_port[3] = {
		.logical_port = 3,
		.type = HSR_RING_PORT,
	},
	.hsr_port[4] = {
		.logical_port = 4,
		.type = HSR_EXTERNAL_PORT,
	},
	.hsr_port[5] = {
		.logical_port = 5,
		.type = HSR_EXTERNAL_PORT,
	},
	.hsr_port[6] = {
		.logical_port = 6,
		.type = HSR_INTERNAL_PORT,
	},
};

static void hsr_task(void *pvParameters)
{
	struct hsr_ctx *ctx = pvParameters;
	QueueHandle_t event_queue_h;
	void *hsr;

	os_log(LOG_INIT, "HSR task started\n");

	event_queue_h = xQueueCreate(HSR_CFG_EVENT_QUEUE_LENGTH, sizeof(struct event));
	if (!event_queue_h) {
		os_log(LOG_ERR, "xQueueCreate() failed\n");
		goto err_queue_create;
	}

	ctx->event_queue_h = event_queue_h;

	/*
	* Intialize hsr stack and apply configuration
	*/
	if (ctx->hsr_cfg->hsr_enabled) {
		hsr = hsr_init(ctx->hsr_cfg, (unsigned long)event_queue_h);
		if (!hsr)
			goto err_hsr_init;
	} else {
		hsr = NULL;
	}

	ctx->hsr = hsr;

	xEventGroupSetBits(ctx->event_group_handle, HSR_TASK_SUCCESS);

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

		default:
			os_log(LOG_ERR, "xQueueReceivet(): invalid event type(%u)\n", e.type);
			break;
		}

	}

	/* Not reached */

err_hsr_init:
	vQueueDelete(event_queue_h);

err_queue_create:
	os_log(LOG_INIT, "hsr task exited\n");

	xEventGroupSetBits(ctx->event_group_handle, HSR_TASK_ERROR);

	vTaskDelete(NULL);
}

__init void *hsr_task_init(struct hsr_config *hsr_cfg)
{
	struct hsr_ctx *ctx;
	BaseType_t rc;
	EventBits_t bits;

	ctx = pvPortMalloc(sizeof(struct hsr_ctx));
	if (!ctx)
		goto err_ctx_alloc;

	memset(ctx, 0, sizeof(struct hsr_ctx));

	ctx->event_group_handle = xEventGroupCreateStatic(&ctx->event_group);
	if (!ctx->event_group_handle)
		goto err_event_group;

	ctx->hsr_cfg = hsr_cfg;

	rc = xTaskCreate(hsr_task, HSR_TASK_NAME, HSR_CFG_STACK_DEPTH, ctx, HSR_CFG_PRIORITY, &ctx->task_h);
	if (rc != pdPASS) {
		os_log(LOG_ERR, "xTaskCreate(%s) failed\n", HSR_TASK_NAME);
		goto err_task_create;
	}

	bits = xEventGroupWaitBits(ctx->event_group_handle, HSR_TASK_SUCCESS | HSR_TASK_ERROR, pdFALSE, pdFALSE, portMAX_DELAY);
	if (bits & HSR_TASK_ERROR)
		goto err_event_group;

	os_log(LOG_INIT, "hsr main completed\n");

	return ctx;

err_event_group:
err_task_create:
	vPortFree(ctx);

err_ctx_alloc:
	return NULL;
}

__exit void hsr_task_exit(void *handle)
{
	struct hsr_ctx *ctx = handle;

	vTaskDelete(ctx->task_h);

	hsr_exit(ctx->hsr);

	vQueueDelete(ctx->event_queue_h);

	vPortFree(ctx);
}
