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
 @brief Media clock interface handling
 @details
*/
#ifndef _LINUX_OSAL_MEDIA_CLOCK_H_
#define _LINUX_OSAL_MEDIA_CLOCK_H_

#include "epoll.h"

struct os_media_clock_rec {
	int fd;
	unsigned int mmap_size;
	void *array_addr;
	unsigned int array_size;
};

struct os_media_clock_gen {
	int fd;
	volatile unsigned int *w_idx;		/**< pointer to write index */
	volatile unsigned int *count;		/**< pointer to time of last update in ns, increments in steps of 125us periods. */
	volatile unsigned int *ptp;		/**< pointer to 32-bit PTP time of last update in ns, according to ENET HW counter */

	unsigned int mmap_size;
	void *array_addr;
	unsigned int array_size;
	unsigned int ts_freq_p;
	unsigned int ts_freq_q;
	unsigned int timer_period;
};

#endif /* _LINUX_OSAL_MEDIA_CLOCK_H_ */
