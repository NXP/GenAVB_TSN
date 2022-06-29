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

#include "os/timer.h"
#include "os/clock.h"
#include "clock.h"

#include "atomic.h"
#include "hr_timer.h"
#include "hw_timer.h"

#include "task.h"

static struct hr_timer_drv hr_timer_drv;
struct hr_timer_drv *hr_timer_drv_h;

static int __hr_timer_program_next_event(struct hr_timer *t, uint64_t next_event, int isr)
{
	uint64_t cycles;

	if (isr) {
		if (clock_time_to_cycles_isr(t->clk_id, next_event, &cycles) < 0)
			goto err;
	} else {
		if (clock_time_to_cycles(t->clk_id, next_event, &cycles) < 0)
			goto err;
	}

	if (hw_timer_set_next_event(t->hw_timer, cycles) < 0)
		goto err;

	return 0;

err:
	if (isr)
		t->stats.err_event_isr++;
	else
		t->stats.err_event++;

	return -1;
}

static inline int hr_timer_program_next_event(struct hr_timer *t, uint64_t next_event)
{
	return __hr_timer_program_next_event(t, next_event, 0);
}

static inline int hr_timer_program_next_event_isr(struct hr_timer *t, uint64_t next_event)
{
	return __hr_timer_program_next_event(t, next_event, 1);
}

static int hr_timer_task_schedule(struct hr_timer_task_ctx *timer_task, uint64_t wake_time)
{
	TickType_t period;

	period = pdMS_TO_TICKS(wake_time / NSECS_PER_MS);
	if (!period)
		period = 1;

	if (xTimerChangePeriod(timer_task->timeout_handle, period, 0) == pdFALSE)
		goto err;

	if (xTimerStart(timer_task->timeout_handle, 0) == pdFAIL)
		goto err;

	os_log(LOG_DEBUG, "timer task timeout set to %d ms\n", wake_time / NSECS_PER_MS);

	return 0;

err:
	timer_task->stats.err_sched++;
	return -1;
}

static void hr_timer_task_run(struct hr_timer_task_ctx *timer_task)
{
	int i;
	uint64_t expires_next = UINT64_MAX;

	for (i = 0; i < OS_CLOCK_MAX; i++) {
		struct slist_node *entry, *next;
		struct slist_head *head;
		struct hr_timer *t;
		uint64_t now, next_event, expire;

		head = &timer_task->pending[i];

		if (slist_empty(head))
			continue;

		slist_for_each_safe(head, entry, next) {
			t = container_of(entry, struct hr_timer, node);

			if (os_clock_gettime64(i, &now) < 0) {
				t->stats.err_clock++;
				continue;
			}

			next_event = t->next_event;

			if (likely(now < next_event))
				expire = t->next_event - now;
			else
				expire = HR_TIMER_MIN_DELTA_NS;

			if (expire <= HR_TIMER_RATIO_WND_NS) {
				if (expire <= HR_TIMER_MIN_DELTA_NS)
					next_event = now + HR_TIMER_MIN_DELTA_NS;

				atomic_set(&t->irq_pending, 1);

				if (hr_timer_program_next_event(t, next_event) < 0)
					os_log(LOG_ERR, "timer(%p) error programming next event\n", t);

				os_log(LOG_DEBUG, "timer(%p) next event programmed: %llu, %llu, %llu\n",
									t, expire, next_event, now);

				if (!t->period || t->irq_reload) {
					atomic_set(&t->enqueued, 0);
					slist_del(head, &t->node);
					continue;
				} else {
					t->next_event += t->period;
					expire += t->period;
				}
			}

			if (expire < expires_next)
				expires_next = expire;
		}
	}

	if (expires_next != UINT64_MAX)
		hr_timer_task_schedule(timer_task, expires_next - HR_TIMER_RATIO_WND_NS / 8);

	timer_task->stats.run++;
}

static void hr_timer_task_enqueue(struct hr_timer_task_ctx *timer_task, struct hr_timer *t)
{
	os_log(LOG_DEBUG, "enqueue t(%p) clk_id: %d\n", t, t->clk_id);

	slist_add_head(&timer_task->pending[t->clk_id], &t->node);

	atomic_set(&t->enqueued, 1);

	hr_timer_task_run(timer_task);

	xEventGroupSetBits(t->event_group_handle, HR_TIMER_SUCCESS);

	timer_task->stats.enqueue++;

	return;
}

static void hr_timer_task_cancel(struct hr_timer_task_ctx *timer_task, struct hr_timer *t)
{
	struct slist_node *entry, *next;
	struct slist_head *head = &timer_task->pending[t->clk_id];
	struct hr_timer *timer_pending;

	os_log(LOG_DEBUG, "cancel t(%p) clk_id: %d\n", t, t->clk_id);

	if (atomic_read(&t->enqueued)) {
		slist_for_each_safe(head, entry, next) {
			timer_pending = container_of(entry, struct hr_timer, node);
			if (timer_pending == t) {
				slist_del(head, &timer_pending->node);
				break;
			}
		}
	}

	xEventGroupSetBits(t->event_group_handle, HR_TIMER_SUCCESS);

	timer_task->stats.cancel++;
}

