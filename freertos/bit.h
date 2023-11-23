/*
* Copyright 2019-2020, 2023 NXP
*
* SPDX-License-Identifier: BSD-3-Clause
*/

/**
 @file
 @brief bit operations
*/

#ifndef _FREERTOS_BIT_H_
#define _FREERTOS_BIT_H_

#include "rtos_abstraction_layer.h"

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
