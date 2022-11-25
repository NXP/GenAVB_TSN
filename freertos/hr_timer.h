/*
* Copyright 2019-2020, 2022 NXP
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

void hr_timer_task_stats(void);
void hr_timer_stats(void);

int hr_timer_start(void *handle, u64 value, u64 interval_p, u64 interval_q,
		   unsigned int flags);
void hr_timer_stop(void *handle);
void *hr_timer_create(os_clock_id_t clk_id, unsigned int flags,
				 void (*func)(void *data, int count), void *data);
void hr_timer_destroy(void *handle);

void hr_timer_clock_discont(os_clock_id_t clk);

int hr_timer_init(void);
void hr_timer_exit(void);

#endif /* _FREERTOS_HR_TIMER_H_ */

