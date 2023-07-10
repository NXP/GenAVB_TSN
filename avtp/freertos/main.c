/*
* Copyright 2018, 2020, 2023 NXP
*
* SPDX-License-Identifier: BSD-3-Clause
*/

/**
 @file
 @brief NXP AVTP FreeRTOS specific code
 @details Setups FreeRTOS task for NXP AVDECC stack component. Implements main loop and event handling.
 */

#include <string.h>
#include <stdlib.h>

#include "FreeRTOS.h"
#include "event_groups.h"
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

#include "avtp/avtp_entry.h"
#include "avtp/config.h"

#define AVTP_TASK_NAME		"AVTP Stack"
#define STATS_TASK_NAME		"Stats Task"
#define AVTP_TASK_SUCCESS	(1 << 0)
#define AVTP_TASK_ERROR		(1 << 1)
#define STATS_TASK_SUCCESS	(1 << 2)
#define STATS_TASK_ERROR	(1 << 3)

#define IPC_POOLING_PERIOD_NS	(10ULL * NSECS_PER_MS)
#define STATS_PERIOD_NS		(10ULL * NSECS_PER_SEC)

struct avtp_ctx {
	EventGroupHandle_t event_group_handle;
	StaticEventGroup_t event_group;
	struct avtp_config *avtp_cfg;
	QueueHandle_t event_queue_h;
	TaskHandle_t task_h;
	TaskHandle_t stats_task_h;
	QueueHandle_t stats_event_queue_h;
	struct ipc_rx stats_ipc_rx;
	void *avtp;
};

const struct avtp_config avtp_default_config = {
	.log_level = avtp_CFG_LOG,

	.port_max = CFG_EP_DEFAULT_NUM_PORTS,

	.logical_port_list = CFG_EP_LOGICAL_PORT_LIST,
};

/**
 *
 */
__init static void process_config(struct avtp_config *avtp_cfg)
{
}


/**
 *
 */
static void stats_task(void *pvParameters)
{
	struct avtp_ctx *ctx = pvParameters;
	QueueHandle_t event_queue_h;

	os_log(LOG_INIT, "stats task init\n");

	event_queue_h = xQueueCreate(STATS_CFG_EVENT_QUEUE_LENGTH, sizeof(struct event));
	if (!event_queue_h) {
		os_log(LOG_ERR, "xQueueCreate() failed\n");
		goto err_queue_create;
	}

	ctx->stats_event_queue_h = event_queue_h;

	if (ipc_rx_init(&ctx->stats_ipc_rx, IPC_AVTP_STATS, stats_ipc_rx, (unsigned long)event_queue_h) < 0)
		goto err_ipc_rx;

	xEventGroupSetBits(ctx->event_group_handle, STATS_TASK_SUCCESS);

	os_log(LOG_INIT, "started\n");

	while (1) {
		struct event e;

		if (xQueueReceive(event_queue_h, &e, pdMS_TO_TICKS(10000)) != pdTRUE)
			continue;

		switch (e.type) {
		case EVENT_TYPE_IPC:
			ipc_rx((struct ipc_rx *)e.data);
			break;
		default:
			os_log(LOG_ERR, "xQueueReceive(): invalid event type(%u)\n", e.type);
			break;
		}
	}

	/* Not reached */

err_ipc_rx:
	vQueueDelete(event_queue_h);

err_queue_create:

	os_log(LOG_INIT, "stats task exited\n");

	xEventGroupSetBits(ctx->event_group_handle, STATS_TASK_ERROR);

	vTaskDelete(NULL);
}


/**
 *
 */
