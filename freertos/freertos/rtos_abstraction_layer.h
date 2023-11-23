/*
* Copyright 2023 NXP
*
* SPDX-License-Identifier: BSD-3-Clause
*/

/**
 @file
 @brief Top header file to include FreeRTOS' API
 @details
*/

#ifndef _FREERTOS_RTOS_ABSTRACTION_LAYER_H_
#define _FREERTOS_RTOS_ABSTRACTION_LAYER_H_

#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "semphr.h"
#include "event_groups.h"
#include "timers.h"
#include "atomic.h"

static inline TickType_t pdMS_TO_TICKS_AT_LEAST(unsigned int uMsec, unsigned int xMinTicks)
{
	TickType_t xMsecTicks;

	xMsecTicks = pdMS_TO_TICKS(uMsec);

	return (xMsecTicks < xMinTicks ? xMinTicks : xMsecTicks);
}

static inline unsigned int pdTICKS_TO_UINT(TickType_t ticks)
{
	return ticks;
}

static inline TickType_t pdUINT_TO_TICKS(unsigned int ticks)
{
	return ticks;
}

#endif /* _FREERTOS_RTOS_ABSTRACTION_LAYER_H_ */
