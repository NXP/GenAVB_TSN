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
 @brief FreeRTOS specific high-resolution timers
 @details
*/

#ifndef _FREERTOS_HR_TIMER_H_
#define _FREERTOS_HR_TIMER_H_

#include "os/clock.h"
#include "os/sys_types.h"
#include "slist.h"

#include "FreeRTOS.h"
#include "timers.h"
#include "event_groups.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#define HR_TIMER_EVENT_QUEUE_LENGTH	16
#define HR_TIMER_TASK_TIMEOUT		"hr_timer"

#define HR_TIMER_STACK_DEPTH		(configMINIMAL_STACK_SIZE + 150)
#define HR_TIMER_TASK_PRIORITY		(configMAX_PRIORITIES - 8)
#define HR_TIMER_TASK_NAME		"hr timer"

struct hr_timer_stats {
	unsigned int start;
	unsigned int stop;
	unsigned int events;
	unsigned int clock_discont;
	unsigned int err_start;
	unsigned int err_event;
	unsigned int err_event_isr;
	unsigned int err_clock;
};

struct hr_timer {
	os_clock_id_t clk_id;

	struct hw_timer *hw_timer;
	struct slist_node node;
	struct slist_node node_drv;

	void (*func)(void *data, int count);
	void *data;

	unsigned int irq_reload;
	unsigned int irq_pending;
	unsigned int enqueued;

	uint64_t period;
	uint64_t next_event;

	EventGroupHandle_t event_group_handle;
	StaticEventGroup_t event_group;

	struct hr_timer_stats stats;
};

struct hr_timer_task_stats {
	unsigned int enqueue;
	unsigned int cancel;
	unsigned int run;
	unsigned int err_sched;
	unsigned int err_timeout;
};

struct hr_timer_task_ctx {
	TaskHandle_t handle;

	StaticQueue_t queue;
	uint8_t queue_buffer[HR_TIMER_EVENT_QUEUE_LENGTH * sizeof(struct event)];
	QueueHandle_t queue_handle;

	TimerHandle_t timeout_handle;
	StaticTimer_t timeout_buffer;

	struct slist_head pending[OS_CLOCK_MAX];

	struct hr_timer_task_stats stats;
};

struct hr_timer_drv {
	struct hr_timer_task_ctx task_ctx;
	struct slist_head list;
	SemaphoreHandle_t lock;
	StaticSemaphore_t lock_buffer;
};

#define HR_TIMER_ERROR		(1 << 0)
#define HR_TIMER_SUCCESS	(1 << 1)

#define HR_TIMER_RATIO_WND_MS	 	200
#define HR_TIMER_RATIO_WND_NS	 	(HR_TIMER_RATIO_WND_MS * NSECS_PER_MS)
#define HR_TIMER_MIN_DELTA_NS	 	1500

int hr_timer_start(struct hr_timer *t, u64 value, u64 interval_p, u64 interval_q,
		   unsigned int flags);
void hr_timer_stop(struct hr_timer *t);
struct hr_timer *hr_timer_create(os_clock_id_t clk_id, unsigned int flags,
				 void (*func)(void *data, int count), void *data);
void hr_timer_destroy(struct hr_timer *t);

void hr_timer_clock_discont(os_clock_id_t clk);

int hr_timer_init(void);
void hr_timer_exit(void);

#endif /* _FREERTOS_HR_TIMER_H_ */

