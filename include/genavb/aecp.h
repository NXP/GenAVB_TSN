/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *    Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *    Neither the name of NXP Semiconductors nor the
 *    names of its contributors may be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 \file aecp.h
 \brief GenAVB public API
 \details 1722.1 AECP format definition and helper functions.

 \copyright Copyright 2015 Freescale Semiconductor, Inc.
*/
#ifndef _GENAVB_PUBLIC_AECP_H_
#define _GENAVB_PUBLIC_AECP_H_

#include "types.h"


#define AVDECC_AECP_MAX_SIZE	(524 + sizeof(avb_u64))  /**< Max size in bytes of an AECP message, including entity id field (based on section 9.2.1.1.7 of 1722.1-2013) */


/**
 * \ingroup aem
 * From section 9.2.1 of IEEE 1722.1-2013.
 */
struct __attribute__ ((packed)) aecp_pdu {
	avb_u64 entity_id;			/**< ID of the entity targeted by (if the message is a command), or sending (if the message is a response) the message. */
	avb_u64 controller_entity_id;		/**< ID of the controller sending (if the message is a command), or receiving (if the message is a response) the message. */
	avb_u16 sequence_id;			/**< Sequence ID of the message, determined by the controller that sent the command (if any). */
};

/**
 * \ingroup aem
 * From section 9.2.1 of IEEE 1722.1-2013.
 */
struct __attribute__ ((packed)) aecp_aem_pdu {
	avb_u64 entity_id;			/**< See ::aecp_pdu. */
	avb_u64 controller_entity_id;		/**< See ::aecp_pdu. */
	avb_u16 sequence_id;			/**< See ::aecp_pdu. */
	avb_u16 u_command_type;			/**< Bit field of U bit (1 for unsolicited responses, 0 in all other cases) and ::aecp_aem_command_type_t. */
};

/**
 * \ingroup aem
 * Extracts the command type from an AECP AEM PDU.
 * \return 	command_type from ::aecp_aem_command_type_t.
 * \param hdr	pointer to struct ::aecp_aem_pdu.
 */
#define AECP_AEM_GET_CMD_TYPE(hdr) \
	(ntohs((hdr)->u_command_type) & 0x7FFF)

/**
 * \ingroup aem
 * Extracts the u bit from an AECP AEM PDU.
 * \return 	u bit (1 for unsolicited responses, 0 otherwise).
 * \param hdr	pointer to struct ::aecp_aem_pdu.
 */
#define AECP_AEM_GET_U(hdr) \
	((ntohs((hdr)->u_command_type) >> 15) & 0x1)

/**
 * \ingroup aem
 * Updates the u_command_type field in an aecp_aem_pdu structure.
 * \return 	none.
 * \param hdr	pointer to struct ::aecp_aem_pdu to be updated.
 * \param u	u bit value (1 for unsolicited responses, 0 otherwise).
 * \param ct	value of AECP AEM command_type (from ::aecp_aem_command_type_t).
 */
#define AECP_AEM_SET_U_CMD_TYPE(hdr, u, ct) \
	((hdr)->u_command_type = htons(((u & 0x1) << 15) | (ct & 0x7FFF)))

#define AECP_AEM_MAX_CONTROL_VALUES	1
#define AECP_AEM_NB_COUNTERS		32


/**
 * \ingroup aem
 * From section 7.4.5 of IEEE 1722.1-2013.
 */
struct __attribute__ ((packed)) aecp_aem_read_desc_cmd_pdu {
	avb_u16 configuration_index;	/**< Configuration from which the descriptor is to be read. */
	avb_u16 reserved;
	avb_u16 descriptor_type;	/**< Type of the descriptor to be read. */
	avb_u16 descriptor_index;	/**< Index of the descriptor to be read. */
};

/**
 * \ingroup aem
 * From section 7.4.5 of IEEE 1722.1-2013.
 */
