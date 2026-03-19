/*
 * Copyright 2020-2021, 2023, 2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 \file socket.c
 \brief control public API for linux
 \details
*/

#define _POSIX_C_SOURCE 200809L

#include "genavb/error.h"
#include "os/stdlib.h"
#include "api/socket.h"

int socket_rx_event_init(struct genavb_socket_rx *sock)
{
	if (sock->flags & GENAVB_SOCKF_NONBLOCK) {
		sock->priv = -1;
		return 0;
	} else {
		return -1;
	}
}

void socket_rx_event_exit(struct genavb_socket_rx *sock)
{
}

int socket_rx_event_check(struct genavb_socket_rx *sock)
{
	return 0;
}

void socket_rx_event_rearm(struct genavb_socket_rx *sock)
{
}

bool socket_rx_flags_ok(genavb_sock_f_t flags)
{
	if (flags & GENAVB_SOCKF_ZEROCOPY)
		return false;

	return true;
}

int genavb_socket_rx_fd(struct genavb_socket_rx *sock)
{
	if (sock->net.fd < 0)
		return -GENAVB_ERR_SOCKET_INVALID;

	return sock->net.fd;
}

bool socket_tx_flags_ok(genavb_sock_f_t flags)
{
	if (flags & GENAVB_SOCKF_ZEROCOPY)
		return false;

	return true;
}

int genavb_socket_tx_fd(struct genavb_socket_tx *sock)
{
	if (sock->net.fd < 0)
		return -GENAVB_ERR_SOCKET_INVALID;

	return sock->net.fd;
}
