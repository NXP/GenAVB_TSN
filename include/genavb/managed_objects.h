 /*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief GenAVB/TSN public API
 @details Managed objects API definition for the GenAVB/TSN library
*/

#ifndef _GENAVB_PUBLIC_MANAGED_OBJECTS_H_
#define _GENAVB_PUBLIC_MANAGED_OBJECTS_H_

#include "types.h"
#include "ptp.h"

/*
 * \defgroup managed-objects	IEEE 802.1AS Managed Objects
 * \ingroup library
 * 
 * Managed object tree definition, IEEE 802.1AS-2011, section 14
 * YANG module ieee1588-ptp-tt augmented by ieee802-dot1as-gptp
 */

/**
* \ingroup managed-objects
* gPTP node top level container enumerations
*/
typedef enum {
	GPTP_NODE_INSTANCE_LIST = 0,
	GPTP_NODE_COMMON_SERVICES,
	GPTP_NODE_MAX
} gptp_top_node_t;

/**
* \ingroup managed-objects
* Instance's container enumerations 
*/
typedef enum {
	GPTP_INSTANCE_INSTANCE_INDEX = 0,
	GPTP_INSTANCE_DEFAULT_DS,
	GPTP_INSTANCE_CURRENT_DS,
	GPTP_INSTANCE_PARENT_DS,
	GPTP_INSTANCE_TIME_PROPERTIES_DS,
	GPTP_INSTANCE_PORT_DS,
	GPTP_INSTANCE_PORT_STATS_DS,
	GPTP_INSTANCE_MAX
} gptp_instance_container_t;

/**
* \ingroup managed-objects
* Common Services's container enumerations 
*/
typedef enum {
	GPTP_COMMON_SERVICES_CMLDS = 0,
	GPTP_COMMON_SERVICES_MAX
} gptp_common_services_container_type_t;

/**
* \ingroup managed-objects
* Default Parameter Data Set container LEAFs enumerations 
*/
typedef enum {
	GPTP_DEFAULT_DS_CLOCK_IDENTITY = 0,
	GPTP_DEFAULT_DS_NUMBER_PORTS,
	GPTP_DEFAULT_DS_CLOCK_CLASS,
	GPTP_DEFAULT_DS_CLOCK_ACCURACY,
	GPTP_DEFAULT_DS_OFFSET_SCALED_LOG_VARIANCE,
	GPTP_DEFAULT_DS_PRIORITY1,
	GPTP_DEFAULT_DS_PRIORITY2,
	GPTP_DEFAULT_DS_GM_CAPABLE,
	GPTP_DEFAULT_DS_CURRENT_UTC_OFFSET,
	GPTP_DEFAULT_DS_CURRENT_UTC_OFFSET_VALID,
	GPTP_DEFAULT_DS_LEAP59,
	GPTP_DEFAULT_DS_LEAP61,
	GPTP_DEFAULT_DS_TIME_TRACEABLE,
	GPTP_DEFAULT_DS_FREQUENCY_TRACEABLE,
	GPTP_DEFAULT_DS_TIME_SOURCE,
	GPTP_DEFAULT_DS_MAX
} gptp_default_ds_leaf_t;

/**
* \ingroup managed-objects
* Current Parameter Data Set container LEAFs enumerations
*/
typedef enum {
	GPTP_CURRENT_DS_STEPS_REMOVED = 0,
	GPTP_CURRENT_DS_OFFSET_FROM_MASTER,
	GPTP_CURRENT_DS_LAST_GM_PHASE_CHANGE,
	GPTP_CURRENT_DS_LAST_GM_FREQ_CHANGE,
	GPTP_CURRENT_DS_GM_TIMEBASE_INDICATOR,
	GPTP_CURRENT_DS_GM_CHANGE_COUNT,
	GPTP_CURRENT_DS_TIME_OF_LAST_GM_CHANGE_EVENT,
	GPTP_CURRENT_DS_TIME_OF_LAST_GM_PHASE_CHANGE_EVENT,
	GPTP_CURRENT_DS_TIME_OF_LAST_GM_FREQ_CHANGE_EVENT,
	GPTP_CURRENT_DS_MAX
} gptp_current_parameter_leaf_t;

