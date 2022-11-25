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
 \file control.h
 \brief GenAVB API private includes
 \details private definitions for the GenAVB library

 \copyright Copyright 2018, 2020 NXP
*/

#ifndef _FREERTOS_PRIVATE_CONTROL_H_
#define _FREERTOS_PRIVATE_CONTROL_H_

#include "FreeRTOS.h"
#include "queue.h"

#include "common/ipc.h"

#define AVB_CONTROL_EVENT_QUEUE_LENGTH	16

struct genavb_control_handle {
	int id;
	struct ipc_tx tx;
	struct ipc_rx rx;
	struct ipc_rx rx_sync;

	QueueHandle_t event_queue_handle;
	StaticQueue_t event_queue;
	uint8_t event_queue_buffer[AVB_CONTROL_EVENT_QUEUE_LENGTH * sizeof(struct event)];
};

#endif /* _FREERTOS_PRIVATE_CONTROL_H_ */
