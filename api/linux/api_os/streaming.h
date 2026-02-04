/*
 * Copyright 2018, 2020-2021, 2023, 2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 \file streaming.h
 \brief GenAVB API private includes
 \details private definitions for the GenAVB library
*/

#ifndef _LINUX_PRIVATE_STREAMING_H_
#define _LINUX_PRIVATE_STREAMING_H_

#include "common/list.h"
#include "include/genavb/genavb.h"

#include "init.h"

struct genavb_set_handle {
	struct list_head list;
	struct genavb_handle *genavb;
	unsigned short id;
	unsigned int num_streams;	/* Number of created streams associated to this set. */
	unsigned int partial_iovec;
	int expect_new_frame;
	unsigned int max_payload_size;	/* Transmit maximum packet payload size (in byte units) */
	unsigned int batch;		/* Transmit batch (in packet units) */
	int fd;
	avtp_direction_t stream_direction;	/**< Stream direction */
};

struct genavb_stream_handle {
	struct list_head list;
	struct genavb_handle *genavb;
	struct genavb_set_handle *set; /* Set handle if the stream is redundant. NULL otherwise. */
	struct genavb_stream_params params;
	genavb_clock_id_t clock_avtp;
};

#endif /* _LINUX_PRIVATE_STREAMING_H_ */
