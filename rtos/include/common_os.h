/*
 * Glue layer code between RTOS code and common code under common/os/
 * Copyright 2024-2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _COMMON_OS_H_
#define _COMMON_OS_H_

#include "rtos_abstraction_layer.h"

/* Define needed symbols/macro for media_clock_rec_pll_common.c */
#ifndef NSEC_PER_SEC
#define NSEC_PER_SEC    NSECS_PER_SEC
#endif

static inline uint32_t __common_os_div64_32(uint64_t *dividend, uint32_t divisor)
{
	uint32_t remainder = *dividend % divisor;
	*dividend = *dividend / divisor;

	return remainder;
}

/* do_div returns the remainder of integer division and modifies the
   64 bits dividend in place */
#define common_os_do_div(dividend, divisor) __common_os_div64_32(&(dividend), divisor)

static inline s64 common_os_div_s64(s64 dividend, s32 divisor)
{
	return dividend / divisor;
}

static inline u64 common_os_div_u64(u64 dividend, u32 divisor)
{
	return dividend / divisor;
}

static inline u64 common_os_div64_u64(u64 dividend, u64 divisor)
{
	return dividend / divisor;
}

static inline s64 common_os_div64_s64(s64 dividend, s64 divisor)
{
	return dividend / divisor;
}

/* Map common_os_atomic declarations to RTOS atomic API */
#define common_os_atomic_t      rtos_atomic_t
#define common_os_atomic_read   rtos_atomic_read
#define common_os_atomic_set    rtos_atomic_set
#define common_os_atomic_add    rtos_atomic_add
#define common_os_atomic_xchg   rtos_atomic_xchg

#define common_os_log_err(...)     os_log(LOG_ERR, __VA_ARGS__)
#define common_os_log_info(...)    os_log(LOG_INFO, __VA_ARGS__)

#define fallthrough             __attribute__((__fallthrough__))

#endif /* _COMMON_OS_H_ */
