/*
 * Copyright 2018-2021, 2023, 2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 \file control.c
 \brief GenAVB public API for linux
 \details API definition for the GenAVB library
*/

#define _POSIX_C_SOURCE 200809L

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/poll.h>
#include <pthread.h>

#include "common/ipc.h"

#include "api/control.h"

int avb_ipc_receive_sync(struct ipc_rx const *rx, unsigned int *msg_type, void *msg, unsigned int *msg_len, int timeout)
{
	struct pollfd sync_poll;
	int rc;

	sync_poll.fd = rx->fd;
	sync_poll.events = POLLIN;
	rc = poll(&sync_poll, 1, timeout);
	while (rc == -1) {
		if (errno == EINTR)
			rc = poll(&sync_poll, 1, timeout);
		else {
			rc = -GENAVB_ERR_CTRL_RX;
			goto exit;
		}
	}

	if (rc != 0) {
		if (sync_poll.revents & POLLIN)
			rc = avb_ipc_receive(rx, msg_type, msg, msg_len);
		else
			rc = -GENAVB_ERR_CTRL_RX;
	} else
		rc = -GENAVB_ERR_CTRL_TIMEOUT;

exit:
	return rc;
}

int genavb_control_rx_fd(struct genavb_control_handle const *handle)
{
	return handle->rx.fd;
}

int genavb_control_tx_fd(struct genavb_control_handle const *handle)
{
	return handle->tx.fd;
}

/* Platform-specific initialization (no-op for Linux) */
int control_init(struct genavb_control_handle *control, genavb_control_id_t id)
{
	return GENAVB_SUCCESS;
}

void *control_priv_data(struct genavb_control_handle *control)
{
	return (void *)-1;
}
