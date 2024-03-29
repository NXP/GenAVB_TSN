/*
 * AVB queue
 * Copyright 2014-2015 Freescale Semiconductor, Inc.
 * Copyright 2023 NXP
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
