/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2017, 2019, 2023-2025 NXP
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef _QUEUE_H_
#define _QUEUE_H_

#define QUEUE_ENTRIES_MAX	32

#ifdef __KERNEL__

#include <linux/atomic.h>
#include "pool.h"

#include "queue_common.h"

#endif /* __KERNEL__ */

#endif /* _QUEUE_H_ */
