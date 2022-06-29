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

void atomic_set(unsigned int *addr, unsigned int val)
{
	*(volatile unsigned int *)addr = val;
}

unsigned int atomic_read(unsigned int *addr)
{
	return *(volatile unsigned int *)addr;
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

