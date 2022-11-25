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
 @brief AVB media clock timer driver
*/

#include <string.h>

#include "mtimer.h"
#include "media_clock.h"

void mtimer_set_callback(struct mtimer_dev *dev, void (*cb)(void *), void *data)
{
	dev->cb = cb;
	dev->cb_data = data;
}

static void __mtimer_wake_up_init(struct mtimer_dev *dev, unsigned int now)
{
	rational_init(&dev->wake_up_next, now, 1);
	rational_add(&dev->wake_up_next, &dev->wake_up_next, &dev->wake_up_per);
}

int mtimer_start(struct mtimer_dev *dev, struct mtimer_start *start, unsigned int time_now,
			bool internal, bool is_media_clock_running)
{
	int rc = 0;

	taskENTER_CRITICAL();

	if (internal) {
		if (!(dev->flags & MTIMER_FLAGS_STARTED_INT)) {
			dev->flags |= MTIMER_FLAGS_STARTED_INT;
			__mtimer_wake_up_init(dev, time_now);
		}
	} else {
		rational_init(&dev->wake_up_per, (uint64_t)NSECS_PER_SEC * start->q, start->p);

		if (!(dev->flags & MTIMER_FLAGS_STARTED_EXT)) {
			rc = 1;
			dev->flags |= MTIMER_FLAGS_STARTED_EXT;

			/* If media clock is already running, set the internal start flag and wake up the timer
			 * Otherwise, wait for the internal media clock start to do that */
			if (is_media_clock_running) {
				dev->flags |= MTIMER_FLAGS_STARTED_INT;
				__mtimer_wake_up_init(dev, time_now);
			}
		}
	}

	taskEXIT_CRITICAL();

	return rc;
}

int mtimer_stop(struct mtimer_dev *dev, bool internal)
{
	int rc = 0;

	taskENTER_CRITICAL();

	if (internal) {
		dev->flags &= ~MTIMER_FLAGS_STARTED_INT;
	} else {
		if (dev->flags & MTIMER_FLAGS_STARTED_EXT) {
			rc = 1;

			dev->flags &= ~(MTIMER_FLAGS_STARTED_INT | MTIMER_FLAGS_STARTED_EXT);
		}
	}

	taskEXIT_CRITICAL();

	return rc;
}

void mtimer_wake_up_init(struct mtimer_dev *dev, unsigned int now)
{
	taskENTER_CRITICAL();

	__mtimer_wake_up_init(dev, now);

	taskEXIT_CRITICAL();
}

void mtimer_wake_up_init_now(struct mtimer_dev *dev, unsigned int now)
{
	taskENTER_CRITICAL();

	rational_init(&dev->wake_up_next, now, 1);

	taskEXIT_CRITICAL();
}

void mtimer_wake_up(struct mtimer_dev *dev, unsigned int now)
{
	taskENTER_CRITICAL();

	if (!(dev->flags & MTIMER_FLAGS_STARTED_INT))
		goto out_unlock;

	if (rational_int_cmp(now, &dev->wake_up_next) >= 0) {

		/* Check if there was a change in period, if so, reset the fractional part */
		if (dev->wake_up_next.q != dev->wake_up_per.q) {
			dev->wake_up_next.q = dev->wake_up_per.q;
			dev->wake_up_next.p = 0;
		}

		rational_add(&dev->wake_up_next, &dev->wake_up_next, &dev->wake_up_per);

		taskEXIT_CRITICAL();

		if (dev->cb)
			dev->cb(dev->cb_data);

		return;
	}

out_unlock:
	taskEXIT_CRITICAL();
}

struct mtimer_dev *mtimer_open(mclock_t type, unsigned int domain)
{
	struct mtimer_dev *dev;

	dev = pvPortMalloc(sizeof(*dev));
	if (!dev)
		goto err_alloc;

	memset(dev, 0, sizeof(*dev));

	dev->type = type;
	dev->domain = domain;

	return dev;

err_alloc:
	return NULL;
}

void mtimer_release(struct mtimer_dev *dev)
{
	mclock_timer_stop(dev);

	vPortFree(dev);
}

