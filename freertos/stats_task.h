/*
* Copyright 2019-2020 NXP
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