/**
* \ingroup managed-objects
* Parent Parameter Data Set container LEAFs enumerations 
*/
typedef enum {
	GPTP_PARENT_DS_PORT_IDENTITY = 0,
	GPTP_PARENT_DS_CUMULATIVE_RATE_RATIO,
	GPTP_PARENT_DS_GRAND_MASTER_IDENTITY,
	GPTP_PARENT_DS_GRAND_MASTER_CLOCK_CLASS,
	GPTP_PARENT_DS_GRAND_MASTER_CLOCK_ACCURACY,
	GPTP_PARENT_DS_GRAND_MASTER_OFFSET_SCALED_LOG_VARIANCE,
	GPTP_PARENT_DS_GRAND_MASTER_PRIORITY1,
	GPTP_PARENT_DS_GRAND_MASTER_PRIORITY2,
	GPTP_PARENT_DS_MAX
} gptp_parent_ds_leaf_t;

/**
* \ingroup managed-objects
* Time Properties Parameter Data Set container LEAFs enumerations
*/
typedef enum {
	GPTP_TIME_PROPERTIES_DS_CURRENT_UTC_OFFSET = 0,
	GPTP_TIME_PROPERTIES_DS_CURRENT_UTC_OFFSET_VALID,
	GPTP_TIME_PROPERTIES_DS_LEAP59,
	GPTP_TIME_PROPERTIES_DS_LEAP61,
	GPTP_TIME_PROPERTIES_DS_TIME_TRACEABLE,
	GPTP_TIME_PROPERTIES_DS_FREQUENCY_TRACEABLE,
	GPTP_TIME_PROPERTIES_DS_TIME_SOURCE,
	GPTP_TIME_PROPERTIES_DS_MAX
} gptp_time_properties_ds_leaf_t;

/**
* \ingroup managed-objects
* Port Parameter Data Set container LEAFs enumerations
*/
typedef enum {
	GPTP_PORT_DS_PORT_ID = 0,
	GPTP_PORT_DS_PORT_IDENTITY,
	GPTP_PORT_DS_PORT_ROLE,
	GPTP_PORT_DS_PTP_PORT_ENABLED,
	GPTP_PORT_DS_IS_MEASURING_DELAY,
	GPTP_PORT_DS_AS_CAPABLE,
	GPTP_PORT_DS_NEIGHBOR_PROP_DELAY,
	GPTP_PORT_DS_NEIGHBOR_PROP_DELAY_THRESH,
	GPTP_PORT_DS_DELAY_ASYMMETRY,
	GPTP_PORT_DS_NEIGHBOR_RATE_RATIO,
	GPTP_PORT_DS_INITIAL_LOG_ANNOUNCE_INTERVAL,
	GPTP_PORT_DS_CURRENT_LOG_ANNOUNCE_INTERVAL,
	GPTP_PORT_DS_ANNOUNCE_RECEIPT_TIMEOUT,
	GPTP_PORT_DS_INITIAL_LOG_SYNC_INTERVAL,
	GPTP_PORT_DS_CURRENT_LOG_SYNC_INTERVAL,
	GPTP_PORT_DS_SYNC_RECEIPT_TIMEOUT,
	GPTP_PORT_DS_SYNC_RECEIPT_TIMEOUT_TIME_INTERVAL,
	GPTP_PORT_DS_INITIAL_LOG_PDELAY_REQ_INTERVAL,
	GPTP_PORT_DS_CURRENT_LOG_PDELAY_REQ_INTERVAL,
	GPTP_PORT_DS_ALLOWED_LOST_RESPONSES,
	GPTP_PORT_DS_ALLOWED_FAULTS,
	GPTP_PORT_DS_VERSION_NUMBER,
	GPTP_PORT_DS_MAX
} gptp_port_ds_leaf_t;

