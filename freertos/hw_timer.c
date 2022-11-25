/*
* Copyright 2018, 2020, 2022 NXP
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
 @brief AVB HW timer generic layer
 @details
*/

#include "hw_timer.h"
#include "net_port.h"
#include "net_tx.h"
#include "common/log.h"


static struct hw_timer_drv hw_timer_drv;
static struct hw_timer_drv *hw_timer_drv_h = NULL;

__init int hw_timer_init(void)
{
	int i;

	if (hw_timer_drv_h != NULL) {
		os_log(LOG_ERR, "a HW timer driver is already initialized\n");
		goto err;
	}

	hw_timer_drv_h  = &hw_timer_drv;

	hw_timer_drv_h->lock = xSemaphoreCreateMutexStatic(&hw_timer_drv_h->lock_buffer);
	if (!hw_timer_drv_h->lock) {
		os_log(LOG_ERR, "xSemaphoreCreateMutexStatic failed\n");
		goto err;
	}

	for (i = 0; i < HW_CLOCK_MAX; i++)
		slist_head_init(&hw_timer_drv_h->list_head[i]);

	return 0;

err:
	return -1;
}

__exit void hw_timer_exit(void)
{
	hw_timer_drv_h  = NULL;
}

int hw_timer_register(hw_clock_id_t id, struct hw_timer *timer, unsigned int flags)
{
	if (!timer || id >= HW_CLOCK_MAX)
		return -1;

	xSemaphoreTake(hw_timer_drv_h->lock, portMAX_DELAY);

	slist_add_head(&hw_timer_drv_h->list_head[id], &timer->node);

	timer->flags = HW_TIMER_F_FREE;

	timer->flags |= hw_timer_user_flags(flags);

	xSemaphoreGive(hw_timer_drv_h->lock);

	os_log(LOG_INIT, "hw_timer(%p) of hw_clock(%d) registered%s\n", timer, id, hw_timer_is_pps(timer) ? ", pps support" : "");

	return 0;
}

static struct hw_timer *__hw_timer_request(hw_clock_id_t id, unsigned int required_flags, unsigned int rejected_flags)
{
	struct slist_node *entry;
	struct slist_head *head;
	struct hw_timer *timer;

	head = &hw_timer_drv_h->list_head[id];

	slist_for_each(head, entry) {
		timer = container_of(entry, struct hw_timer, node);

		if (!(timer->flags & HW_TIMER_F_FREE))
			continue;

		if (!((timer->flags & required_flags) == required_flags))
			continue;

		if (timer->flags & rejected_flags)
			continue;

		goto exit;
	}

	timer = NULL;

exit:
	return timer;
}

struct hw_timer *hw_timer_request(hw_clock_id_t id, unsigned int flags, void (*func)(void *), void *data)
{
	struct hw_timer *timer;

	/* Sanitize flags */
	flags = hw_timer_user_flags(flags);

	xSemaphoreTake(hw_timer_drv_h->lock, portMAX_DELAY);

	timer = __hw_timer_request(id, flags, hw_timer_user_flags(~flags));

	/* If no more regular timer, try to get one with extra features */
	if (!timer && !flags)
		timer = __hw_timer_request(id, flags, 0);

	if (timer) {
		timer->flags &= ~HW_TIMER_F_FREE;
		timer->func = func;
		timer->data = data;
		os_log(LOG_INFO, "hw_timer(%p)%s\n", timer, hw_timer_is_pps(timer) ? " pps" : "");
	}

	xSemaphoreGive(hw_timer_drv_h->lock);

	return timer;
}

void hw_timer_free(struct hw_timer *timer)
{
	xSemaphoreTake(hw_timer_drv_h->lock, portMAX_DELAY);

	timer->flags |= HW_TIMER_F_FREE;
	timer->func = NULL;
	timer->data = NULL;

	xSemaphoreGive(hw_timer_drv_h->lock);
}

int hw_timer_cancel(struct hw_timer *timer)
{
	return timer->cancel_event(timer);
}