struct __attribute__ ((packed)) aecp_aem_read_desc_rsp_pdu {
	avb_u16 configuration_index;	/**< Configuration from which the descriptor was read. */
	avb_u16 reserved;
};

/**
 * \ingroup aem
 * From section 7.4.1 of IEEE 1722.1-2013.
 */
struct __attribute__ ((packed)) aecp_aem_acquire_entity_pdu {
	avb_u32 flags;			/**< Bitmask of ::AECP_AEM_ACQUIRE_PERSISTENT, ::AECP_AEM_ACQUIRE_RELEASE. */
	avb_u64 owner_id;		/**< Set to 0 for a command, and to the Entity ID of the AVDECC Controller which owns the AVDECC Entity for a response. */
	avb_u16 descriptor_type;	/**< Type of the descriptor being acquired. */
	avb_u16 descriptor_index;	/**< Index of the descriptor being acquired. */
};

/**
 * \ingroup aem
 * From section 7.4.35 of IEEE 1722.1-2013.
 */
struct __attribute__ ((packed)) aecp_aem_start_streaming_cmd_pdu {
	avb_u16 descriptor_type;	/**< Type of the descriptor to be read. */
	avb_u16 descriptor_index;	/**< Index of the descriptor to be read. */
};

/**
 * \ingroup aem
 * From section 7.4.36 of IEEE 1722.1-2013.
 */
struct __attribute__ ((packed)) aecp_aem_stop_streaming_cmd_pdu {
	avb_u16 descriptor_type;	/**< Type of the descriptor to be read. */
	avb_u16 descriptor_index;	/**< Index of the descriptor to be read. */
};

/**
 * \ingroup aem
 * Acquire the AVDECC Entity and disable the CONTROLLER_AVAILABLE test for future ACQUIRE_ENTITY commands until released.
 * The AVDECC Entity returns an ENTITY_ACQUIRED response immediately to any other Controller.
 */
#define AECP_AEM_ACQUIRE_PERSISTENT	(1 << 0)
/**
 * \ingroup aem
 * Release the AVDECC Controller from the acquisition.
 */
#define AECP_AEM_ACQUIRE_RELEASE	(1 << 31)


/**
 * \ingroup aem
 * From section 7.4.25/26 of IEEE 1722.1-2013.
 */
struct __attribute__ ((packed)) aecp_aem_set_get_control_pdu {
	avb_u16 descriptor_type;	/**< Set to AEM_DESC_TYPE_CONTROL. */
	avb_u16 descriptor_index;	/**< Index of the Control for which the current values are being set. */
};

/**
 * \ingroup aem
 * From section 7.4.42 of IEEE 1722.1-2013.
 */
struct __attribute__ ((packed)) aecp_aem_get_counters_cmd_pdu {
	avb_u16 descriptor_type;	/**< ENTITY, AVB_INTERFACE, CLOCK_DOMAIN or STREAM_INPUT */
	avb_u16 descriptor_index;	/**< Index of the descriptor whose counters are to be fetched */
};

#define AECP_AEM_COUNTER_CLOCK_DOMAIN_LOCKED			0
#define AECP_AEM_COUNTER_CLOCK_DOMAIN_UNLOCKED			1
#define AECP_AEM_COUNTER_CLOCK_DOMAIN_ENTITY_SPECIFIC_1		24
#define AECP_AEM_COUNTER_CLOCK_DOMAIN_ENTITY_SPECIFIC_2		25
#define AECP_AEM_COUNTER_CLOCK_DOMAIN_ENTITY_SPECIFIC_3		26
#define AECP_AEM_COUNTER_CLOCK_DOMAIN_ENTITY_SPECIFIC_4		27
#define AECP_AEM_COUNTER_CLOCK_DOMAIN_ENTITY_SPECIFIC_5		28
#define AECP_AEM_COUNTER_CLOCK_DOMAIN_ENTITY_SPECIFIC_6		29
#define AECP_AEM_COUNTER_CLOCK_DOMAIN_ENTITY_SPECIFIC_7		30
#define AECP_AEM_COUNTER_CLOCK_DOMAIN_ENTITY_SPECIFIC_8		31

