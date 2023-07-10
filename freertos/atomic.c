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

#include "atomic.h"

#include "FreeRTOS.h"
#include "task.h"

void atomic_add(unsigned int val, unsigned int *addr)
{
	taskENTER_CRITICAL();

	*addr += val;

	taskEXIT_CRITICAL();
}

void atomic_sub(unsigned int val, unsigned int *addr)
{
	taskENTER_CRITICAL();

	*addr -= val;

	taskEXIT_CRITICAL();
}

void atomic_inc(unsigned int *addr)
{
	atomic_add(1, addr);
}

void atomic_dec(unsigned int *addr)
{
	atomic_sub(1, addr);
}

unsigned int atomic_xchg(unsigned int *addr, unsigned int new)
{
	unsigned int ret;

	taskENTER_CRITICAL();

	ret = *addr;
	*addr = new;

	taskEXIT_CRITICAL();

	return ret;
}
