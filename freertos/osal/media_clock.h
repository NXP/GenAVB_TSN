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
 @brief
 @details
*/

#ifndef _FREERTOS_OSAL_MEDIA_CLOCK_H
#define _FREERTOS_OSAL_MEDIA_CLOCK_H

struct os_media_clock_rec {
	unsigned long priv;
	void *array_addr;
	unsigned int array_size;
	struct mclock_dev *mclock_dev;
};

struct os_media_clock_gen {
	struct mclock_dev *mclock_dev;
	volatile unsigned int *w_idx;		/**< pointer to write index */
	volatile unsigned int *count;		/**< pointer to time of last update in ns, increments in steps of 125us periods. */
	volatile unsigned int *ptp;		/**< pointer to 32-bit PTP time of last update in ns, according to ENET HW counter */

	unsigned long priv;
	void *array_addr;
	unsigned int array_size;
	unsigned int ts_freq_p;
	unsigned int ts_freq_q;
	unsigned int timer_period;
};

#endif /* _FREERTOS_OSAL_MEDIA_CLOCK_H */
