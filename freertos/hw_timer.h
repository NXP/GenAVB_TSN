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
#ifndef _HW_TIMER_H_
#define _HW_TIMER_H_

#include "FreeRTOS.h"
#include "semphr.h"

#include "slist.h"
#include "net_port.h"
#include "hw_clock.h"
#include "common/log.h"
#include "common/stats.h"

struct hw_avb_timer_dev;

struct hw_avb_timer_event {
	unsigned int ticks;
};

struct hw_avb_timer {
	unsigned long period; // in us
	unsigned users;
	struct hw_avb_timer_dev *dev;
	SemaphoreHandle_t lock;
	StaticSemaphore_t lock_buffer;

	struct stats runtime_stats;
	struct stats delay_stats;
};

#define HW_AVB_TIMER_MIN_DELAY_US	10 /* Minimum delay between compare value and now (in microseconds) */

#define HW_AVB_TIMER_PERIOD_US	125
#define HW_AVB_TIMER_PERIOD_NS	(HW_AVB_TIMER_PERIOD_US * 1000)

struct hw_avb_timer_dev {
	unsigned int (*irq_ack)(struct hw_avb_timer_dev *, unsigned int *, unsigned int *);
	unsigned int (*elapsed)(struct hw_avb_timer_dev *, unsigned int);
	void (*start)(struct hw_avb_timer_dev *);
	void (*stop)(struct hw_avb_timer_dev *);
	void (*set_period)(struct hw_avb_timer_dev *, unsigned long period_us);
	unsigned int period; /* in cycles */
	unsigned long rate; /* in cycles per second */
	bool initialized;

	unsigned int min_delay_cycles;

	unsigned int recovery_errors;
};

int  hw_avb_timer_register_device(struct hw_avb_timer_dev *);
void hw_avb_timer_unregister_device(struct hw_avb_timer_dev *);
void hw_avb_timer_interrupt(struct hw_avb_timer_dev *);
int  hw_avb_timer_init(void);
void hw_avb_timer_exit(void);
void hw_avb_timer_start(void);

struct hw_timer {
	unsigned int id;
	unsigned int flags;
	struct slist_node node;

	int (*set_next_event)(struct hw_timer *dev, uint64_t cycles);
	int (*cancel_event)(struct hw_timer *dev);

	void (*func)(void *data);
	void *data;
};

struct hw_timer_drv {
	struct slist_head list_head[HW_CLOCK_MAX];
	SemaphoreHandle_t lock;
	StaticSemaphore_t lock_buffer;
};

#define HW_TIMER_F_FREE		(1 << 0)
#define HW_TIMER_F_PPS		(1 << 1)
#define HW_TIMER_F_PPS_ENABLED	(1 << 2)

#define hw_timer_is_pps(timer)	       ((timer)->flags & HW_TIMER_F_PPS)
#define hw_timer_is_pps_enabled(timer) ((timer)->flags & HW_TIMER_F_PPS_ENABLED)

static inline void hw_timer_pps_disable(struct hw_timer *t)
{
	t->flags &= ~HW_TIMER_F_PPS_ENABLED;
}

static inline int hw_timer_pps_enable(struct hw_timer *t)
{
	if (hw_timer_is_pps(t))
		t->flags |= HW_TIMER_F_PPS_ENABLED;
	else
		return -1;

	return 0;
}

int hw_timer_register(hw_clock_id_t id, struct hw_timer *timer, bool pps);
struct hw_timer *hw_timer_request(hw_clock_id_t id, bool pps, void (*func)(void *), void *data);
void hw_timer_free(struct hw_timer *timer);
int hw_timer_set_next_event(struct hw_timer *timer, uint64_t cycles);
int hw_timer_cancel(struct hw_timer *timer);
int hw_timer_init(void);
void hw_timer_exit(void);

#endif /* _HW_TIMER_H_ */

