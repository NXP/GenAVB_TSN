/*
* Copyright 2017-2020, 2022-2023 NXP
*
* SPDX-License-Identifier: BSD-3-Clause
*/

/**
 @file
 @brief FreeRTOS specific timer service implementation
 @details
*/

#include "os/timer.h"

#include "common/log.h"
#include "common/types.h"

#include "hr_timer.h"

#include "rtos_abstraction_layer.h"

#include "mtimer.h"

#define TIMER_NAME		"timer"
#define PERIODIC_TIMER_NAME	"periodic_timer"

static void timer_media_callback(void *data)
{
	struct os_timer *t = (struct os_timer *)data;
	struct event e;

	e.type = EVENT_TYPE_TIMER;
	e.data = t;

	if (xQueueSend(t->q_handle, &e, pdMS_TO_TICKS(0)) != pdPASS)
		t->event_err++;
}

static int timer_media_create(struct os_timer *t, os_clock_id_t id)
{
	mclock_t type;
	int domain;

	switch (id) {
	case OS_CLOCK_MEDIA_HW_0:
		type = GEN;
		domain = 0;
		break;
	case OS_CLOCK_MEDIA_HW_1:
		type = GEN;
		domain = 1;
		break;
	case OS_CLOCK_MEDIA_REC_0:
		type = REC;
		domain = 0;
		break;
	case OS_CLOCK_MEDIA_REC_1:
		type = REC;
		domain = 1;
		break;
	case OS_CLOCK_MEDIA_PTP_0:
		type = PTP;
		domain = 0;
		break;
	case OS_CLOCK_MEDIA_PTP_1:
		type = PTP;
		domain = 1;
		break;
	default:
		goto err;
	}

	t->handle = mtimer_open(type, domain);
	if (!t->handle) {
		os_log(LOG_ERR, "os_timer(%p) mtimer_open, clock_id %d\n", t, id);
		goto err;
	}

	mtimer_set_callback(t->handle, timer_media_callback, t);

	return 0;
err:
	return -1;
}

static int timer_media_start(struct os_timer *t, uint64_t value, uint64_t interval_p, uint64_t interval_q, unsigned int flags)
{
	struct mtimer_start start;

	if (value || flags)
		goto err;

	start.p = interval_p;
	start.q = interval_q;

	if (mclock_timer_start(t->handle, &start)) {
		os_log(LOG_ERR, "os_timer(%p), mclock_timer_start failed\n", t);
		goto err;
	}

	return 0;

err:
	return -1;
}

static void timer_media_stop(struct os_timer *t)
{
	mclock_timer_stop(t->handle);
}

static void timer_media_destroy(struct os_timer *t)
{
	mtimer_release(t->handle);
}

static void timer_system_callback(TimerHandle_t handle)
{
	struct os_timer *t = pvTimerGetTimerID(handle);

	if (!t->q_handle) {
		t->func(t, 1);
	} else {
		struct event e;

		e.type = EVENT_TYPE_TIMER;
		e.data = t;

		if (xQueueSend(t->q_handle, &e, pdMS_TO_TICKS(0)) != pdPASS)
			t->event_err++;
	}
}

static int timer_system_create(struct os_timer *t)
{
	os_log(LOG_INFO, "os_timer(%p), queue: %p\n", t, t->q_handle);

	return 0;
}

static int timer_system_start(struct os_timer *t, uint64_t value, uint64_t interval_p, uint64_t interval_q, unsigned int flags)
{
	TickType_t period;
	BaseType_t rc;

	os_log(LOG_DEBUG, "os_timer(%p), value: %llu, interval: %llu/%llu, flags: %x\n", t, value, interval_p, interval_q, flags);

	if (flags & (OS_TIMER_FLAGS_PPS | OS_TIMER_FLAGS_RECOVERY))
		goto err;

	if (interval_p) {
		/* Either periodic or single shot */
		if (value)
			goto err;

		/* At least ms resolution */
		if (interval_p < NSECS_PER_MS)
			goto err;

		/* Rational period not supported */
		if (interval_q != 1)
			goto err;

		period = pdMS_TO_TICKS(interval_p / NSECS_PER_MS);
	} else {
		/* Only relative time supported */
		if (flags)
			goto err;

		/* At least ms resolution */
		if (value < NSECS_PER_MS)
			goto err;

		period = pdMS_TO_TICKS(value / NSECS_PER_MS);
	}

	if (!t->handle) {
		if (interval_p)
			t->handle = xTimerCreate(PERIODIC_TIMER_NAME, period, pdTRUE, t, timer_system_callback);
		else
			t->handle = xTimerCreate(TIMER_NAME, period, pdFALSE, t, timer_system_callback);

		if (!t->handle)
			goto err;

		rc = xTimerStart(t->handle, pdMS_TO_TICKS(0));
		if (rc == pdFALSE)
			goto err;
	} else {
		rc = xTimerChangePeriod(t->handle, period, pdMS_TO_TICKS(0));
		if (rc == pdFAIL)
			goto err;
	}

	return 0;

err:
	return -1;
}

