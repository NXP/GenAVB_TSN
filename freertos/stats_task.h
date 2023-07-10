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

#ifndef _FREERTOS_STATS_TASK_H_
#define _FREERTOS_STATS_TASK_H_

#include "FreeRTOS.h"
#include "task.h"

#define STATS_TASK_NAME			"genavb_stats"
#define STATS_TASK_STACK_DEPTH		256
#define STATS_TASK_PRIORITY		2
#define STATS_TASK_DELAY_MS		10000

struct stats_ctx {
	TaskHandle_t task_handle;
};

extern struct stats_ctx stats_ctx;

int stats_task_init(void);
void stats_task_exit(void);

#endif /* _FREERTOS_STATS_TASK_H_ */
