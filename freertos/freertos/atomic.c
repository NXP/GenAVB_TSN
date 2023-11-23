/*
* Copyright 2018, 2020, 2023 NXP
*
* SPDX-License-Identifier: BSD-3-Clause
*/

/**
 @file
 @brief atomic acces interface
 @details
*/

#include "rtos_abstraction_layer.h"

void rtos_atomic_add(unsigned int val, rtos_atomic_t *addr)
{
	taskENTER_CRITICAL();

	*addr += val;

	taskEXIT_CRITICAL();
}

void rtos_atomic_sub(unsigned int val, rtos_atomic_t *addr)
{
	taskENTER_CRITICAL();

	*addr -= val;

	taskEXIT_CRITICAL();
}

void rtos_atomic_inc(rtos_atomic_t *addr)
{
	rtos_atomic_add(1, addr);
}

void rtos_atomic_dec(rtos_atomic_t *addr)
{
	rtos_atomic_sub(1, addr);
}

unsigned int rtos_atomic_xchg(rtos_atomic_t *addr, unsigned int new)
{
	unsigned int ret;

	taskENTER_CRITICAL();

	ret = *addr;
	*addr = new;

	taskEXIT_CRITICAL();

	return ret;
}
