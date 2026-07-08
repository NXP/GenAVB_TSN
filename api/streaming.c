/*
* Copyright 2018, 2021, 2023-2025 NXP
*
* SPDX-License-Identifier: BSD-3-Clause
*/

/**
 \file streaming.c
 \brief GenAVB public API for linux
 \details API definition for the GenAVB library
*/

#include "os/string.h"
#include "os/stdlib.h"
#include "common/ipc.h"
#include "common/avtp.h"

#include "genavb/avdecc.h"

#include "streaming.h"
#include "control.h"
#include "clock.h"

/* Searches for a valid redundant set_id in the right set_id range.
 * Also marks the found set_id as now used.
 *
 * \return			0 if a valid set_id could be found for the static stream, -1 otherwise
 * \param genavb		pointer to the genavb handle tracking the redundant set for static streams
 * \param set_id		input/output parameter, the new redundant set's id to be set to the found set_id
 * \param direction		direction of the stream, either AVTP_DIRECTION_LISTENER or AVTP_DIRECTION_TALKER
 */
int static_set_id_alloc(struct genavb_handle *genavb, u16 *set_id, avtp_direction_t direction)
{
	u64 *static_set_id_mask;
	unsigned short id_base;
	int rc = -1;
	int i;

	if (direction == AVTP_DIRECTION_LISTENER) {
		id_base = STATIC_STREAM_ID_BASE + LISTENER_SET_ID_BASE;
		static_set_id_mask = &genavb->listener_static_set_id_mask;
	} else {
		id_base = STATIC_STREAM_ID_BASE + TALKER_SET_ID_BASE;
		static_set_id_mask = &genavb->talker_static_set_id_mask;
	}

	for (i = 0; i < STATIC_STREAM_NUM_SET_ID; i++) {
		if (!(((*static_set_id_mask) >> i) & 0x1)) {
			*static_set_id_mask |= (1ULL << i);

			*set_id = i + id_base;

			rc = 0;
			break;
		}
	}

	return rc;
}

/* Clears marked/previously used redundant set_id for a static stream.
 *
 * \return			none
 * \param genavb		pointer to the genavb handle tracking the redundant set for static streams
 * \param set_id		the redundant set's id of the static stream
 * \param direction		direction of the stream, either AVTP_DIRECTION_LISTENER or AVTP_DIRECTION_TALKER
 */
void static_set_id_free(struct genavb_handle *genavb, u16 set_id, avtp_direction_t direction)
{
	u64 *static_set_id_mask;
	unsigned short id_base;

	if (direction == AVTP_DIRECTION_LISTENER) {
		static_set_id_mask = &genavb->listener_static_set_id_mask;
		id_base = STATIC_STREAM_ID_BASE + LISTENER_SET_ID_BASE;
	} else {
		static_set_id_mask = &genavb->talker_static_set_id_mask;
		id_base = STATIC_STREAM_ID_BASE + TALKER_SET_ID_BASE;
	}

	*static_set_id_mask &= ~(1ULL << (set_id - id_base));
}

unsigned int genavb_stream_presentation_offset(const struct genavb_stream_handle *handle)
{
	return stream_presentation_offset(handle->params.talker.max_transit_time, handle->params.talker.latency);
}

genavb_clock_id_t genavb_stream_avtp_clock(const struct genavb_stream_handle *handle)
{
	return handle->clock_avtp;
}

int streaming_init(struct genavb_handle *genavb);

