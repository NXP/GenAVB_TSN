/*
* Copyright 2014-2015 Freescale Semiconductor, Inc.
* Copyright 2020-2023 NXP
*
* SPDX-License-Identifier: BSD-3-Clause
*/

/**
 @brief AECP common definitions
*/

#ifndef _AECP_H_
#define _AECP_H_

#include "common/types.h"
#include "common/list.h"
#include "common/aecp.h"
#include "common/net.h"
#include "common/ipc.h"
#include "common/random.h"
#include "common/timer.h"

#define MILAN_PROTOCOL_VERSION 1
#define MONITOR_TIMER_GRANULARITY		100 /* Software timer granularity */
#define MONITOR_TIMER_INTERVAL_MIN		30000 /* 30sec */
#define MONITOR_TIMER_INTERVAL_MAX		60000 /* 60sec */
#define MONITOR_TIMER_INTERVAL ((unsigned int)random_range(MONITOR_TIMER_INTERVAL_MIN, MONITOR_TIMER_INTERVAL_MAX - MONITOR_TIMER_GRANULARITY))

#define MILAN_CERTIFICATION_VERSION(a, b, c, d) ((u32)((u32)a << 24 | (u32)b << 16 | (u32)c << 8 | (u32)d))

struct unsolicited_ctx {
	struct list_head list;
	u8 mac_dst[6];		/**< Controller MAC address. */
	u64 controller_id;	/**< Controller entity ID. */
	u16 port_id;		/**< Port on which the controller has registered. */
	u16 sequence_id;	/**< Sequence id of the next unsolicited notifications. */
	struct timer monitor_timer; /**< Timer to monitor if the controller is still available. Per AVNU.IO.CONTROL 7.5.3 */
	struct aecp_ctx *aecp; /**< Parent AECP context. */
};

/**
 * Context variables for the AECP protocol.
 */
struct aecp_ctx {
	struct list_head inflight_network;		/**< List of AECP commands in-flight on the network (response expected from the network) */
	struct list_head inflight_application;		/**< List of AECP commands in-flight within the application (response expected from the local application) */
	struct list_head unsolicited;			/**< List of controllers that have registered to received unsolicited notifications from this entity. */
	u16 sequence_id;				/**< Sequence ID to use for commands, in host byte order. */
	struct unsolicited_ctx *unsolicited_storage;
	struct list_head free_unsolicited;
	unsigned int max_unsolicited_registrations;
};

struct avdecc_port;
struct entity;


int aecp_init(struct aecp_ctx *aecp, void *data, struct avdecc_entity_config *cfg);
int aecp_exit(struct aecp_ctx *aecp);
unsigned int aecp_data_size(struct avdecc_entity_config *cfg);
int aecp_net_rx(struct avdecc_port *port, struct aecp_pdu *pdu, u8 msg_type, u8 status, u16 len, u8 *mac_src);
int aecp_ipc_rx_controller(struct entity *entity, struct ipc_aecp_msg *aecp_msg, u32 len, struct ipc_tx *ipc, unsigned int ipc_dst);
void aecp_ipc_rx_controlled(struct entity *entity, struct ipc_aecp_msg *aecp_msg, u32 len);
int aecp_aem_send_async_unsolicited_notification(struct aecp_ctx *aecp, u16 response_type, u16 descriptor_type, u16 descriptor_index);

#endif /* _AECP_H_ */