/**
* \ingroup managed-objects
* Port Parameter Statistics container LEAFs enumerations
*/
typedef enum {
	GPTP_PORT_STATS_DS_PORT_ID = 0,
	GPTP_PORT_STATS_DS_RX_SYNC_COUNT,
	GPTP_PORT_STATS_DS_RX_FOLLOW_UP_COUNT,
	GPTP_PORT_STATS_DS_RX_PDELAY_REQUEST_COUNT,
	GPTP_PORT_STATS_DS_RX_PDELAY_RESPONSE_COUNT,
	GPTP_PORT_STATS_DS_RX_PDELAY_RESPONSE_FOLLOW_UP_COUNT,
	GPTP_PORT_STATS_DS_RX_ANNOUNCE_COUNT,
	GPTP_PORT_STATS_DS_RX_PTP_PACKET_DISCARD_COUNT,
	GPTP_PORT_STATS_DS_SYNC_RECEIPT_TIMEOUT_COUNT,
	GPTP_PORT_STATS_DS_ANNOUNCE_RECEIPT_TIMEOUT_COUNT,
	GPTP_PORT_STATS_DS_PDELAY_ALLOWED_LOST_RESPONSES_EXCEEDED_COUNT,
	GPTP_PORT_STATS_DS_TX_SYNC_COUNT,
	GPTP_PORT_STATS_DS_TX_FOLLOW_UP_COUNT,
	GPTP_PORT_STATS_DS_TX_PDELAY_REQUEST_COUNT,
	GPTP_PORT_STATS_DS_TX_PDELAY_RESPONSE_COUNT,
	GPTP_PORT_STATS_DS_TX_PDELAY_RESPONSE_FOLLOW_UP_COUNT,
	GPTP_PORT_STATS_DS_TX_ANNOUNCE_COUNT,
	GPTP_PORT_STATS_DS_MAX
} gptp_port_statistics_ds_leaf_t;

struct  __attribute__((packed)) genavb_mobj_cmd_node_header {
	uint16_t id;
	uint16_t length;
};

#define STACK_DEPTH	16

#define STATUS_OK	0

/**
 * defgroup managed-objects
 * \brief Managed object message structure for building and parsing gPTP messages
 * 
 * This structure provides a stateful interface for constructing and parsing
 * hierarchical managed object messages according to IEEE 802.1AS specifications.
 * It maintains a stack-based approach to handle nested container nodes and
 * tracks the current position within the message buffer.
 * 
 * The structure supports building messages by starting container nodes,
 * adding leaf values, and properly closing containers. It also supports
 * parsing received messages by navigating through the node hierarchy.
 * 
 * \note Must be initialized with genavb_mobj_cmd_init() before use
 * \note The provided buffer must remain valid throughout the structure's lifetime
 * \note Maximum nesting depth is limited by STACK_DEPTH
 */
struct genavb_mobj_cmd {
	uint8_t *buf; /* start of message buffer, fixed */
	uint8_t *end; /* end of message buffer, fixed */

	struct genavb_mobj_cmd_node_header *stack[STACK_DEPTH];
	struct genavb_mobj_cmd_node_header *cur; /* current node in stack */

	uint16_t sp; /* stack write index*/
	uint16_t top; /* buf write index */
};

/** 
 * \ingroup managed-objects
 * \brief Initialize a managed message structure
 * 
 * Initializes a genavb_mobj_cmd structure with the provided buffer for building
 * or parsing managed object messages. Sets up the buffer boundaries and
 * resets all internal state.

 * \param cmd    Pointer to the genavb_mobj_cmd structure to initialize
 * \param buf    Pointer to the message buffer to use
 * \param length Size of the message buffer in bytes
 *
 * \return 0 on success, negative error code on failure
 */
int genavb_mobj_cmd_init(struct genavb_mobj_cmd *cmd, uint8_t *buf, unsigned int length);

/** 
 * \ingroup managed-objects
 * \brief Start a new node in the managed object tree
 * 
 * Creates a new container node in the managed object hierarchy and pushes it
 * onto the internal stack. The node will contain child nodes or leaf values
 * added after this call until genavb_mobj_cmd_end_node() is called.
 *
 * \param cmd Pointer to the genavb_mobj_cmd structure
 * \param id  Node identifier from the appropriate enumeration
 * 
 * \return 0 on success, negative error on failure
 */
int genavb_mobj_cmd_start_node(struct genavb_mobj_cmd *cmd, uint16_t id);

