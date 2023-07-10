/*
* Copyright 2018, 2020, 2022-2023 NXP
*
* SPDX-License-Identifier: BSD-3-Clause
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
#include "hw_clock.h"

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

/* Internal flags */
#define HW_TIMER_F_FREE		(1 << 0)
#define HW_TIMER_F_PPS_ENABLED	(1 << 1)
/* User flags */
#define HW_TIMER_F_PPS		(1 << 16)
#define HW_TIMER_F_RECOVERY	(1 << 17)

#define hw_timer_user_flags(flags)	((flags) & (HW_TIMER_F_PPS | HW_TIMER_F_RECOVERY))
#define hw_timer_is_pps(timer)		((timer)->flags & HW_TIMER_F_PPS)
#define hw_timer_is_pps_enabled(timer)	((timer)->flags & HW_TIMER_F_PPS_ENABLED)

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

static inline int hw_timer_set_next_event(struct hw_timer *timer, uint64_t cycles)
{
	return timer->set_next_event(timer, cycles);
}

int hw_timer_register(hw_clock_id_t id, struct hw_timer *timer, unsigned int flags);
struct hw_timer *hw_timer_request(hw_clock_id_t id, unsigned int flags, void (*func)(void *), void *data);
void hw_timer_free(struct hw_timer *timer);
int hw_timer_cancel(struct hw_timer *timer);
int hw_timer_init(void);
void hw_timer_exit(void);

#endif /* _HW_TIMER_H_ */
