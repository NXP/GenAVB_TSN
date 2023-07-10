/*
* Copyright 2017, 2020, 2023 NXP
*
* SPDX-License-Identifier: BSD-3-Clause
*/

/**
 @file
 @brief FreeRTOS specific type definitions
 @details
*/

#ifndef _FREERTOS_OSAL_SYS_TYPES_H_
#define _FREERTOS_OSAL_SYS_TYPES_H_

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>

#define __init	__attribute__((section (".text.init")))
#define __exit	__attribute__((section (".text.exit")))

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

#define BITS_PER_BYTE	8
#define BITS_PER_LONG (sizeof(long) * BITS_PER_BYTE)

/* FIXME this will break for non gcc compilers */
#define unlikely(x)	__builtin_expect(x, 0)
#define likely(x)	__builtin_expect(x, 1)

struct event {
	unsigned int type;
	void *data;
};

enum event_type {
	EVENT_TYPE_TIMER = 0,
	EVENT_TYPE_DESC,
	EVENT_TYPE_NET_RX,
	EVENT_TYPE_IPC,
	EVENT_TYPE_MEDIA,
	EVENT_TYPE_MCLOCK,
	EVENT_TYPE_NET_TX_TS,

	/* Net rx task events */
	EVENT_PHY_UP,
	EVENT_PHY_DOWN,
	EVENT_RX_TIMER,
	EVENT_SOCKET_BIND,
	EVENT_SOCKET_UNBIND,
	EVENT_SOCKET_CALLBACK,
	EVENT_SOCKET_OPT,

	/* Net tx task events */
	EVENT_SOCKET_CONNECT,
	EVENT_SOCKET_DISCONNECT,
	EVENT_QOS_SCHED,
	EVENT_SR_CONFIG,
	EVENT_FP_SCHED,

	/* Hr timer task events */
	EVENT_HR_TIMER_ENQUEUE,
	EVENT_HR_TIMER_CANCEL,
	EVENT_HR_TIMER_TIMEOUT,
};
#endif /* _FREERTOS_OSAL_SYS_TYPES_H_ */
