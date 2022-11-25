/*
* Copyright 2014 Freescale Semiconductor, Inc.
* Copyright 2020-2021 NXP
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
 @brief Timer Service implementation
 @details
*/
#ifndef _COMMON_TIMER_H_
#define _COMMON_TIMER_H_

#include "common/list.h"
#include "os/timer.h"

#include "config.h"

/* software timers flags */
#define TIMER_TYPE_SYS		(1 << 0)
#define TIMER_STATE_CREATED	(1 << 1)
#define TIMER_STATE_STARTED	(1 << 2)

#define NS_PER_MS 	(1000*1000)
#define MS_PER_S	(1000)

struct timer_sys {
	struct list_head head;
	struct list_head *next_to_process;
	unsigned int ms;
	unsigned int flags;
	unsigned int users;
	struct timer_ctx *ctx;
	struct os_timer os_timer; /* OS dependant fields */
};


struct timer {
	struct list_head list;
	int ms;
	unsigned int flags;
	struct timer_sys *timer_sys;
	void (*func)(void *);
	void *data;
};

struct timer_ctx {
	unsigned short max_sys_timers;
	unsigned short num_soft_timers;
	unsigned short num_sys_timers;
	unsigned long priv;

	/* variable size array */
	struct timer_sys timer_sys_table[]; /* contains system timers only , either shared or exclusively used */
};

#define timer_create timer_init
int timer_init(struct timer_ctx *tctx, struct timer *t, unsigned int flags, unsigned int ms);
int timer_start(struct timer *t, unsigned int ms);
int timer_restart(struct timer *t, unsigned int ms);
void timer_stop(struct timer *t);
int timer_is_running(struct timer *t);
int timer_destroy(struct timer *t);
unsigned int timer_pool_size(unsigned int n);
int timer_pool_init(struct timer_ctx *tctx, unsigned int n, unsigned long priv);
void timer_pool_exit(struct timer_ctx *tctx);


#endif /* _COMMON_TIMER_H_ */
