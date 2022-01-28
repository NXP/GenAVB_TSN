/*
* Copyright 2017, 2020 NXP
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
 @brief FreeRTOS specific timer service implementation
 @details
*/

#ifndef _FREERTOS_OSAL_TIMER_H_
#define _FREERTOS_OSAL_TIMER_H_

#include "osal/sys_types.h"

struct os_timer {
	int clk_id;

	void *handle;
	void *q_handle;

	void (*func)(struct os_timer *t, int count);
	int (*start)(struct os_timer *t, uint64_t value, uint64_t interval_p, uint64_t interval_q, unsigned int flags);
	void (*stop)(struct os_timer *t);
	void (*destroy)(struct os_timer *t);
};

#endif /* _FREERTOS_OSAL_TIMER_H_ */
