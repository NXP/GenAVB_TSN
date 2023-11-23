/*
* Copyright 2022-2023 NXP
*
* SPDX-License-Identifier: BSD-3-Clause
*/

/**
 @file
 @brief NXP MAAP FreeRTOS specific code
 @details Setups FreeRTOS task for NXP MAAP stack component. Implements main loop and event handling.
 */

#include <string.h>
#include <stdlib.h>

#include "rtos_abstraction_layer.h"

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
#define MAAP_TASK_SUCCESS	(1 << 0)
#define MAAP_TASK_ERROR		(1 << 1)

struct maap_ctx {
	EventGroupHandle_t event_group_handle;
	StaticEventGroup_t event_group;
	struct maap_config *maap_cfg;
	QueueHandle_t event_queue_h;
	TaskHandle_t task_h;
	void *maap;
};

const struct maap_config maap_default_config = {
	.log_level = maap_CFG_LOG,

	.port_max = CFG_EP_DEFAULT_NUM_PORTS,
#ifdef CONFIG_MANAGEMENT
	.management_enabled = true,
#else
	.management_enabled = false,
#endif

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
	struct maap_ctx *ctx = pvParameters;
	struct maap_config *maap_cfg = ctx->maap_cfg;
	struct maap_ctx *maap;
	QueueHandle_t event_queue_h;

	os_log(LOG_INIT, "maap task init\n");

	event_queue_h = xQueueCreate(MAAP_CFG_EVENT_QUEUE_LENGTH, sizeof(struct event));
	if (!event_queue_h) {
		os_log(LOG_ERR, "xQueueCreate() failed\n");
		goto err_queue_create;
	}

	ctx->event_queue_h = event_queue_h;

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

	ctx->maap = maap;

	xEventGroupSetBits(ctx->event_group_handle, MAAP_TASK_SUCCESS);

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

	/* Not reached */

err_maap_init:
	vQueueDelete(event_queue_h);

err_queue_create:
	os_log(LOG_INIT, "maap task exited\n");

	xEventGroupSetBits(ctx->event_group_handle, MAAP_TASK_ERROR);

	vTaskDelete(NULL);
}

__init void *maap_task_init(struct maap_config *maap_cfg)
{
	struct maap_ctx *ctx;
	BaseType_t rc;
	EventBits_t bits;

	ctx = pvPortMalloc(sizeof(struct maap_ctx));
	if (!ctx)
		goto err_ctx_alloc;

	ctx->event_group_handle = xEventGroupCreateStatic(&ctx->event_group);
	if (!ctx->event_group_handle)
		goto err_event_group;

	ctx->maap_cfg = maap_cfg;

	rc = xTaskCreate(maap_task, MAAP_TASK_NAME, MAAP_CFG_STACK_DEPTH, ctx, MAAP_CFG_PRIORITY, &ctx->task_h);
	if (rc != pdPASS) {
		os_log(LOG_ERR, "xTaskCreate(%s) failed\n", MAAP_TASK_NAME);
		goto err_task_create;
	}

	bits = xEventGroupWaitBits(ctx->event_group_handle, MAAP_TASK_SUCCESS | MAAP_TASK_ERROR, pdFALSE, pdFALSE, portMAX_DELAY);
	if (bits & MAAP_TASK_ERROR)
		goto err_event_group;

	os_log(LOG_INIT, "maap main completed\n");

	return ctx;

err_event_group:
err_task_create:
	vPortFree(ctx);

err_ctx_alloc:
	return NULL;
}

__exit void maap_task_exit(void *handle)
{
	struct maap_ctx *ctx = handle;

	vTaskDelete(ctx->task_h);

	maap_exit(ctx->maap);

	vQueueDelete(ctx->event_queue_h);

	vPortFree(ctx);
}
