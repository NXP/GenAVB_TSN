/*
* Copyright 2018, 2020, 2022-2023 NXP
*
* SPDX-License-Identifier: BSD-3-Clause
*/

/**
 \file init.h
 \brief GenAVB API private includes
 \details private definitions for the GenAVB library

 \copyright Copyright 2018, 2020, 2022-2023 NXP
*/

#ifndef _FREERTOS_PRIVATE_INIT_H_
#define _FREERTOS_PRIVATE_INIT_H_

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "common/ipc.h"

#define AVTP_EVENT_QUEUE_LENGTH	8

struct genavb_handle {
	unsigned int flags;
#ifdef CONFIG_MANAGEMENT
	void *management_handle;
#endif
#ifdef CONFIG_GPTP
	void *gptp_handle;
#endif
#ifdef CONFIG_SRP
	void *srp_handle;
#endif
#ifdef CONFIG_AVDECC
	void *avdecc_handle;
#endif
#ifdef CONFIG_AVTP
	TaskHandle_t avtp_handle;
	struct ipc_tx avtp_tx;
	struct ipc_rx avtp_rx;
	QueueHandle_t event_queue_handle;
	StaticQueue_t event_queue;
	uint8_t event_queue_buffer[AVTP_EVENT_QUEUE_LENGTH * sizeof(struct event)];
#endif
#ifdef CONFIG_MAAP
	void *maap_handle;
#endif
};

#endif /* _FREERTOS_PRIVATE_INIT_H_ */
