/*
 * Copyright 2018, 2020-2021, 2023, 2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 \file init.h
 \brief GenAVB API private includes
 \details private definitions for the GenAVB library
*/

#ifndef _LINUX_PRIVATE_INIT_H_
#define _LINUX_PRIVATE_INIT_H_

#include "common/ipc.h"
#include "common/list.h"

struct genavb_handle {
	int flags;
	struct list_head streams;
	struct list_head sets;
	struct ipc_tx avtp_tx;
	struct ipc_rx avtp_rx;

	u64 listener_static_set_id_mask;	/* bit mask of allocated set IDs for static non-redundant listener streams. */
	u64 talker_static_set_id_mask;		/* bit mask of allocated set IDs for static non-redundant talker streams. */
};

#endif /* _LINUX_PRIVATE_INIT_H_ */