static void avtp_task(void *pvParameters)
{
	struct avtp_ctx *ctx = pvParameters;
	struct avtp_config *avtp_cfg = ctx->avtp_cfg;
	struct avtp_ctx *avtp;
	QueueHandle_t event_queue_h;
	uint64_t current_time = 0, sched_time = 0, ipc_time = 0, stats_time = 0;
	unsigned int i, events_count;
	struct process_stats stats;
	EventBits_t bits;

	os_log(LOG_INIT, "avtp task init\n");

	event_queue_h = xQueueCreate(AVTP_CFG_EVENT_QUEUE_LENGTH, sizeof(struct event));
	if (!event_queue_h) {
		os_log(LOG_ERR, "xQueueCreate() failed\n");
		goto err_queue_create;
	}

	ctx->event_queue_h = event_queue_h;

	/**
	 * Create stats task
	 */
	if (xTaskCreate(stats_task, STATS_TASK_NAME, STATS_CFG_STACK_DEPTH, ctx, STATS_CFG_PRIORITY, &ctx->stats_task_h) != pdPASS) {
		os_log(LOG_ERR, "xTaskCreate(%s) failed\n", STATS_TASK_NAME);
		goto err_task_create;
	}

	bits = xEventGroupWaitBits(ctx->event_group_handle, STATS_TASK_SUCCESS | STATS_TASK_ERROR, pdFALSE, pdFALSE, portMAX_DELAY);
	if (bits & STATS_TASK_ERROR)
		goto err_task_create;

	/**
	 * AVTP Config
	 */
	process_config(avtp_cfg);

	for (i = 0; i < CFG_MAX_NUM_PORT; i++)
		avtp_cfg->clock_gptp_list[i] = logical_port_to_gptp_clock(avtp_cfg->logical_port_list[i], CFG_DEFAULT_GPTP_DOMAIN);

	/**
	 * AVTP Init
	 */
	avtp = avtp_init(avtp_cfg, (unsigned long)event_queue_h);
	if (!avtp)
		goto err_avtp_init;

	ctx->avtp = avtp;

	xEventGroupSetBits(ctx->event_group_handle, AVTP_TASK_SUCCESS);

	os_log(LOG_INIT, "started\n");

	stats_init(&stats.events, 31, NULL, NULL);
	stats_init(&stats.sched_intvl, 31, NULL, NULL);
	stats_init(&stats.processing_time, 31, NULL, NULL);

	/**
	 * Main loop
	 */
	while (1) {
		struct event e;
		BaseType_t rc;

		events_count = 0;

		rc = xQueueReceive(event_queue_h, &e, pdMS_TO_TICKS(2));

		if (os_clock_gettime64(OS_CLOCK_SYSTEM_MONOTONIC, &current_time) == 0) {
			stats_update(&stats.sched_intvl, current_time - sched_time);
			sched_time = current_time;
		}

		if (rc != pdTRUE)
			goto timeout;

		do {
			events_count++;

			switch (e.type) {
			case EVENT_TYPE_NET_RX:
				net_rx_multi((struct net_rx *)e.data);
				break;

			case EVENT_TYPE_TIMER:
				os_timer_process((struct os_timer *)e.data);
				break;

			case EVENT_TYPE_MEDIA:
				avtp_media_event(e.data);
				break;

			default:
				os_log(LOG_ERR, "xQueueReceive(): invalid event type(%u)\n", e.type);
				break;
			}

		} while (events_count < 8 && xQueueReceive(event_queue_h, &e, 0));

timeout:
		stats_update(&stats.events, events_count);

		if (os_clock_gettime64(OS_CLOCK_SYSTEM_MONOTONIC, &current_time) == 0) {
			if ((current_time - ipc_time) > IPC_POOLING_PERIOD_NS) {
				avtp_ipc_rx(avtp);
				avtp_stream_free(avtp, current_time);
				ipc_time = current_time;

				if ((current_time - stats_time) > STATS_PERIOD_NS) {
					avtp_stats_dump(avtp, &stats);
					stats_time = current_time;
				}
			}

			stats_update(&stats.processing_time, current_time - sched_time);
		}
	}

	/* Not reached */

err_avtp_init:
	vTaskDelete(ctx->stats_task_h);

	ipc_rx_exit(&ctx->stats_ipc_rx);

	vQueueDelete(ctx->stats_event_queue_h);

err_task_create:
	vQueueDelete(event_queue_h);

err_queue_create:

	os_log(LOG_INIT, "avtp task exited\n");

	xEventGroupSetBits(ctx->event_group_handle, AVTP_TASK_ERROR);

	vTaskDelete(NULL);
}

/**
 *
 */
__init void *avtp_task_init(struct avtp_config *avtp_cfg)
{
	struct avtp_ctx *ctx;
	BaseType_t rc;
	EventBits_t bits;

	ctx = pvPortMalloc(sizeof(struct avtp_ctx));
	if (!ctx)
		goto err_ctx_alloc;

	ctx->event_group_handle = xEventGroupCreateStatic(&ctx->event_group);
	if (!ctx->event_group_handle)
		goto err_event_group;

	ctx->avtp_cfg = avtp_cfg;

	rc = xTaskCreate(avtp_task, AVTP_TASK_NAME, AVTP_CFG_STACK_DEPTH, ctx, AVTP_CFG_PRIORITY, &ctx->task_h);
	if (rc != pdPASS) {
		os_log(LOG_ERR, "xTaskCreate(%s) failed\n", AVTP_TASK_NAME);
		goto err_task_create;
	}

	bits = xEventGroupWaitBits(ctx->event_group_handle, AVTP_TASK_SUCCESS | AVTP_TASK_ERROR, pdFALSE, pdFALSE, portMAX_DELAY);
	if (bits & AVTP_TASK_ERROR)
		goto err_event_group;

	os_log(LOG_INIT, "avtp main completed\n");

	return ctx;

err_event_group:
err_task_create:
	vPortFree(ctx);

err_ctx_alloc:
	return NULL;
}

__exit void avtp_task_exit(void *handle)
{
	struct avtp_ctx *ctx = handle;

	vTaskDelete(ctx->task_h);

	avtp_exit(ctx->avtp);

	vTaskDelete(ctx->stats_task_h);

	ipc_rx_exit(&ctx->stats_ipc_rx);

	vQueueDelete(ctx->event_queue_h);

	vQueueDelete(ctx->stats_event_queue_h);

	vPortFree(ctx);
}
