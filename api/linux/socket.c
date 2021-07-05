/*
* Copyright 2020 NXP
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
 \file socket.c
 \brief control public API for linux
 \details
 \copyright Copyright 2020 NXP
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

int genavb_socket_rx_fd(struct genavb_socket_rx *sock)
{
	if (sock->net.fd < 0)
		return -GENAVB_ERR_SOCKET_INVALID;

	return sock->net.fd;
}

int genavb_socket_tx_fd(struct genavb_socket_tx *sock)
{
	if (sock->net.fd < 0)
		return -GENAVB_ERR_SOCKET_INVALID;

	return sock->net.fd;
}


