/*
 * Copyright 2019-2021, 2023, 2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 \file timer.h
 \brief GenAVB API private includes
 \details private definitions for the GenAVB library
*/

#ifndef _PRIVATE_TIMER_H_
#define _PRIVATE_TIMER_H_

#include "os/timer.h"

struct genavb_timer {
	struct os_timer os_t;
	void (*callback)(void *data, int count);
	void *data;
};

#endif /* _PRIVATE_TIMER_H_ */
