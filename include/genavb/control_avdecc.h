/*
 * Copyright 2018-2019, 2021-2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 \file control_avdecc.h
 \brief GenAVB/TSN public control API
 \details AVDECC control API definition for the GenAVB/TSN library
*/

#ifndef _GENAVB_PUBLIC_CONTROL_AVDECC_API_H_
#define _GENAVB_PUBLIC_CONTROL_AVDECC_API_H_

#include "control.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "config.h"
#include "types.h"
#include "avdecc.h"
#include "streaming.h"

#include "genavb/aem.h"

/**
 * \ingroup control
 */
#define AVB_AECP_MAX_MSG_SIZE		532 	/**< Maximum size of an AECP message. Based on IEEE 1722.1-2013 section 9.2.1.1.7 which specifies a maximum of 524, excluding the target entity ID. */

/**
 * \ingroup control
 * AVDECC AECP messages.
 */
struct genavb_aecp_msg {
	avb_u8 msg_type;			/**< AECP message_type, from ::aecp_message_type_t. Usually one of ::AECP_AEM_COMMAND, ::AECP_AEM_RESPONSE, ::AECP_ADDRESS_ACCESS_COMMAND, ::AECP_ADDRESS_ACCESS_RESPONSE. */
	avb_u8 status;				/**< status field, from ::aecp_status_t. */
	avb_u16 len;				/**< length of the data in buf, i.e. length of the AECP AEM/ADDRESS_ACCESS message, including the entity ID field. */
	avb_u8 buf[AVB_AECP_MAX_MSG_SIZE];	/**< AECP AEM/ADDRESS_ACCESS PDU structure. */
};

/**
 * \ingroup control
 * AVDECC ADP messages.
 * Will be sent by the stack to notify the application of a new entity or of a departing one.
 * May also be sent by the application to request details about a given entity. In such a case, the application shall set msg_type to ::ADP_ENTITY_AVAILABLE,
 * info.entity_id to the AVDECC id the requested entity, with the other fields being unused. The stack will reply with the corresponding entity if it exists (the same entity
 * may appear on multiple interfaces: the stack will reply with one message for each interface).
 * May also be sent by the application to  request a dump of all entities. In such a case, the application shall set msg_type to ::ADP_ENTITY_DISCOVER, with
 * the other fields being unused. The stack will reply with a list of all currently discovered entities (one message for each entity).
 * If no entity was found or if an invalid msg_type was used, a reply will be sent to the application with msg_type set to ::ADP_ENTITY_NOTFOUND.
 */
struct genavb_adp_msg {
	avb_u8 msg_type;		/**< ADP message_type, according to table 6.1 of 1722.1-2013. One of ::ADP_ENTITY_AVAILABLE, ::ADP_ENTITY_DEPARTING, ::ADP_ENTITY_DISCOVER. */
	avb_u16 total;			/**< total number of discovered entities. */
	struct entity_info info;	/**< description of the entity being reported. */
};

/**
 * \ingroup control
 * AVDECC ACMP commands.
 * Sent by the application to request a connection-related action (connect/disconnect a stream, get info on a stream).
 * Derived from section 8.2.2.6 of 1722.1-2013.
 */
struct genavb_acmp_command {
	acmp_message_type_t message_type;	/**< Type of ACMP message. Must be one of the COMMAND values defined in ::acmp_message_type_t; */
	avb_u64 talker_entity_id;		/**< Entity id of the AVDECC talker targeted by the command (in network order). */
	avb_u64 listener_entity_id;		/**< Entity id of the AVDECC listener targeted by the command (in network order). */
	avb_u16 talker_unique_id;		/**< Uniquely identifies the stream source of the AVDECC talker. Usually the id of the STREAM_OUTPUT descriptor. */
	avb_u16 listener_unique_id;		/**< Uniquely identifies the stream sink of the AVDECC listener. Usually the id of the STREAM_INPUT descriptor. */
	avb_u16 connection_count;		/**< Used by the GET_TX_CONNECTION_COMMAND to retrieve information about a specific talker connection. */
	avb_u16 flags;				/**< Stream attributes. Bitmask of ACMP_FLAG_* values. */
	avb_u16 stream_vlan_id;			/**< unused? */
};

/**
 * \ingroup control
 * AVDECC ACMP responses.
 * Sent by the stack in reply to an ACMP  command from the controller application.
 * Derived from section 8.2.2.1 of 1722.1-2013.
 */
