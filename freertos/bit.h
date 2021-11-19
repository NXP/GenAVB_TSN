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
 @brief bit operations
*/

#ifndef _FREERTOS_BIT_H_
#define _FREERTOS_BIT_H_

#include "FreeRTOS.h"
#include "task.h"

static inline void clear_bit(unsigned int nr, volatile uint32_t *addr)
{
	uint32_t *word = ((uint32_t *)addr + (nr >> 5));

	taskENTER_CRITICAL();

	*word &= ~(1 << (nr & 0x1f));

	taskEXIT_CRITICAL();
}

static inline void set_bit(unsigned int nr, volatile uint32_t *addr)
{
	uint32_t *word = ((uint32_t *)addr + (nr >> 5));

	taskENTER_CRITICAL();

	*word |= (1 << (nr & 0x1f));

	taskEXIT_CRITICAL();
}

static inline int test_bit(unsigned int nr, volatile uint32_t *addr)
{
	uint32_t *word = ((uint32_t *)addr + (nr >> 5));
	int ret;

	taskENTER_CRITICAL();

	ret = (*word & (1 << (nr & 0x1f)));

	taskEXIT_CRITICAL();

	return ret;
}

static inline int test_and_clear_bit(unsigned int nr, volatile uint32_t *addr)
{
	uint32_t *word = ((uint32_t *)addr + (nr >> 5));
	int ret;

	taskENTER_CRITICAL();

	ret = (*word & (1 << (nr & 0x1f)));
	*word &= ~(1 << (nr & 0x1f));

	taskEXIT_CRITICAL();

	return ret;
}

static inline int test_and_set_bit(unsigned int nr, volatile uint32_t *addr)
{
	uint32_t *word = ((uint32_t *)addr + (nr >> 5));
	int ret;

	taskENTER_CRITICAL();

	ret = (*word & (1 << (nr & 0x1f)));
	*word |= (1 << (nr & 0x1f));

	taskEXIT_CRITICAL();

	return ret;
}

#endif /* _FREERTOS_BIT_H_ */
