/*
 * Copyright 2018, 2020-2021, 2023, 2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 \file streaming.h
 \brief GenAVB API private streaming API includes
 \details private definitions for the GenAVB library streaming API
*/

#ifndef _PRIVATE_STREAMING_H_
#define _PRIVATE_STREAMING_H_

#include "genavb/genavb.h"

#include "api/init.h"
#include "api_os/streaming.h"

int connect_avtp(struct genavb_handle *genavb, struct genavb_stream_handle *stream,
		 unsigned int *max_payload_size, unsigned int *batch);

int disconnect_avtp(struct genavb_handle *genavb, struct genavb_stream_params const *params);

int static_set_id_alloc(struct genavb_handle *genavb, u16 *set_id, avtp_direction_t direction);

void static_set_id_free(struct genavb_handle *genavb, u16 set_id, avtp_direction_t direction);

#endif /* _PRIVATE_STREAMING_H_ */
