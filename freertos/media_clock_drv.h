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
 @brief AVB media clock driver
*/

#ifndef _MCLOCK_DRV_H_
#define _MCLOCK_DRV_H_

#include "media_clock.h"
#include "media_clock_gen_ptp.h"
#include "mtimer.h"

struct mclock_drv {
	struct mclock_gen_ptp gen_ptp[MCLOCK_DOMAIN_PTP_RANGE];
	struct slist_head mclock_devices;
	struct mclock_timer isr[MCLOCK_TIMER_MAX];
	SemaphoreHandle_t mutex;
	StaticSemaphore_t mutex_buffer;
};

#endif /* _MCLOCK_DRV_H_ */
