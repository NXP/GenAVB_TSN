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
 \file timer.c
 \brief timer public API for freertos
 \details
 \copyright Copyright 2019-2020 NXP
*/

#include <string.h>

#include "api/timer.h"
#include "api/clock.h"
#include "genavb/error.h"
#include "common/types.h"

#include "FreeRTOS.h"

static void genavb_timer_callback(struct os_timer *t, int count)
{
	struct genavb_timer *timer = container_of(t, struct genavb_timer, os_t);

	if (timer->callback)
		timer->callback(timer->data, count);
}

int genavb_timer_create(struct genavb_timer **t, genavb_clock_id_t id, genavb_timer_f_t flags)
{
	os_clock_id_t os_id;
	int rc;
	unsigned int flags_os = 0;

	if (!t) {
		rc = -GENAVB_ERR_INVALID;
		goto out;
	}

	if (flags & GENAVB_TIMERF_PPS)
		flags_os |= OS_TIMER_FLAGS_PPS;

	if (id >= GENAVB_CLOCK_MAX) {
		rc = -GENAVB_ERR_CLOCK;
		goto out_set_null;
	}

	os_id = genavb_clock_to_os_clock(id);
	if (os_id >= OS_CLOCK_MAX) {
		rc = -GENAVB_ERR_CLOCK;
		goto out_set_null;
	}

	*t = pvPortMalloc(sizeof(struct genavb_timer));
	if (!*t) {
		rc = -GENAVB_ERR_NO_MEMORY;
		goto out_set_null;
	}

	memset(*t, 0, sizeof(struct genavb_timer));

	if (os_timer_create(&(*t)->os_t, os_id, flags_os, genavb_timer_callback, 0) < 0) {
		rc = -GENAVB_ERR_TIMER;
		goto out_free_timer;
	}

	return GENAVB_SUCCESS;

out_free_timer:
	vPortFree(*t);

out_set_null:
	*t = NULL;
out:
	return rc;
}

int genavb_timer_set_callback(struct genavb_timer *t, void (*callback)(void *, int),
			      void *data)
{
	if (!t)
		return GENAVB_ERR_INVALID;

	portENTER_CRITICAL();

	t->data = data;
	t->callback = callback;

	portEXIT_CRITICAL();

	return GENAVB_SUCCESS;
}

int genavb_timer_start(struct genavb_timer *t, uint64_t value, uint64_t interval,
							genavb_timer_f_t flags)
{
	unsigned int flags_os = 0;

	if (!t)
		return -GENAVB_ERR_INVALID;

	if (flags & GENAVB_TIMERF_ABS)
		flags_os |= OS_TIMER_FLAGS_ABSOLUTE;

	if (flags & GENAVB_TIMERF_PPS)
		flags_os |= OS_TIMER_FLAGS_PPS;

	if (os_timer_start(&t->os_t, value, interval, 1, flags_os) < 0)
		return -GENAVB_ERR_TIMER;

	return GENAVB_SUCCESS;
}

void genavb_timer_stop(struct genavb_timer *t)
{
	if (t)
		os_timer_stop(&t->os_t);
}

void genavb_timer_destroy(struct genavb_timer *t)
{
	if (t) {
		os_timer_destroy(&t->os_t);
		vPortFree(t);
	}
}

