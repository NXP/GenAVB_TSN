/*
 * Copyright 2018-2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 \file init.h
 \brief GenAVB API private includes
 \details private definitions for the GenAVB library
*/

#ifndef _RTOS_PRIVATE_INIT_H_
#define _RTOS_PRIVATE_INIT_H_

#include "rtos_abstraction_layer.h"

#include "common/ipc.h"

#define AVTP_EVENT_QUEUE_LENGTH	8

struct genavb_handle {
	unsigned int flags;
#ifdef CONFIG_GENAVB_TSN_MANAGEMENT
	void *management_handle;
#endif
#ifdef CONFIG_GENAVB_TSN_GPTP
	void *gptp_handle;
#endif
#ifdef CONFIG_GENAVB_TSN_SRP
	void *srp_handle;
#endif
#ifdef CONFIG_GENAVB_TSN_AVDECC
	void *avdecc_handle;
#endif
#ifdef CONFIG_GENAVB_TSN_AVTP
	void *avtp_handle;
	struct ipc_tx avtp_tx;
	struct ipc_rx avtp_rx;
	rtos_mqueue_t event_queue;
	uint8_t event_queue_buffer[AVTP_EVENT_QUEUE_LENGTH * sizeof(struct event)];
#endif
#ifdef CONFIG_GENAVB_TSN_MAAP
	void *maap_handle;
#endif
#ifdef CONFIG_GENAVB_TSN_HSR
	void *hsr_handle;
	struct ipc_tx hsr_ipc_tx;
#endif

	u64 listener_static_set_id_mask;	/* bit mask of allocated set IDs for static non-redundant listener streams. */
	u64 talker_static_set_id_mask;		/* bit mask of allocated set IDs for static non-redundant talker streams. */
};

#endif /* _RTOS_PRIVATE_INIT_H_ */
