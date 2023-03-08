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

#ifndef _FREERTOS_ATOMIC_H_
#define _FREERTOS_ATOMIC_H_


static inline void atomic_set(unsigned int *addr, unsigned int val)
{
	*(volatile unsigned int *)addr = val;
}

static inline unsigned int atomic_read(unsigned int *addr)
{
	return *(volatile unsigned int *)addr;
}

void atomic_add(unsigned int val, unsigned int *addr);
void atomic_sub(unsigned int val, unsigned int *addr);
void atomic_inc(unsigned int *addr);
void atomic_dec(unsigned int *addr);
unsigned int atomic_xchg(unsigned int *addr, unsigned int new);

#endif /* _FREERTOS_ATOMIC_H_ */
