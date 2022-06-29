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
 @brief Basic types OS abstraction
 @details
*/

#ifndef _COMMON_TYPES_H_
#define _COMMON_TYPES_H_

#include "genavb/types.h"
#include "genavb/net_types.h"
#include "os/sys_types.h"

#define offset_of(type, member) ((unsigned long)&(((type *)0)->member))
#define container_of(entry, type, member) ((type *)((unsigned char *)(entry) - offset_of(type, member)))

#define min(a,b)  ((a)<(b)?(a):(b))
#define max(a,b)  ((a)>(b)?(a):(b))

/* The functions below use 32bit accesses to possibly non 32bit aligned addresses,
 * this works for ARMv7 and above CPU's. For other architectures we may need
 * to re-define these functions.
 * Volatile is used when casting to u32 * to prevent compiler from "merging" loads
 * and stores (use ldrd/strd/ldm/stm instructions for possibly unaligned addresses).
 */
static inline u64 get_64(const void *a)
{
#if defined(__BIG_ENDIAN__)
	return (((u64)(*(volatile u32 *)a)) << 32) | ((u64)(*((volatile u32 *)a + 1)));
#elif defined(__LITTLE_ENDIAN__)
	return (((u64)(*((volatile u32 *)a + 1))) << 32) | ((u64)(*(volatile u32 *)a));
#else
	#error
#endif
}

static inline u64 get_48(const void *a)
{
#if defined(__BIG_ENDIAN__)
	return (((u64)(*(volatile u32 *)a)) << 16) | ((u64)(*((u16 *)a + 2)));
#elif defined(__LITTLE_ENDIAN__)
	return (((u64)(*((u16 *)a + 2))) << 32) | ((u64)(*(volatile u32 *)a));
#else
	#error
#endif
}

static inline u64 get_ntohll(const void *a)
{
	return ntohll(get_64(a));
}

static inline u64 get_htonll(const void *a)
{
	return htonll(get_64(a));
}

static inline int cmp_64(const void *a, const void *b)
{
	if ((*(volatile u32 *)a == *(volatile u32 *)b) && (*((volatile u32 *)a + 1) == *((volatile u32 *)b + 1)))
		return 1;
	else
		return 0;
}

static inline void copy_64(void *dst, void const *src)
{
	*(volatile u32 *)dst = *(volatile u32 *)src;
	*((volatile u32 *)dst + 1) = *(volatile u32 *)((volatile u32 *)src + 1);
}

#endif /* _COMMON_TYPES_H_ */