/** 
 * \ingroup managed-objects
 * \brief End the current node in the managed object tree
 * 
 * Finalizes the current container node by updating its length field and
 * popping it from the internal stack. This completes the node started by
 * the most recent genavb_mobj_cmd_start_node() call.
 * 
 * \param cmd Pointer to the genavb_mobj_cmd structure
 * 
 * \return 0 on success, negative error on failure
 */
int genavb_mobj_cmd_end_node(struct genavb_mobj_cmd *cmd);

/**
 * \ingroup managed-objects
 * \brief Get the total length of a completed managed message
 *
 * Returns the total length of the managed object message in bytes.
 * This function should only be called after all container nodes have
 * been properly closed with genavb_mobj_cmd_end_node() to ensure
 * the message is complete.
 *
 * \param cmd Pointer to the managed message structure
 *
 * \return Total message length in bytes on success, negative error on failure
 */
int genavb_mobj_cmd_len(struct genavb_mobj_cmd *cmd);

/**
 * \ingroup managed-objects
 * \brief Return dataa buffer associated to the managed message
 *
 * Returns the total length of the managed object message in bytes.
 * This function should only be called after all container nodes have
 * been properly closed with genavb_mobj_cmd_end_node() to ensure
 * the message is complete.
 *
 * \param cmd Pointer to the managed message structure
 *
 * \return Total message length in bytes on success, negative error on failure
 */
uint8_t * genavb_mobj_cmd_buf(struct genavb_mobj_cmd *cmd);

/** 
 * \ingroup managed-objects
 * \brief Add an index to the current container
 * 
 * Creates a leaf node with the specified ID and index value within the current
 * container node. This is typically used for list entries or indexed elements
 * in the managed object tree.
 * 
 * \param cmd Pointer to the genavb_mobj_cmd structure
 * \param id  Node identifier
 * \param index The index value to store in the node
 * 
 * \return 0 on success, negative error on failure
 */
int genavb_mobj_cmd_set_list_index(struct genavb_mobj_cmd *cmd, uint16_t id, uint16_t index);

/** 
 * \ingroup managed-objects
 * \brief  Retrieve a leaf from the managed object tree
 * 
 * Searches for and retrieves a leaf with the specified ID from the current
 * position in the managed object tree. This is used when parsing received
 * managed object messages.
 * 
 * \param cmd Pointer to the genavb_mobj_cmd structure
 * \param id  leaf identifier to search for
 * 
 * \return 0 on success, negative error on failure
 */
int genavb_mobj_cmd_get_leaf(struct genavb_mobj_cmd *cmd, uint16_t id);

/**
 * \ingroup managed-objects
 * \brief  Set value in a leaf node with the specified size
 *
 * Creates a leaf node of the specified size with the specified ID.
  *
 * \param cmd   Pointer to the genavb_mobj_cmd structure
 * \param id    Node identifier for the leaf node
 * \param value pointer to the value to store
 * \param size  size in bytes of the stored value
 *
 * \return 0 on success, negative error on failure
 */
int genavb_mobj_cmd_set_leaf(struct genavb_mobj_cmd *cmd, uint16_t id, void *value, unsigned int size);

/** 
 * \ingroup managed-objects
 * \brief  Set an 8-bit unsigned integer value in a leaf node
 * 
 * Creates a leaf node with the specified ID containing an 8-bit unsigned
 * integer value. This is used for setting boolean flags, small counters,
 * or enumeration values in the managed object tree.
 * 
 * \param cmd   Pointer to the genavb_mobj_cmd structure
 * \param id    Node identifier for the leaf node
 * \param value 8-bit unsigned integer value to store
 * 
 * \return 0 on success, negative error on failure
 */
int genavb_mobj_cmd_set_leaf_u8(struct genavb_mobj_cmd *cmd, uint16_t id, uint8_t value);

/** 
 * \ingroup managed-objects
 * \brief  Set an 16-bit unsigned integer value in a leaf node
 * 
 * Creates a leaf node with the specified ID containing an 16-bit unsigned
 * integer value. This is used for setting boolean flags, small counters,
 * or enumeration values in the managed object tree.
 * 
 * \param cmd   Pointer to the genavb_mobj_cmd structure
 * \param id    Node identifier for the leaf node
 * \param value 16-bit unsigned integer value to store
 * 
 * \return 0 on success, negative error on failure
 */