/**
 * \ingroup aem
 * From section 7.4.42 of IEEE 1722.1-2013.
 */
struct __attribute__ ((packed)) aecp_aem_get_counters_rsp_pdu {
	avb_u16 descriptor_type;	/**< ENTITY, AVB_INTERFACE, CLOCK_DOMAIN or STREAM_INPUT */
	avb_u16 descriptor_index;	/**< Index of the descriptor whose counters are fetched */
	avb_u32 counters_valid;
	avb_u32 counters_block[AECP_AEM_NB_COUNTERS];
};

/**
 * \ingroup aem
 * AECP message types, from table 9.1 of IEEE 1722.1-2013.
 */
typedef enum {
	AECP_AEM_COMMAND		= 0x00,		/**< AECP message type, AEM command. */
	AECP_AEM_RESPONSE		= 0x01,		/**< AECP message type, AEM response. */
	AECP_ADDRESS_ACCESS_COMMAND	= 0x02,
	AECP_ADDRESS_ACCESS_RESPONSE	= 0x03,
	AECP_AVC_COMMAND		= 0x04,
	AECP_AVC_RESPONSE		= 0x05,
	AECP_VENDOR_UNIQUE_COMMAND	= 0x06,
	AECP_VENDOR_UNIQUE_RESPONSE	= 0x07,
	AECP_HDCP_APM_COMMAND		= 0x08,
	AECP_HDCP_APM_RESPONSE		= 0x09,
	AECP_EXTENDED_COMMAND		= 0x0E,
	AECP_EXTENDED_RESPONSE		= 0x0F
} aecp_message_type_t;

/**
 * \ingroup aem
 * AECP AEM commands, from table 7.125 of IEEE 1722.1-2013.
 */
