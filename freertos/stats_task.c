/*
* Copyright 2019-2020, 2023 NXP
*
* SPDX-License-Identifier: BSD-3-Clause
*/

/**
 @file
 @brief FreeRTOS stats prinitng task
 @details
*/

#include "stats_task.h"

#include "common/log.h"
#include "debug_print.h"

struct stats_ctx stats_ctx;

static void stats_task(void *pvParameters)
{
	while (1) {
		vTaskDelay(pdMS_TO_TICKS(STATS_TASK_DELAY_MS));
		hr_timer_show_stats();
		mclock_show_stats();
		net_port_qos_show_stats();
		net_port_show_stats();
    }
}

__init int stats_task_init(void)
{
	int rc;

	rc = xTaskCreate(stats_task, STATS_TASK_NAME, STATS_TASK_STACK_DEPTH, &stats_ctx, STATS_TASK_PRIORITY, &stats_ctx.task_handle);
	if (rc != pdPASS) {
		os_log(LOG_ERR, "xTaskCreate(%s) failed\n", STATS_TASK_NAME);
		rc = -1;
		goto err;
	}

	return 0;

err:
	return rc;
}

__exit void stats_task_exit(void)
{
	vTaskDelete(stats_ctx.task_handle);
}