int genavb_mobj_cmd_set_leaf_u16(struct genavb_mobj_cmd *cmd, uint16_t id, uint16_t value);

/** 
 * \ingroup managed-objects
 * \brief  Set an 32-bit unsigned integer value in a leaf node
 * 
 * Creates a leaf node with the specified ID containing an 32-bit unsigned
 * integer value. This is used for setting boolean flags, small counters,
 * or enumeration values in the managed object tree.
 * 
 * \param cmd   Pointer to the genavb_mobj_cmd structure
 * \param id    Node identifier for the leaf node
 * \param value 32-bit unsigned integer value to store
 * 
 * \return 0 on success, negative error on failure
 */
int genavb_mobj_cmd_set_leaf_u32(struct genavb_mobj_cmd *cmd, uint16_t id, uint32_t value);

/** 
 * \ingroup managed-objects
 * \brief  Set an 64-bit unsigned integer value in a leaf node
 * 
 * Creates a leaf node with the specified ID containing an 64-bit unsigned
 * integer value. This is used for setting boolean flags, small counters,
 * or enumeration values in the managed object tree.
 * 
 * \param cmd   Pointer to the genavb_mobj_cmd structure
 * \param id    Node identifier for the leaf node
 * \param value 64-bit unsigned integer value to store
 * 
 * \return 0 on success, negative error on failure
 */
int genavb_mobj_cmd_set_leaf_u64(struct genavb_mobj_cmd *cmd, uint16_t id, uint64_t value);

/**
 * \ingroup managed-objects
 * \brief  Set an 8-bit signed integer value in a leaf node
 *
 * Creates a leaf node with the specified ID containing an 8-bit signed
 * integer value. This is used for setting boolean flags, small counters,
 * or enumeration values in the managed object tree.
 *
 * \param cmd   Pointer to the genavb_mobj_cmd structure
 * \param id    Node identifier for the leaf node
 * \param value 8-bit signed integer value to store
 *
 * \return 0 on success, negative error on failure
 */
int genavb_mobj_cmd_set_leaf_s8(struct genavb_mobj_cmd *cmd, uint16_t id, int8_t value);

/**
 * \ingroup managed-objects
 * \brief  Set an 16-bit signed integer value in a leaf node
 *
 * Creates a leaf node with the specified ID containing an 16-bit signed
 * integer value. This is used for setting boolean flags, small counters,
 * or enumeration values in the managed object tree.
 *
 * \param cmd   Pointer to the genavb_mobj_cmd structure
 * \param id    Node identifier for the leaf node
 * \param value 16-bit signed integer value to store
 *
 * \return 0 on success, negative error on failure
 */
int genavb_mobj_cmd_set_leaf_s16(struct genavb_mobj_cmd *cmd, uint16_t id, int16_t value);

/**
 * \ingroup managed-objects
 * \brief  Set an 32-bit signed integer value in a leaf node
 *
 * Creates a leaf node with the specified ID containing an 32-bit signed
 * integer value. This is used for setting boolean flags, small counters,
 * or enumeration values in the managed object tree.
 *
 * \param cmd   Pointer to the genavb_mobj_cmd structure
 * \param id    Node identifier for the leaf node
 * \param value 32-bit signed integer value to store
 *
 * \return 0 on success, negative error on failure
 */
int genavb_mobj_cmd_set_leaf_s32(struct genavb_mobj_cmd *cmd, uint16_t id, int32_t value);

/**
 * \ingroup managed-objects
 * \brief  Set an 64-bit signed integer value in a leaf node
 *
 * Creates a leaf node with the specified ID containing an 64-bit signed
 * integer value. This is used for setting boolean flags, small counters,
 * or enumeration values in the managed object tree.
 *
 * \param cmd   Pointer to the genavb_mobj_cmd structure
 * \param id    Node identifier for the leaf node
 * \param value 64-bit signed integer value to store
 *
 * \return 0 on success, negative error on failure
 */
int genavb_mobj_cmd_set_leaf_s64(struct genavb_mobj_cmd *cmd, uint16_t id, int64_t value);

