/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2018, 2021-2026 NXP
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

/* Used for GET_COUNTERS unsolicited notifications: Per IEEE 1722-1-2021 7.5.2 and Milan v1.2 5.4.5: we should not send more than one unsolicited notification per descriptor per second. */
#define AECP_GET_COUNTERS_ASYNC_UNSOLICITED_NOTIFICATION_MS 1000 /* 1sec */
#define AECP_GET_COUNTERS_ASYNC_UNSOLICITED_NOTIFICATION_GRANULARITY_MS 100

#define AECP_GET_AS_PATH_ASYNC_UNSOLICITED_NOTIFICATION_MS 1000 /* 1sec */
#define AECP_GET_AS_PATH_ASYNC_UNSOLICITED_NOTIFICATION_GRANULARITY_MS 100

#define MILAN_CERTIFICATION_VERSION(a, b, c, d) ((u32)((u32)a << 24 | (u32)b << 16 | (u32)c << 8 | (u32)d))

#define AECP_IDENTIFY_TRANSMIT_DELAY_GRANULARITY_MS	10
#define AECP_IDENTIFY_TIMEOUT_GRANULARITY_MS		100
#define AECP_IDENTIFY_TRANSMIT_DELAY_MS			150	/* 150ms */
#define AECP_IDENTIFY_TIMEOUT_MS			1000	/* 1sec */

/**
 * Controller Entity ID for Identification notification from IEEE 1722-2021, Table 7-180
 */
#define AECP_IDENTIFY_CONTROLLER_ID	0x90e0f0fffe010001ULL

/**
 * IDENTIFY states from IEEE 1722-2021, 7.5.1.3
 */
typedef enum {
	IDENTIFY_STATE_INITIALIZE = 0,
	IDENTIFY_STATE_WAITING,
	IDENTIFY_STATE_IDENTIFY
} identify_state_t;

/**
 * IDENTIFY states from IEEE 1722-2021, 7.5.1.3
 */
typedef enum {
	IDENTIFY_EVENT_BEGIN = 0,
	IDENTIFY_EVENT_RUN
} identify_event_t;

struct identify_sm {
	identify_state_t state;
	struct timer identify_tx;
	struct timer timeout;
	bool button_pressed;
	u8 identify_to_send;
	u16 sequence_id;
};

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
	struct identify_sm identify_sm;
};

struct avdecc_port;
struct entity;


int aecp_init(struct aecp_ctx *aecp, void *data, struct avdecc_entity_config *cfg);
int aecp_exit(struct aecp_ctx *aecp);
int aecp_current_config_init(struct aecp_ctx *aecp);
void aecp_current_config_exit(struct aecp_ctx *aecp);
unsigned int aecp_data_size(struct avdecc_entity_config *cfg);
int aecp_net_rx(struct avdecc_port *port, struct aecp_pdu *pdu, u8 msg_type, u8 status, u16 len, u8 *mac_src);
int aecp_ipc_rx_controller(struct entity *entity, struct ipc_aecp_msg *aecp_msg, u32 len, struct ipc_tx *ipc, unsigned int ipc_dst);
void aecp_ipc_rx_controlled(struct entity *entity, struct ipc_aecp_msg *aecp_msg, u32 len);
int aecp_aem_send_async_unsolicited_notification(struct aecp_ctx *aecp, u16 response_type, u16 descriptor_type, u16 descriptor_index);
void aecp_register_get_counters_async_notification(struct entity *entity, u16 descriptor_type, u16 descriptor_index);
void aecp_register_get_as_path_asyn_notification(struct entity *entity, unsigned int port_id);
int aecp_update_persistent_param(struct entity *entity, struct ipc_media_stack_persistent_param *persistent_param);
int aecp_update_memory_object(struct entity *entity, struct genavb_msg_memory_object_desc_attr *memory_object);
void aecp_aem_identify_sm(struct entity *entity, identify_event_t event);

#endif /* _AECP_H_ */