#define AVTP_TIMEOUT 3000
int connect_avtp(struct genavb_handle *genavb, struct genavb_stream_handle *stream,
		 unsigned int *max_payload_size, unsigned int *batch)
{
	struct genavb_stream_params *params = &stream->params;
	int rc;
	unsigned int msg_type, msg_len;

	if (!(genavb->flags & AVTP_INITIALIZED)) {
		rc = streaming_init(genavb);
		if (rc < 0)
			goto exit;

		genavb->flags |= AVTP_INITIALIZED;
	}

	/*
	* Send connect to AVTP
	*/
	rc = avb_ipc_send(&genavb->avtp_tx, IPC_AVTP_CONNECT, params, sizeof(*params), 0);
	if (rc != GENAVB_SUCCESS)
		goto exit;

	if (params->direction == AVTP_DIRECTION_LISTENER) {
		struct ipc_avtp_listener_connect_response response;

		msg_len = sizeof(response);
		rc = avb_ipc_receive_sync(&genavb->avtp_rx, &msg_type, &response, &msg_len, AVTP_TIMEOUT);
		if (rc != GENAVB_SUCCESS)
			goto exit;

		if (msg_type != IPC_AVTP_LISTENER_CONNECT_RESPONSE) {
			rc = -GENAVB_ERR_CTRL_RX;
			goto exit;
		}

		if (os_memcmp(&response.stream_id, params->stream_id, 8)) {
			rc = -GENAVB_ERR_CTRL_RX;
			goto exit;
		}

		if (response.status != GENAVB_SUCCESS){
			rc = -response.status;
			goto exit;
		}

		rc = response.status;

		stream->clock_avtp = os_clock_to_genavb_clock(response.clock_avtp);

	} else {
		struct ipc_avtp_talker_connect_response response;

		msg_len = sizeof(response);
		rc = avb_ipc_receive_sync(&genavb->avtp_rx, &msg_type, &response, &msg_len, AVTP_TIMEOUT);
		if (rc != GENAVB_SUCCESS)
			goto exit;

		if (msg_type != IPC_AVTP_TALKER_CONNECT_RESPONSE) {
			rc = -GENAVB_ERR_CTRL_RX;
			goto exit;
		}

		if (os_memcmp(&response.stream_id, params->stream_id, 8)) {
			rc = -GENAVB_ERR_CTRL_RX;
			goto exit;
		}

		if (response.status != GENAVB_SUCCESS){
			rc = -response.status;
			goto exit;
		}

		rc = response.status;

		params->talker.latency = response.latency;

		stream->clock_avtp = os_clock_to_genavb_clock(response.clock_avtp);
		*batch = response.batch;
		*max_payload_size = response.max_payload_size;
	}

exit:
	return rc;
}

int disconnect_avtp(struct genavb_handle *genavb, struct genavb_stream_params const *params)
{
	struct ipc_avtp_disconnect avtp_disconnect;
	int rc;
	struct ipc_avtp_disconnect_response avtp_disconnect_response;
	unsigned int msg_type, msg_len;

	if (!(genavb->flags & AVTP_INITIALIZED)) {
		rc = -GENAVB_ERR_CTRL_TX;
		goto exit;
	}

	/*
	* Send talker disconnect to AVTP
	*/
	os_memcpy(&avtp_disconnect.stream_id, params->stream_id, 8);
	avtp_disconnect.stream_class = params->stream_class;
	avtp_disconnect.port = params->port;
	avtp_disconnect.direction = params->direction;

	rc = avb_ipc_send(&genavb->avtp_tx, IPC_AVTP_DISCONNECT, &avtp_disconnect, sizeof(struct ipc_avtp_disconnect), 0);
	if (rc != GENAVB_SUCCESS)
		goto exit;

	msg_len = sizeof(struct ipc_avtp_disconnect_response);
	rc = avb_ipc_receive_sync(&genavb->avtp_rx, &msg_type, &avtp_disconnect_response, &msg_len, AVTP_TIMEOUT);
	if (rc != GENAVB_SUCCESS)
		goto exit;

	if (msg_type != IPC_AVTP_DISCONNECT_RESPONSE) {
		rc = -GENAVB_ERR_CTRL_RX;
		goto exit;
	}

	if (os_memcmp(&avtp_disconnect_response.stream_id, params->stream_id, 8)) {
		rc = -GENAVB_ERR_CTRL_RX;
		goto exit;
	}

	if (avtp_disconnect_response.status != GENAVB_SUCCESS){
		rc = -avtp_disconnect_response.status;
		goto exit;
	}

	rc = avtp_disconnect_response.status;

exit:
	return rc;
}
