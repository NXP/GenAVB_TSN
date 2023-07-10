/*
* Copyright 2019-2020, 2023 NXP
*
* SPDX-License-Identifier: BSD-3-Clause
*/

/**
 @file
 @brief basic queue implementation
 @details
*/

#ifndef _QUEUE_H_
#define _QUEUE_H_

#include "os/sys_types.h"
#include "atomic.h"

#define QUEUE_ENTRIES_MAX	32

typedef unsigned int atomic_t;
static inline void smp_wmb(void) {}

#include "common/os/queue_common.h"

#endif /* _QUEUE_H_ */
