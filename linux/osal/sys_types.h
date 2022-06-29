/*
* Copyright 2015 Freescale Semiconductor, Inc.
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
 @brief Linux specific type definitions
 @details
*/

#ifndef _LINUX_OSAL_SYS_TYPES_H_
#define _LINUX_OSAL_SYS_TYPES_H_

#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>
#include <unistd.h>     /* for size_t */

#define __init
#define __exit

typedef uint64_t	u64;
typedef uint32_t	u32;
typedef uint16_t	u16;
typedef uint8_t		u8;
typedef int64_t		s64;
typedef int32_t		s32;
typedef int16_t		s16;
typedef int8_t		s8;

#define TRUE	1
#define FALSE	0

/* FIXME this will break for non gcc compilers */
#define unlikely(x)	__builtin_expect(x, 0)
#define likely(x)	__builtin_expect(x, 1)

#endif /* _LINUX_OSAL_SYS_TYPES_H_ */
