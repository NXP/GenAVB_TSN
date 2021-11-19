/*
* Copyright 2018, 2020 NXP
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
#include "media_clock.h"

static struct hw_avb_timer gtimer = {0};
struct hw_avb_timer *gtimer_h = NULL;

void hw_avb_timer_interrupt(struct hw_avb_timer_dev *timer_dev)
{
	unsigned int ticks;
	unsigned int cycles, dcycles;
	BaseType_t wake = pdFALSE;

	if (!timer_dev->initialized)
		return;

	ticks = timer_dev->irq_ack(timer_dev, &cycles, &dcycles);
	stats_update(&gtimer_h->delay_stats, dcycles);

	port_scheduler_notify(&wake);

	mclock_interrupt(ticks, &wake);

	dcycles = timer_dev->elapsed(timer_dev, cycles);

	stats_update(&gtimer_h->runtime_stats, dcycles);

	portYIELD_FROM_ISR(wake);
}

void hw_avb_timer_start(void)
{
	struct hw_avb_timer *timer;

	if (gtimer_h == NULL) {
		os_log(LOG_ERR, "HW timer not initialized\n");
		return;
	}

	timer = gtimer_h;

	if (!timer->dev) {
		os_log(LOG_ERR, "no HW timer device registered\n");
		return;
	}

	if (!timer->users)
		timer->dev->start(timer->dev);

	timer->users++;

	os_log(LOG_INIT, "hw_timer_start done\n");
}

void hw_avb_timer_stop(struct hw_avb_timer *timer)
{
	if (timer->users) {
		timer->users--;

		if (!timer->users)
			timer->dev->stop(timer->dev);
	}
}

int hw_avb_timer_register_device(struct hw_avb_timer_dev *dev)
{
	int rc = 0;
	struct hw_avb_timer *timer;

	if (gtimer_h == NULL) {
		os_log(LOG_ERR, "HW timer not initialized\n");
		rc = -1;
		goto err;
	}

	timer = gtimer_h;

	xSemaphoreTake(timer->lock, portMAX_DELAY);

	if (timer->dev) {
		os_log(LOG_ERR, "a HW timer device is already registered\n");
		rc = -1;
		goto register_err;
	}

	timer->dev = dev;

	os_log(LOG_INFO, "dev(%p) , ref clock %lu Hz, min delay cycles %u\n",
		dev, dev->rate, dev->min_delay_cycles);

	/* Start timer */
	dev->set_period(dev, timer->period);

	xSemaphoreGive(timer->lock);

	return 0;

register_err:
	xSemaphoreGive(timer->lock);
err:
	return rc;
}

void hw_avb_timer_unregister_device(struct hw_avb_timer_dev *dev)
{
	struct hw_avb_timer *timer;

	if (gtimer_h == NULL) {
		os_log(LOG_ERR, "HW timer not initialized\n");
		return;
	}

	timer = gtimer_h;

	xSemaphoreTake(timer->lock, portMAX_DELAY);

	if (timer->dev == dev) {
		hw_avb_timer_stop(timer);
		if (timer->users)
			os_log(LOG_INFO, "Warning, users are still pending while"
			"timer device (%p) is unregistered\n", dev);

		timer->dev = NULL;
	}

	xSemaphoreGive(timer->lock);
}

__init int hw_avb_timer_init(void)
{
	int rc = 0;

	if (gtimer_h != NULL) {
		os_log(LOG_ERR, "a HW timer device is already initialized\n");
		rc = -1;
		goto err;
	}

	gtimer_h = &gtimer;

	gtimer_h->lock = xSemaphoreCreateMutexStatic(&gtimer_h->lock_buffer);
	if (!gtimer_h->lock) {
		os_log(LOG_ERR, "xSemaphoreCreateMutexStatic failed\n");
		rc = -1;
		goto err_semaphore_create;
	}

	gtimer_h->period = HW_AVB_TIMER_PERIOD_US;
	gtimer_h->dev = NULL;

	stats_init(&gtimer_h->runtime_stats, 31, "hw_timer runtime stats", NULL);
	stats_init(&gtimer_h->delay_stats, 31, "hw_timer delay stats", NULL);

	os_log(LOG_INIT, "hw_timer_init done\n");

	return rc;

err_semaphore_create:
	gtimer_h = NULL;
err:
	return rc;
}

__exit void hw_avb_timer_exit(void)
{
	gtimer_h = NULL;
}

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

int hw_timer_register(hw_clock_id_t id, struct hw_timer *timer, bool pps)
{
	if (!timer || id >= HW_CLOCK_MAX)
		return -1;

	xSemaphoreTake(hw_timer_drv_h->lock, portMAX_DELAY);

	slist_add_head(&hw_timer_drv_h->list_head[id], &timer->node);

	timer->flags = HW_TIMER_F_FREE;

	if (pps)
		timer->flags |= HW_TIMER_F_PPS;

	xSemaphoreGive(hw_timer_drv_h->lock);

	os_log(LOG_INIT, "hw_timer(%p) of clock id: %d registered%s\n", timer, id, pps ? ", pps support" : "");

	return 0;
}

static struct hw_timer *__hw_timer_request(hw_clock_id_t id, bool pps)
{
	struct slist_node *entry;
	struct slist_head *head;
	struct hw_timer *timer;

	head = &hw_timer_drv_h->list_head[id];

	slist_for_each(head, entry) {
		timer = container_of(entry, struct hw_timer, node);

		if (!(timer->flags & HW_TIMER_F_FREE))
			continue;

		if (pps && !hw_timer_is_pps(timer))
			continue;

		if (!pps && hw_timer_is_pps(timer))
			continue;

		goto exit;
	}

	timer = NULL;

exit:
	return timer;
}

struct hw_timer *hw_timer_request(hw_clock_id_t id, bool pps, void (*func)(void *), void *data)
{
	struct hw_timer *timer;

	xSemaphoreTake(hw_timer_drv_h->lock, portMAX_DELAY);

	timer = __hw_timer_request(id, pps);

	/* If no more regular timer, try to get a pps one */
	if (!timer && !pps)
		timer = __hw_timer_request(id, true);

	if (timer) {
		timer->flags &= ~HW_TIMER_F_FREE;
		timer->func = func;
		timer->data = data;
		os_log(LOG_INFO, "hw_timer(%p)%s\n", timer, pps ? " pps" : "");
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

int hw_timer_set_next_event(struct hw_timer *timer, uint64_t cycles)
{
	return timer->set_next_event(timer, cycles);
}

int hw_timer_cancel(struct hw_timer *timer)
{
	return timer->cancel_event(timer);
}

