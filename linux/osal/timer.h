/*
* Copyright 2014 Freescale Semiconductor, Inc.
* Copyright 2020 NXP
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
 @brief Linux specific timer service implementation
 @details
*/

#ifndef _LINUX_OSAL_TIMER_H_
#define _LINUX_OSAL_TIMER_H_

#include "osal/sys_types.h"

#include "epoll.h"

struct os_timer {
	int epoll_fd;
	int fd;
	struct linux_epoll_data epoll_data;

	void (*func)(struct os_timer *t, int count);
	void (*process)(struct os_timer *t);
	int (*start)(struct os_timer *t, u64 value, u64 interval_p, u64 interval_q, unsigned int flags);
	void (*stop)(struct os_timer *t);
	void (*destroy)(struct os_timer *t);
};


#endif /* _LINUX_OSAL_TIMER_H_ */