struct genavb_acmp_response {
	acmp_message_type_t message_type;	/**< Type of ACMP message. Must be one of the RESPONSE values defined in ::acmp_message_type_t; */
	acmp_status_t status;			/**< Status result of the command. */
	avb_u64 stream_id;			/**< Id of the stream the command operated on (in network order). */
	avb_u64 talker_entity_id;		/**< Entity id of the AVDECC talker targeted by the command (in network order). */
	avb_u64 listener_entity_id;		/**< Entity id of the AVDECC listener targeted by the command (in network order). */
	avb_u16 talker_unique_id;		/**< Uniquely identifies the stream source of the AVDECC talker. Usually the id of the STREAM_OUTPUT descriptor (in network order). */
	avb_u16 listener_unique_id;		/**< Uniquely identifies the stream sink of the AVDECC listener. Usually the id of the STREAM_INPUT descriptor (in network order). */
	avb_u8 stream_dest_mac[6];		/**< Destination MAC address used for the stream (in network order). */
	avb_u16 connection_count;		/**< Used by the GET_TX_CONNECTION_COMMAND to retrieve information about a specific talker connection (in network order). */
	avb_u16 flags;				/**< Stream attributes. Bitmask of ACMP_FLAG_* values. */
	avb_u16 stream_vlan_id;			/**< VLAN id assigned to the stream (normally coming from SRP on the talker). In network order.  */
};

/**
 * \ingroup control
 * CONTROLLER channel message type.
 */
union genavb_controller_msg {
	struct genavb_aecp_msg aecp;		/**< AECP message */
	struct genavb_adp_msg adp;			/**< ADP message */
	struct genavb_acmp_command acmp_command;	/**< ACMP command */
	struct genavb_acmp_response acmp_response;	/**< ACMP response */
};

/**
 * \ingroup control
 * CONTROLLED channel message type.
 */
union genavb_controlled_msg {
	struct genavb_aecp_msg aecp;		/**< AECP message */

};

/**
 * \ingroup control
 *  Avdecc connect parameters
 */
struct genavb_msg_media_stack_connect {
	struct genavb_stream_params stream_params;
	avb_u16 entity_index;			/**< Avdecc entity index */
	avb_u16 configuration_index;		/**< Avdecc configuration descriptor index */
	avb_u16 stream_index;			/**< Avdecc stream unique id */
	avb_u16 flags;				/**< Avdecc stream flags */
};

/**
 * \ingroup control
 *  Avdecc disconnect parameters
 */
struct genavb_msg_media_stack_disconnect {
	avb_u16 stream_index;           /**< Avdecc stream unique id */
	avb_u8 stream_id[8];            /**< Stream ID (in network order) */
	avb_u16 port;                   /**< Network port */
	sr_class_t stream_class;        /**< Stream class */
	avtp_direction_t direction;     /**< Stream direction */
};

/**
 * \ingroup control
 * AVDECC Listener stream started status
 */
typedef enum {
	ACMP_LISTENER_STREAM_STOPPED = 0,	/**< This stream is stopped and should discard the stream AVTPDUs. */
	ACMP_LISTENER_STREAM_STARTED		/**< This stream is started and should process the stream AVTPDUs. */
} genavb_acmp_listener_stream_started_status_t;

/**
 * \ingroup control
 *  Avdecc binding parameters
 */
struct genavb_msg_media_stack_bind {
	avb_u64 entity_id;					/**< Avdecc listener entity id */
	avb_u16 entity_index;					/**< Avdecc listener entity index */
	avb_u16 listener_stream_index;				/**< Avdecc listener stream index */
	avb_u64 talker_entity_id;				/**< Avdecc talker entity id */
	avb_u16 talker_stream_index;				/**< Avdecc talker stream index */
	avb_u64 controller_entity_id;				/**< Avdecc controller entity id */
	genavb_acmp_listener_stream_started_status_t started;	/**< Avdecc stream started/stopped flag ::genavb_acmp_listener_stream_started_status_t */
};

/**
 * \ingroup control
 *  Avdecc unbinding parameters
 */
struct genavb_msg_media_stack_unbind {
	avb_u64 entity_id;			/**< Avdecc listener entity id */
	avb_u16 entity_index;			/**< Avdecc listener entity index */
	avb_u16 listener_stream_index;		/**< Avdecc listener stream index */
};

