/*
* Copyright 2018, 2020-2021, 2023 NXP
*
* SPDX-License-Identifier: BSD-3-Clause
*/

/**
 \file socket.c
 \brief control public API for freertos
 \details
 \copyright Copyright 2018, 2020-2021, 2023 NXP
*/

#include "rtos_abstraction_layer.h"

#include "common/ipc.h"
#include "freertos/net_port.h"

#include "api/socket.h"

#define SOCKET_EVENT_QUEUE_LENGTH	16

int socket_rx_event_init(struct genavb_socket_rx *sock)
{
	QueueHandle_t event_queue_h = NULL;

	if (sock->flags & GENAVB_SOCKF_NONBLOCK) {
		goto exit;
	} else {
		event_queue_h = xQueueCreate(SOCKET_EVENT_QUEUE_LENGTH, sizeof(struct event));
		if (!event_queue_h) {
			goto err;
		}
	}

exit:
	sock->priv = (unsigned long)event_queue_h;

	return 0;

err:
	return -1;
}

void socket_rx_event_exit(struct genavb_socket_rx *sock)
{
	if (sock->priv)
		vQueueDelete((QueueHandle_t)sock->priv);
}

int socket_rx_event_check(struct genavb_socket_rx *sock)
{
	if (sock->priv) {
		struct event e;

		/* Blocking */
		if (xQueueReceive((QueueHandle_t)sock->priv, &e, portMAX_DELAY) != pdTRUE)
			goto err;

		if (e.type != EVENT_TYPE_NET_RX)
			goto err;
	}

	return 0;

err:
	return -1;
}

void socket_rx_event_rearm(struct genavb_socket_rx *sock)
{
	if (sock->priv)
		net_rx_enable_callback(&sock->net);
}

int genavb_socket_rx_set_callback(struct genavb_socket_rx *sock, void (*callback)(void *), void *data)
{
	int rc;

	if (!sock) {
		rc = -GENAVB_ERR_INVALID;
		goto err;
	}

	if (net_rx_set_callback(&sock->net, callback, data) < 0) {
		rc = -GENAVB_ERR_CTRL_INIT;
		goto err;
	}

	return GENAVB_SUCCESS;

err:
	return rc;
}

int genavb_socket_rx_enable_callback(struct genavb_socket_rx *sock)
{
	int rc;

	if (!sock) {
		rc = -GENAVB_ERR_INVALID;
		goto err;
	}

	if (net_rx_enable_callback(&sock->net) < 0) {
		rc = -GENAVB_ERR_CTRL_INIT;
		goto err;
	}

	return GENAVB_SUCCESS;

err:
	return rc;
}
