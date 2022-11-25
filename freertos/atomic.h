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