static void timer_system_stop(struct os_timer *t)
{
	xTimerStop(t->handle, pdMS_TO_TICKS(0));
}

static void timer_system_destroy(struct os_timer *t)
{
	os_log(LOG_INFO, "os_timer(%p)\n", t);

	if (t->handle) {
		xTimerDelete(t->handle, pdMS_TO_TICKS(0));
		t->handle = NULL;
	}
}

static void timer_hr_callback(void *data, int count)
{
	struct os_timer *t = data;

	if (!t->q_handle) {
		t->func(t, count);
	} else {
		BaseType_t xHigherPriorityTaskWoken = pdFALSE;
		struct event e;

		e.type = EVENT_TYPE_TIMER;
		e.data = t;

		if (xQueueSendFromISR(t->q_handle, &e, &xHigherPriorityTaskWoken) != pdPASS)
			t->event_err++;

		portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
	}
}

static int timer_hr_create(struct os_timer *t, os_clock_id_t clk_id, unsigned int flags)
{
	t->handle = hr_timer_create(clk_id, flags, timer_hr_callback, t);
	if (!t->handle)
		return -1;

	return 0;
}

static int timer_hr_start(struct os_timer *t, uint64_t value, uint64_t interval_p, uint64_t interval_q, unsigned int flags)
{
	return hr_timer_start(t->handle, value, interval_p, interval_q, flags);
}

static void timer_hr_stop(struct os_timer *t)
{
	hr_timer_stop(t->handle);
}

static void timer_hr_destroy(struct os_timer *t)
{
	if (t->handle) {
		hr_timer_destroy(t->handle);
		t->handle = NULL;
	}
}

int os_timer_create(struct os_timer *t, os_clock_id_t id, unsigned int flags, void (*func)(struct os_timer *t, int count), unsigned long priv)
{
	if (t->handle || !func)
		goto err;

	switch (id) {
	case OS_CLOCK_SYSTEM_MONOTONIC_COARSE:

		if (flags)
			goto err;

		if (timer_system_create(t) < 0)
			goto err;

		t->start = timer_system_start;
		t->stop = timer_system_stop;
		t->destroy = timer_system_destroy;
		break;

	case OS_CLOCK_MEDIA_HW_0:
	case OS_CLOCK_MEDIA_HW_1:
	case OS_CLOCK_MEDIA_REC_0:
	case OS_CLOCK_MEDIA_REC_1:
	case OS_CLOCK_MEDIA_PTP_0:
	case OS_CLOCK_MEDIA_PTP_1:

		if (flags)
			goto err;

		if (timer_media_create(t, id) < 0)
			goto err;

		t->start = timer_media_start;
		t->stop = timer_media_stop;
		t->destroy = timer_media_destroy;
		break;

	default:
		if (timer_hr_create(t, id, flags) < 0)
			goto err;

		t->start = timer_hr_start;
		t->stop = timer_hr_stop;
		t->destroy = timer_hr_destroy;
		break;
	}

	t->q_handle = (void *)priv;
	t->func = func;

	os_log(LOG_INFO, "os_timer(%p), queue: %p\n", t, t->q_handle);

	return 0;

err:
	return -1;
}

int os_timer_start(struct os_timer *t, uint64_t value, uint64_t interval_p, uint64_t interval_q, unsigned int flags)
{
	os_log(LOG_DEBUG, "os_timer(%p), value: %llu, interval: %llu/%llu, flags: %x\n", t, value, interval_p, interval_q, flags);

	return t->start(t, value, interval_p, interval_q, flags);
}

void os_timer_stop(struct os_timer *t)
{
	os_log(LOG_DEBUG, "os_timer(%p)\n", t);

	t->stop(t);
}

void os_timer_destroy(struct os_timer *t)
{
	os_log(LOG_INFO, "os_timer(%p)\n", t);

	t->destroy(t);
}

void os_timer_process(struct os_timer *t)
{
	os_log(LOG_DEBUG, "os_timer(%p)\n", t);

	t->func(t, 1);
}