typedef enum {
	AECP_AEM_CMD_ACQUIRE_ENTITY			= 0x0000,	/**< ACQUIRE_ENTITY command/response */
	AECP_AEM_CMD_LOCK_ENTITY			= 0x0001,
	AECP_AEM_CMD_ENTITY_AVAILABLE			= 0x0002,
	AECP_AEM_CMD_CONTROLLER_AVAILABLE		= 0x0003,
	AECP_AEM_CMD_READ_DESCRIPTOR			= 0x0004,	/**< READ_DESCRIPTOR command/response */
	AECP_AEM_CMD_WRITE_DESCRIPTOR			= 0x0005,
	AECP_AEM_CMD_SET_CONFIGURATION			= 0x0006,
	AECP_AEM_CMD_GET_CONFIGURATION			= 0x0007,
	AECP_AEM_CMD_SET_STREAM_FORMAT			= 0x0008,
	AECP_AEM_CMD_GET_STREAM_FORMAT			= 0x0009,
	AECP_AEM_CMD_SET_VIDEO_FORMAT			= 0x000a,
	AECP_AEM_CMD_GET_VIDEO_FORMAT			= 0x000b,
	AECP_AEM_CMD_SET_SENSOR_FORMAT			= 0x000c,
	AECP_AEM_CMD_GET_SENSOR_FORMAT			= 0x000d,
	AECP_AEM_CMD_SET_STREAM_INFO			= 0x000e,
	AECP_AEM_CMD_GET_STREAM_INFO			= 0x000f,
	AECP_AEM_CMD_SET_NAME				= 0x0010,
	AECP_AEM_CMD_GET_NAME				= 0x0011,
	AECP_AEM_CMD_SET_ASSOCIATION_ID			= 0x0012,
	AECP_AEM_CMD_GET_ASSOCIATION_ID			= 0x0013,
	AECP_AEM_CMD_SET_SAMPLING_RATE			= 0x0014,
	AECP_AEM_CMD_GET_SAMPLING_RATE			= 0x0015,
	AECP_AEM_CMD_SET_CLOCK_SOURCE			= 0x0016,
	AECP_AEM_CMD_GET_CLOCK_SOURCE			= 0x0017,
	AECP_AEM_CMD_SET_CONTROL			= 0x0018,	/**< SET_CONTROL command/response */
	AECP_AEM_CMD_GET_CONTROL			= 0x0019,	/**< GET_CONTROL command/response */
	AECP_AEM_CMD_INCREMENT_CONTROL			= 0x001a,
	AECP_AEM_CMD_DECREMENT_CONTROL			= 0x001b,
	AECP_AEM_CMD_SET_SIGNAL_SELECTOR		= 0x001c,
	AECP_AEM_CMD_GET_SIGNAL_SELECTOR		= 0x001d,
	AECP_AEM_CMD_SET_MIXER				= 0x001e,
	AECP_AEM_CMD_GET_MIXER				= 0x001f,
	AECP_AEM_CMD_SET_MATRIX				= 0x0020,
	AECP_AEM_CMD_GET_MATRIX				= 0x0021,
	AECP_AEM_CMD_START_STREAMING			= 0x0022,	/**< START_STREAMING command/response */
	AECP_AEM_CMD_STOP_STREAMING			= 0x0023,	/**< STOP_STREAMING command/response */
	AECP_AEM_CMD_REGISTER_UNSOLICITED_NOTIFICATION	= 0x0024,	/**< REGISTER_UNSOLICITED_NOTIFICATION command/response */
	AECP_AEM_CMD_DEREGISTER_UNSOLICITED_NOTIFICATION = 0x0025,	/**< DEREGISTER_UNSOLICITED_NOTIFICATION command/response */
	AECP_AEM_CMD_IDENTIFY_NOTIFICATION		= 0x0026,
	AECP_AEM_CMD_GET_AVB_INFO			= 0x0027,
	AECP_AEM_CMD_GET_AS_PATH			= 0x0028,
	AECP_AEM_CMD_GET_COUNTERS			= 0x0029,
	AECP_AEM_CMD_REBOOT				= 0x002a,
	AECP_AEM_CMD_GET_AUDIO_MAP			= 0x002b,
	AECP_AEM_CMD_ADD_AUDIO_MAPPINGS			= 0x002c,
	AECP_AEM_CMD_REMOVE_AUDIO_MAPPINGS		= 0x002d,
	AECP_AEM_CMD_GET_VIDEO_MAP			= 0x002e,
	AECP_AEM_CMD_ADD_VIDEO_MAPPINGS			= 0x002f,
	AECP_AEM_CMD_REMOVE_VIDEO_MAPPINGS		= 0x0030,
	AECP_AEM_CMD_GET_SENSOR_MAP			= 0x0031,
	AECP_AEM_CMD_ADD_SENSOR_MAPPINGS		= 0x0032,
	AECP_AEM_CMD_REMOVE_SENSOR_MAPPINGS		= 0x0033,
	AECP_AEM_CMD_START_OPERATION			= 0x0034,
	AECP_AEM_CMD_ABORT_OPERATION			= 0x0035,
	AECP_AEM_CMD_OPERATION_STATUS			= 0x0036,
	AECP_AEM_CMD_AUTH_ADD_KEY			= 0x0037,
	AECP_AEM_CMD_AUTH_DELETE_KEY			= 0x0038,
	AECP_AEM_CMD_AUTH_GET_KEY_LIST			= 0x0039,
	AECP_AEM_CMD_AUTH_GET_KEY			= 0x003a,
	AECP_AEM_CMD_AUTH_ADD_KEY_TO_CHAIN		= 0x003b,
	AECP_AEM_CMD_AUTH_DELETE_KEY_FROM_CHAIN		= 0x003c,
	AECP_AEM_CMD_AUTH_GET_KEYCHAIN_LIST		= 0x003d,
	AECP_AEM_CMD_AUTH_GET_IDENTITY			= 0x003e,
	AECP_AEM_CMD_AUTH_ADD_TOKEN			= 0x003f,
	AECP_AEM_CMD_AUTH_DELETE_TOKEN			= 0x0040,
	AECP_AEM_CMD_AUTHENTICATE			= 0x0041,
	AECP_AEM_CMD_DEAUTHENTICATE			= 0x0042,
	AECP_AEM_CMD_ENABLE_TRANSPORT_SECURITY		= 0x0043,
	AECP_AEM_CMD_DISABLE_TRANSPORT_SECURITY		= 0x0044,
	AECP_AEM_CMD_ENABLE_STREAM_ENCRYPTION		= 0x0045,
	AECP_AEM_CMD_DISABLE_STREAM_ENCRYPTION		= 0x0046,
	AECP_AEM_CMD_SET_MEMORY_OBJECT_LENGTH		= 0x0047,
	AECP_AEM_CMD_GET_MEMORY_OBJECT_LENGTH		= 0x0048,
	AECP_AEM_CMD_SET_STREAM_BACKUP			= 0x0049,
	AECP_AEM_CMD_GET_STREAM_BACKUP			= 0x004a,
	AECP_AEM_CMD_EXPANSION				= 0x7fff
} aecp_aem_command_type_t;