/**
 * \ingroup control
 * AVDECC descriptors' parameter types
 */
typedef enum {
	AVDECC_PERSISTENT_PARAM_SAMPLING_RATE = 0,		/**< Sampling rate from an AUDIO_UNIT */
	AVDECC_PERSISTENT_PARAM_FORMAT,				/**< Format from a STREAM_INPUT/OUTPUT */
	AVDECC_PERSISTENT_PARAM_PRESENTATION_TIME_OFFSET,	/**< Presentation time offset from a STREAM_OUTPUT */
	AVDECC_PERSISTENT_PARAM_CLOCK_SOURCE_INDEX,		/**< Clock source index from a CLOCK_DOMAIN */
	AVDECC_PERSISTENT_PARAM_OBJECT_NAME,			/**< Object name from a descriptor */
	AVDECC_PERSISTENT_PARAM_GROUP_NAME,			/**< Group name from an ENTITY */
	AVDECC_PERSISTENT_PARAM_ENTITY_NAME			/**< Entity name from an ENTITY */
} genavb_avdecc_persistent_param_t;

/**
 * \ingroup control
 * Avdecc descriptor's persistent parameter message
 */
struct genavb_msg_media_stack_persistent_param {
	avb_u64 entity_id;				/**< Avdecc entity id in network order */
	avb_u16 desc_type;				/**< Avdecc descriptor type */
	avb_u16 desc_index;				/**< Avdecc descriptor index */
	genavb_avdecc_persistent_param_t param_type;	/**< Avdecc descriptor's parameter type */
	union {
		avb_u64 format;				/**< Avdecc STREAM_INPUT/OUTPUT current format */
		avb_u32 presentation_time_offset;	/**< Avdecc STREAM_OUTPUT current presentation time offset */
		avb_u32 sampling_rate;			/**< Avdecc AUDIO_UNIT current sampling rate */
		avb_u16 clock_source_index;		/**< Avdecc CLOCK_DOMAIN current clock source */
		avb_u8 name[AEM_STR_LEN_MAX];		/**< Avdecc descriptors' current entity/group/object name */
	} u;
};

struct genavb_msg_memory_object_desc_attr {
	avb_u16 cfg_idx;		/* memory object's AEM configuration index */
	avb_u16 desc_idx;		/* memory object's AEM descriptor index */
	avb_u16 object_type;		/* memory object's type */
	avb_u64 max_len;		/* maximum length in bytes of the memory object */
	avb_u64 max_segment_len;	/* maximum length in bytes of a segment that can be used with the Memory Object Uploads */
	avb_u64 len;			/* actual length in bytes of the memory object */
};

/**
 * \ingroup control
 * Avdecc descriptor's memory object message
 */
struct genavb_msg_media_stack_memory_object {
	avb_u64 entity_id;	/**< Avdecc entity id in network order (an ID of 0 means the first non-controller entity) */
	struct genavb_msg_memory_object_desc_attr desc_attr;
};

/**
 * \ingroup control
 * Avdecc start message
 */
struct genavb_msg_media_stack_start {
	avb_u64 entity_id;				/**< Avdecc entity id in network order (an ID of 0 start the first non-controller entity) */
};

/**
 * \ingroup control
 * MEDIA_STACK channel message type.
 */
union genavb_media_stack_msg {
	struct genavb_msg_media_stack_connect media_stack_connect;		/**< MEDIA STACK Connect parameters message */
	struct genavb_msg_media_stack_disconnect media_stack_disconnect;	/**< MEDIA STACK Disconnect parameters message */
	struct genavb_msg_media_stack_bind media_stack_bind;			/**< MEDIA STACK binding parameters message */
	struct genavb_msg_media_stack_unbind media_stack_unbind;		/**< MEDIA STACK unbinding parameters message */
	struct genavb_msg_clock_domain_set_source media_stack_set_clock_source;	/**< MEDIA STACK set clock source parameters message */
	struct genavb_msg_media_stack_persistent_param media_stack_persistent_param;	/**< MEDIA STACK descriptors' persistent parameters message */
	struct genavb_msg_media_stack_memory_object media_stack_memory_object;	/**< MEDIA STACK memory object information message */
	struct genavb_msg_media_stack_start media_stack_start;			/**< MEDIA STACK start message */
};


#ifdef __cplusplus
}
#endif

#endif /* _GENAVB_PUBLIC_CONTROL_AVDECC_API_H_ */
