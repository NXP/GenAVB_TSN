/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016, 2018-2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief AVTP main header file
 @details Definition of AVTP stack component entry point functions and global context structure.
*/

#ifndef _AVTP_H_
#define _AVTP_H_

#include "common/net.h"
#include "common/ipc.h"
#include "common/list.h"
#include "common/avtp.h"
#include "common/timer.h"
#include "common/61883_iidc.h"
#include "common/log.h"
#include "clock_domain.h"
#include "media_clock.h"

#include "avtp_entry.h"

#define AVTP_RECEIVE_TIMEOUT	100000000UL	/* 100ms timeout after which we consider a stream to have been interrupted */

struct avtp_port {
	struct list_head talker;

	struct list_head listener;

	struct clock_source ptp_source;

	unsigned int logical_port;
	unsigned int port_id;

	os_clock_id_t clock_gptp;
};

/**
 * AVTP global context structure
 */
struct avtp_ctx {
	struct ipc_rx ipc_rx_media_stack;
	struct ipc_tx ipc_tx_media_stack;

	struct ipc_rx ipc_rx_clock_domain;
	struct ipc_tx ipc_tx_clock_domain;
	struct ipc_tx ipc_tx_clock_domain_sync;

	struct ipc_tx ipc_tx_stats;

	struct ipc_rx ipc_rx_avdecc;
	struct ipc_tx ipc_tx_avdecc;

	struct list_head stream_destroyed;
	struct list_head redundant_set_destroyed;

	struct clock_domain domain[AVTP_CFG_NUM_DOMAINS];

	struct timer_ctx *timer_ctx;

	unsigned long priv;

	u32 stream_listener_count;
	u32 stream_talker_count;

	struct list_head redundant_set_talker;
	struct list_head redundant_set_listener;

	unsigned int port_max;

	/* variable size array */
	struct avtp_port port[];
};


/**
 * AVTP receive descriptor
 */
struct avtp_rx_desc {
	struct net_rx_desc desc;

	u16 l4_offset;		/**< offset (in bytes) to layer 4 header */
	u16 l4_len;		/**< length (in bytes) of layer 4 header and payload */
	u32 format_specific_data_2;
	u16 protocol_specific_header;
	u16 flags;
	u32 avtp_timestamp;
};

struct ipc_avtp_process_stats {
	struct process_stats stats;
};

#define avtp_port_to_context(port_)	container_of(port_, struct avtp_ctx, port[port_->port_id])

unsigned int avtp_to_logical_port(unsigned int port_id);
unsigned int avtp_to_clock_gptp(unsigned int port_id);
os_clock_id_t avtp_to_clock_avtp(unsigned int port_id);
unsigned int avtp_data_header_init(struct avtp_data_hdr *avtp_data, u8 subtype, void *stream_id);
void avtp_alternative_net_rx(struct net_rx *net_rx, struct net_rx_desc **desc, unsigned int n);
void avtp_stream_net_rx(struct net_rx *, struct net_rx_desc **, unsigned int);

static inline void avtp_data_header_set_timestamp(struct avtp_data_hdr *avtp_data, u32 tstamp)
{
	avtp_data->avtp_timestamp = htonl(tstamp);
	avtp_data->tv = 1;
}

static inline void avtp_data_header_set_timestamp_invalid(struct avtp_data_hdr *avtp_data)
{
	avtp_data->avtp_timestamp = 0;
	avtp_data->tv = 0;
}

static inline void avtp_data_header_set_len(struct avtp_data_hdr *avtp_data, u16 len)
{
	avtp_data->stream_data_length = htons(len);
}

static inline void avtp_data_header_toggle_mcr(struct avtp_data_hdr *avtp_data)
{
	avtp_data->mr ^= 1;
}

static inline void avtp_data_header_set_protocol_specific(struct avtp_data_hdr *avtp_data, u16 protocol_specific_header)
{
	avtp_data->protocol_specific_header = protocol_specific_header;
}

#endif /* _AVTP_H_ */
