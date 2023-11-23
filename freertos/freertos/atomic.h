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

typedef unsigned int rtos_atomic_t;

static inline void rtos_atomic_set(rtos_atomic_t *addr, unsigned int val)
{
	*(volatile rtos_atomic_t *)addr = val;
}

static inline unsigned int rtos_atomic_read(rtos_atomic_t *addr)
{
	return *(volatile rtos_atomic_t *)addr;
}

void rtos_atomic_add(unsigned int val, rtos_atomic_t *addr);
void rtos_atomic_sub(unsigned int val, rtos_atomic_t *addr);
void rtos_atomic_inc(rtos_atomic_t *addr);
void rtos_atomic_dec(rtos_atomic_t *addr);
unsigned int rtos_atomic_xchg(rtos_atomic_t *addr, unsigned int new);

#endif /* _FREERTOS_ATOMIC_H_ */
