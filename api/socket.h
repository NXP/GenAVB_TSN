/*
 * Copyright 2018, 2020-2021, 2023, 2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 \file socket.h
 \brief GenAVB API private includes
 \details private definitions for the GenAVB library
*/

#ifndef _PRIVATE_SOCKET_H
#define _PRIVATE_SOCKET_H

#include "common/net.h"
#include "include/genavb/socket.h"

#define HEADER_TEMPLATE_SIZE 18

struct genavb_socket_rx {
	genavb_sock_f_t flags;
	struct net_rx net;
	struct genavb_socket_rx_params params;
	unsigned long priv;
};

struct genavb_socket_tx {
	genavb_sock_f_t flags;
	struct net_tx net;
	struct genavb_socket_tx_params params;
	uint8_t header_template[HEADER_TEMPLATE_SIZE];
	int header_len;
};

int socket_rx_event_init(struct genavb_socket_rx *sock);
void socket_rx_event_exit(struct genavb_socket_rx *sock);
int socket_rx_event_check(struct genavb_socket_rx *sock);
void socket_rx_event_rearm(struct genavb_socket_rx *sock);
bool socket_rx_flags_ok(genavb_sock_f_t flags);

bool socket_tx_flags_ok(genavb_sock_f_t flags);

static inline struct net_rx_desc *net_rx_desc_from_buffer(void *buf)
{
	struct net_rx_desc *desc;

	desc = (struct net_rx_desc *)((uint8_t *)buf - NET_DATA_OFFSET);

	return desc;
}

static inline void *net_rx_desc_to_buffer(struct net_rx_desc *desc)
{
	void *buf;

	buf = (void *)((uint8_t *)desc + NET_DATA_OFFSET);

	return buf;
}

static inline struct net_tx_desc *net_tx_desc_from_buffer(void *buf)
{
	struct net_tx_desc *desc;

	desc = (struct net_tx_desc *)((uint8_t *)buf - NET_DATA_OFFSET);

	return desc;
}

static inline void *net_tx_desc_to_buffer(struct net_tx_desc *desc)
{
	void *buf;

	buf = (void *)((uint8_t *)desc + NET_DATA_OFFSET);

	return buf;
}

#endif /* _PRIVATE_SOCKET_H */