/**
 * \ingroup aem
 * AECP and AECP AEM status codes, from section 9.2.11.6 and table 7.126 of IEEE 1722.1-2013.
 */
typedef enum {
	AECP_SUCCESS			= 0,  /**< The AVDECC Entity successfully performed the command and has valid results. */
	AECP_AEM_SUCCESS		= 0,  /**< The AVDECC Entity successfully performed the command and has valid results. */
	AECP_NOT_IMPLEMENTED		= 1,  /**< The AVDECC Entity does not support the command type. */
	AECP_AEM_NOT_IMPLEMENTED	= 1,  /**< The AVDECC Entity does not support the command type. */
	AECP_AEM_NO_SUCH_DESCRIPTOR	= 2,  /**< A descriptor with the descriptor_type and descriptor_index specified does not exist. */
	AECP_AEM_ENTITY_LOCKED		= 3,  /**< The AVDECC Entity has been locked by another AVDECC Controller. */
	AECP_AEM_ENTITY_ACQUIRED	= 4,  /**< The AVDECC Entity has been acquired by another AVDECC Controller. */
	AECP_AEM_NOT_AUTHENTICATED	= 5,  /**< The AVDECC Controller is not authenticated with the AVDECC Entity. */
	AECP_AEM_AUTHENTICATION_DISABLED = 6,  /**< The AVDECC Controller is trying to use an authentication command when authentication isn’t enable on the AVDECC Entity. */
	AECP_AEM_BAD_ARGUMENTS		= 7,  /**< One or more of the values in the fields of the frame were deemed to be bad by the AVDECC Entity (unsupported, incorrect combination, etc.). */
	AECP_AEM_NO_RESOURCES		= 8,  /**< The AVDECC Entity cannot complete the command because it does not have the resources to support it. */
	AECP_AEM_IN_PROGRESS		= 9,  /**< The AVDECC Entity is processing the command and will send a second response at a later time with the result of the command. */
	AECP_AEM_ENTITY_MISBEHAVING	= 10, /**< The AVDECC Entity is generated an internal error while trying to process the command. */
	AECP_AEM_NOT_SUPPORTED		= 11, /**< The command is implemented but the target of the command is not supported. For example trying to set the value of a read-only Control. */
	AECP_AEM_STREAM_IS_RUNNING	= 12, /**< The Stream is currently streaming and the command is one which cannot be executed on an Active Stream. */
	AECP_AEM_TIMEOUT		= 13 /**< Added for communication with GenAVB controller application, to signal a timeout waiting for a response from the target entity */
} aecp_status_t;

#endif /* _GENAVB_PUBLIC_AECP_H_ */

