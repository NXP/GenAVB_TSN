/*
* Copyright 2018, 2020 NXP
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
 \file init.h
 \brief GenAVB API private includes
 \details private definitions for the GenAVB library

 \copyright Copyright 2018, 2020 NXP
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
	TaskHandle_t management_handle;
#endif
#ifdef CONFIG_GPTP
	TaskHandle_t gptp_handle;
#endif
#ifdef CONFIG_SRP
	TaskHandle_t srp_handle;
#endif
#ifdef CONFIG_AVDECC
	TaskHandle_t avdecc_handle;
#endif
#ifdef CONFIG_AVTP
	TaskHandle_t avtp_handle;
	struct ipc_tx avtp_tx;
	struct ipc_rx avtp_rx;
	QueueHandle_t event_queue_handle;
	StaticQueue_t event_queue;
	uint8_t event_queue_buffer[AVTP_EVENT_QUEUE_LENGTH * sizeof(struct event)];
#endif
};

#endif /* _FREERTOS_PRIVATE_INIT_H_ */