static void hr_timer_task_timeout(TimerHandle_t t)
{
	struct hr_timer_task_ctx *timer_task = pvTimerGetTimerID(t);
	struct event e;

	e.type = EVENT_HR_TIMER_TIMEOUT;

	if (xQueueSend(timer_task->queue_handle, &e, 0) != pdPASS)
		timer_task->stats.err_timeout++;
}

static void hr_timer_task(void *pvParameters)
{
	struct hr_timer_task_ctx *timer_task = pvParameters;
	struct event e;

	for (;;) {
		if (xQueueReceive(timer_task->queue_handle, &e, portMAX_DELAY) != pdTRUE)
			continue;

		switch (e.type) {
		case EVENT_HR_TIMER_ENQUEUE:
			hr_timer_task_enqueue(timer_task, e.data);
			break;

		case EVENT_HR_TIMER_CANCEL:
			hr_timer_task_cancel(timer_task, e.data);
			break;

		case EVENT_HR_TIMER_TIMEOUT:
			hr_timer_task_run(timer_task);
			break;

		default:
			break;
		}
	}
}

__init int hr_timer_init(void)
{
	struct hr_timer_task_ctx *task_ctx;
	int rc, i;

	hr_timer_drv_h = &hr_timer_drv;
	task_ctx = &hr_timer_drv_h->task_ctx;

	slist_head_init(&hr_timer_drv_h->list);

	for (i = 0; i < OS_CLOCK_MAX; i++)
		slist_head_init(&task_ctx->pending[i]);

	hr_timer_drv_h->lock = xSemaphoreCreateMutexStatic(&hr_timer_drv_h->lock_buffer);
	if (!hr_timer_drv_h->lock) {
		os_log(LOG_ERR, "xSemaphoreCreateMutexStatic failed\n");
		rc = -1;
		goto err;
	}

	task_ctx->queue_handle = xQueueCreateStatic(HR_TIMER_EVENT_QUEUE_LENGTH,
						    sizeof(struct event),
						    task_ctx->queue_buffer,
						    &task_ctx->queue);
	if (!task_ctx->queue_handle) {
		os_log(LOG_ERR, "xQueueCreateStatic(hr timer) failed\n");
		goto err;
	}

	task_ctx->timeout_handle = xTimerCreateStatic(HR_TIMER_TASK_TIMEOUT,
						      pdMS_TO_TICKS(1),
						      pdFALSE, task_ctx,
						      hr_timer_task_timeout,
						      &task_ctx->timeout_buffer);
	if (!task_ctx->timeout_handle) {
		os_log(LOG_ERR, "xTimerCreateStatic(%s) failed\n", HR_TIMER_TASK_TIMEOUT);
		goto err;
	}

	rc = xTaskCreate(hr_timer_task, HR_TIMER_TASK_NAME,
			 HR_TIMER_STACK_DEPTH, task_ctx,
			 HR_TIMER_TASK_PRIORITY, &task_ctx->handle);
	if (rc != pdPASS) {
		os_log(LOG_ERR, "xTaskCreate(%s) failed\n", HR_TIMER_TASK_NAME);
		goto err;
	}

	return 0;

err:
	return -1;
}

__exit void hr_timer_exit(void)
{
	struct hr_timer_task_ctx *task_ctx = &hr_timer_drv_h->task_ctx;

	vTaskDelete(task_ctx->handle);

	if (xTimerDelete(task_ctx->timeout_handle, pdMS_TO_TICKS(10)) == pdFAIL) {
		os_log(LOG_ERR, "xTimerDelete failed\n");
		goto exit;
	}

	while (xTimerIsTimerActive(task_ctx->timeout_handle))
		vTaskDelay(pdMS_TO_TICKS(1));

exit:
	hr_timer_drv_h = NULL;
}

static int __hr_timer_send_event(struct hr_timer_task_ctx *timer_task,
					struct hr_timer *t, unsigned int type)
{
	struct event e;
	EventBits_t rc;

	e.type = type;
	e.data = t;

	if (xQueueSendToBack(timer_task->queue_handle, &e, 0) != pdTRUE)
		goto err;

	rc = xEventGroupWaitBits(t->event_group_handle,
				 HR_TIMER_ERROR | HR_TIMER_SUCCESS,
				 pdTRUE, pdFALSE, pdMS_TO_TICKS(2));

	if (!(rc & HR_TIMER_SUCCESS))
		goto err;

	return 0;

err:
	return -1;
}

static int hr_timer_enqueue(struct hr_timer_task_ctx *timer_task, struct hr_timer *t)
{
	return __hr_timer_send_event(timer_task, t, EVENT_HR_TIMER_ENQUEUE);
}

static int hr_timer_cancel(struct hr_timer_task_ctx *timer_task, struct hr_timer *t)
{
	return __hr_timer_send_event(timer_task, t, EVENT_HR_TIMER_CANCEL);
}