/**
 * \ingroup managed-objects
 * \brief  Set a ptp_scaled_ns value in a leaf node
 *
 * Creates a leaf node with the specified ID containing a ptp_scaled_ns value.
 *
 * \param cmd   Pointer to the genavb_mobj_cmd structure
 * \param id    Node identifier for the leaf node
 * \param value pointer to the ptp_scaled_ns value to store
 *
 * \return 0 on success, negative error on failure
 */
int genavb_mobj_cmd_set_leaf_scaled_ns(struct genavb_mobj_cmd *cmd, uint16_t id, struct ptp_scaled_ns *value);

/**
 * \ingroup managed-objects
 * \brief  Set a ptp_u_scaled_ns value in a leaf node
 *
 * Creates a leaf node with the specified ID containing a ptp_u_scaled_ns value.
 *
 * \param cmd   Pointer to the genavb_mobj_cmd structure
 * \param id    Node identifier for the leaf node
 * \param value pointer to the ptp_u_scaled_ns value to store
 *
 * \return 0 on success, negative error on failure
 */
int genavb_mobj_cmd_set_leaf_uscaled_ns(struct genavb_mobj_cmd *cmd, uint16_t id, struct ptp_u_scaled_ns *value);

/**
 * \ingroup managed-objects
 * \brief  Set a ptp_clock_identity value in a leaf node
 *
 * Creates a leaf node with the specified ID containing a ptp_clock_identity
 * integer value.
 *
 * \param cmd   Pointer to the genavb_mobj_cmd structure
 * \param id    Node identifier for the leaf node
 * \param value pointer to the ptp_clock_identity value to store
 *
 * \return 0 on success, negative error on failure
 */
int genavb_mobj_cmd_set_leaf_port_identity(struct genavb_mobj_cmd *cmd, uint16_t id, struct ptp_port_identity *value);

/**
 * \ingroup managed-objects
 * \brief  Set a ptp_clock_identity value in a leaf node
 *
 * Creates a leaf node with the specified ID containing a ptp_clock_identity
 * integer value.
 *
 * \param cmd   Pointer to the genavb_mobj_cmd structure
 * \param id    Node identifier for the leaf node
 * \param value pointer tot the ptp_clock_identity value to store
 *
 * \return 0 on success, negative error on failure
 */
int genavb_mobj_cmd_set_leaf_clock_identity(struct genavb_mobj_cmd *cmd, uint16_t id, struct ptp_clock_identity *value);

/**
 * \ingroup managed-objects
 * \brief  Set a mac adress value in a leaf node
 *
 * Creates a leaf node with the specified ID containing a mac adress
 *
 * \param cmd   Pointer to the genavb_mobj_cmd structure
 * \param id    Node identifier for the leaf node
 * \param value pointer to the mac address value to store
 *
 * \return 0 on success, negative error on failure
 */
int genavb_mobj_cmd_set_leaf_mac_address(struct genavb_mobj_cmd *cmd, uint16_t id, uint8_t *value);

/**
 * \ingroup managed-objects
 * \brief  Set a double value in a leaf node
 *
 * Creates a leaf node with the specified ID containing a double value.
 *
 * \param cmd   Pointer to the genavb_mobj_cmd structure
 * \param id    Node identifier for the leaf node
 * \param value double value to store
 *
 * \return 0 on success, negative error on failure
 */
int genavb_mobj_cmd_set_leaf_double(struct genavb_mobj_cmd *cmd, uint16_t id, double value);

/**
 * \ingroup managed-objects
 * \brief  Set a bool value in a leaf node
 *
 * Creates a leaf node with the specified ID containing a bool value.
 *
 * \param cmd   Pointer to the genavb_mobj_cmd structure
 * \param id    Node identifier for the leaf node
 * \param value bool value to store
 *
 * \return 0 on success, negative error on failure
 */
int genavb_mobj_cmd_set_leaf_bool(struct genavb_mobj_cmd *cmd, uint16_t id, bool value);

/**
 * \ingroup managed-objects
 * \brief  Check managed objects response validity
 *
 * Check status and global length of the managed object response
 *
 * \param rsp   Pointer to the response buffer
 *
 * \return 0 on success, negative error on failure
 */
int genavb_mobj_rsp_check(uint8_t *rsp);

#endif /* _GENAVB_PUBLIC_MANAGED_OBJECTS_H_ */