static void __hr_timer_stop(struct hr_timer *t)
{
	/*
	 * Remove timer from programming task.
	 */
	if (atomic_read(&t->enqueued))
		hr_timer_cancel(&hr_timer_drv_h->task_ctx, t);

	/*
	 * Make sure IRQ handler cannot call its callback
	 * and then disable IRQ in HW timer.
	 */
	if (atomic_read(&t->irq_pending)) {
		atomic_set(&t->irq_pending, 0);
		hw_timer_cancel(t->hw_timer);
	}
}

static void hr_timer_callback(void *data)
{
	struct hr_timer *t = data;
	int rc = 1;

	if (!atomic_read(&t->irq_pending))
		return;

	if (t->irq_reload) {
		t->next_event += t->period;
		if (hr_timer_program_next_event_isr(t, t->next_event) < 0)
			rc = -1;
	} else
		atomic_set(&t->irq_pending, 0);

	t->func(t->data, rc);

	t->stats.events++;
}

int hr_timer_start(struct hr_timer *t, u64 value, u64 interval_p, u64 interval_q, unsigned int flags)
{
	uint64_t now;

	if (!t)
		goto err_null;

	/* No support for rational period */
	if (interval_p && (interval_q != 1))
		goto err;

	if (os_clock_gettime64(t->clk_id, &now) < 0)
		goto err;

	/* For periodic timer, first expiration at the end of the first period */
	value += interval_p;

	if (flags & OS_TIMER_FLAGS_PPS) {
		if (hw_timer_pps_enable(t->hw_timer) < 0)
			goto err;
	} else {
		hw_timer_pps_disable(t->hw_timer);
	}

	/*
	 * An expire time already in the past is not valid, other cases
	 * which would eventually be in the past at the time of programming the
	 * timer would be set to expire immediately.
	 */
	if ((flags & OS_TIMER_FLAGS_ABSOLUTE) && (value < now))
		goto err;

	__hr_timer_stop(t);

	if (flags & OS_TIMER_FLAGS_ABSOLUTE)
		t->next_event = value;
	else
		t->next_event = value + now;

	if (interval_p) {
		t->period = interval_p;

		/*
		 * If the timer has a period smaller than the valid ratio window
		 * it can be re-armed automatically in the interrupt callback.
		 */
		if (interval_p < HR_TIMER_RATIO_WND_NS)
			t->irq_reload = 1;
		else
			t->irq_reload = 0;
	}
	else
		t->period = 0;

	if (hr_timer_enqueue(&hr_timer_drv_h->task_ctx, t) < 0)
		goto err;

	t->stats.start++;

	return 0;

err:
	t->stats.err_start++;

err_null:
	return -1;
}

void hr_timer_stop(struct hr_timer *t)
{
	if (!t)
		return;

	__hr_timer_stop(t);

	t->stats.stop++;
}

struct hr_timer *hr_timer_create(os_clock_id_t clk_id, unsigned int flags,
				 void (*func)(void *data, int count), void *data)
{
	struct hr_timer *t;
	hw_clock_id_t hw_id;
	bool pps = false;

	if (flags & OS_TIMER_FLAGS_PPS)
		pps = true;

	if (!func)
		goto err;

	hw_id = clock_to_hw_clock(clk_id);

	if (hw_id == HW_CLOCK_NONE)
		goto err;

	t = pvPortMalloc(sizeof(struct hr_timer));
	if (!t)
		goto err;

	memset(t, 0, sizeof(*t));

	t->clk_id = clk_id;
	t->func = func;
	t->data = data;

	t->event_group_handle = xEventGroupCreateStatic(&t->event_group);
	if (!t->event_group_handle)
		goto err_free_timer;

	t->hw_timer = hw_timer_request(hw_id, pps, hr_timer_callback, t);
	if (!t->hw_timer)
		goto err_free_timer;

	xSemaphoreTake(hr_timer_drv_h->lock, portMAX_DELAY);

	slist_add_head(&hr_timer_drv_h->list, &t->node_drv);

	xSemaphoreGive(hr_timer_drv_h->lock);

	return t;

err_free_timer:
	vPortFree(t);

err:
	return NULL;
}

void hr_timer_destroy(struct hr_timer *t)
{
	if (!t)
		return;

	xSemaphoreTake(hr_timer_drv_h->lock, portMAX_DELAY);

	slist_del(&hr_timer_drv_h->list, &t->node_drv);

	xSemaphoreGive(hr_timer_drv_h->lock);

	__hr_timer_stop(t);
	hw_timer_free(t->hw_timer);
	vPortFree(t);
}

void hr_timer_clock_discont(os_clock_id_t clk)
{
	struct slist_node *entry;
	struct hr_timer *t;

	xSemaphoreTake(hr_timer_drv_h->lock, portMAX_DELAY);

	slist_for_each(&hr_timer_drv_h->list, entry) {
		t = container_of(entry, struct hr_timer, node_drv);

		if (clk == t->clk_id) {
			__hr_timer_stop(t);
			t->func(t->data, -1);
			t->stats.clock_discont++;
		}
	}

	xSemaphoreGive(hr_timer_drv_h->lock);
}
