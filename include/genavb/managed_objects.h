 /*
 * Copyright 2025-2026 NXP
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

struct  __attribute__((packed)) genavb_mobj_cmd_node_header {
	uint16_t id;
	uint16_t length;
};

#define GENAVB_MOBJ_STACK_DEPTH	16

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
 * \note Maximum nesting depth is limited by GENAVB_MOBJ_STACK_DEPTH
 */

struct genavb_mobj_node_stack {
    struct genavb_mobj_cmd_node_header *hdr;
    const struct genavb_mobj_node_descriptor *desc;
};

struct genavb_mobj_cmd {
	uint8_t *buf; /* start of message buffer, fixed */
	uint8_t *end; /* end of message buffer, fixed */

	struct genavb_mobj_node_stack stack[GENAVB_MOBJ_STACK_DEPTH];
	struct genavb_mobj_node_stack *cur; /* current node in stack */

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
 * \param module Yang module name related to the message structure (e.g. "ptp")
 * \param buf    Pointer to the message buffer to use
 * \param length Size of the message buffer in bytes
 *
 * \return 0 on success, negative error code on failure
 */
int genavb_mobj_cmd_init(struct genavb_mobj_cmd *cmd, const char *module, uint8_t *buf, unsigned int length);

/**
 * \ingroup managed_objects
 * \brief Start a new node in the managed object command
 *
 * Creates a new container node in the managed object hierarchy and pushes it
 * onto the internal stack. The node will contain child nodes or leaf values
 * added after this call until genavb_mobj_cmd_end_node() is called.
 *
 * \return	0 on success, negative value on failure
 * \param cmd			Pointer to managed object command structure
 * \param node_name		Node's name
 */
int genavb_mobj_cmd_start_node(struct genavb_mobj_cmd *cmd, const char *node_name);

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
 * \brief Return data buffer associated to the managed message
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
 * \ingroup managed_objects
 * \brief Set list index
 *
 * Creates a list node with the specified name and index value within the current
 * container node. This is typically used for list entries or indexed elements
 * in the managed object tree.
 *
 * \param cmd			Pointer to managed object command structure
 * \param list_name		String name of the list
 * \param index			Index value to set
 *
 * \return	0 on success, negative value on failure
 */
int genavb_mobj_cmd_set_list_index(struct genavb_mobj_cmd *cmd, const char *list_name, uint16_t index);

/**
 * \ingroup managed-objects
 * \brief  Retrieve a leaf
 *
 * Searches for and retrieves a leaf with the specified name from the current
 * position in the managed object tree.
 *
 * \param cmd 			Pointer to the genavb_mobj_cmd structure
 * \param leaf_name		String name of the leaf
 *
 * \return 0 on success, negative error on failure
 */
int genavb_mobj_cmd_get_leaf(struct genavb_mobj_cmd *cmd, const char *leaf_name);

/**
 * \ingroup managed-objects
 * \brief  Set value in a leaf node with the specified size
 *
 * Creates a leaf node of the specified size with the specified name.
 *
 * \param cmd			Pointer to managed object command structure
 * \param leaf_name		String name of the leaf
 * \param leaf_val		Pointer to leaf value data
 * \param size			Size of the leaf value data in bytes
 *
 * \return	0 on success, negative value on failure
 */
int genavb_mobj_cmd_set_leaf(struct genavb_mobj_cmd *cmd, const char *leaf_name, void *leaf_val, unsigned int size);

/**
 * \ingroup managed_objects
 * \brief Set 8-bit unsigned integer leaf value
 *
 * Creates a leaf node with the specified name containing an 8-bit unsigned
 * integer value. This is used for setting boolean flags, small counters,
 * or enumeration values in the managed object tree.
 *
 * \param cmd			Pointer to managed object command structure
 * \param leaf_name		String name of the leaf
 * \param leaf_val		8-bit unsigned integer value
 *
 * \return	0 on success, negative value on failure
 */
int genavb_mobj_cmd_set_leaf_u8(struct genavb_mobj_cmd *cmd, const char *leaf_name, uint8_t leaf_val);

/**
 * \ingroup managed_objects
 * \brief Set 16-bit unsigned integer leaf value
 *
 * Creates a leaf node with the specified name containing an 16-bit unsigned
 * integer value. This is used for setting boolean flags, small counters,
 * or enumeration values in the managed object tree.
 *
 * \param cmd			Pointer to managed object command structure
 * \param leaf_name		String name of the leaf
 * \param leaf_val		16-bit unsigned integer value
 *
 * \return	0 on success, negative value on failure
 */
int genavb_mobj_cmd_set_leaf_u16(struct genavb_mobj_cmd *cmd, const char *leaf_name, uint16_t leaf_val);

/**
 * \ingroup managed_objects
 * \brief Set 32-bit unsigned integer leaf value
 *
 * Creates a leaf node with the specified name containing an 32-bit unsigned
 * integer value. This is used for setting boolean flags, small counters,
 * or enumeration values in the managed object tree.
 *
 * \param cmd			Pointer to managed object command structure
 * \param leaf_name		String name of the leaf
 * \param leaf_val		32-bit unsigned integer value
 *
 * \return	0 on success, negative value on failure
 */
int genavb_mobj_cmd_set_leaf_u32(struct genavb_mobj_cmd *cmd, const char *leaf_name, uint32_t leaf_val);

/**
 * \ingroup managed_objects
 * \brief Set 64-bit unsigned integer leaf value
  *
 * Creates a leaf node with the specified name containing an 64-bit unsigned
 * integer value. This is used for setting boolean flags, small counters,
 * or enumeration values in the managed object tree.
 *
 * \param cmd			Pointer to managed object command structure
 * \param leaf_name		String name of the leaf
 * \param leaf_val		64-bit unsigned integer value
 *
 * \return	0 on success, negative value on failure
 */
int genavb_mobj_cmd_set_leaf_u64(struct genavb_mobj_cmd *cmd, const char *leaf_name, uint64_t leaf_val);

/**
 * \ingroup managed_objects
 * \brief Set 8-bit signed integer leaf value
 *
 * Creates a leaf node with the specified name containing an 8-bit signed
 * integer value. This is used for setting boolean flags, small counters,
 * or enumeration values in the managed object tree.
 *
 * \param cmd			Pointer to managed object command structure
 * \param leaf_name		String name of the leaf
 * \param leaf_val		8-bit signed integer value
 *
 * \return	0 on success, negative value on failure
 */
int genavb_mobj_cmd_set_leaf_s8(struct genavb_mobj_cmd *cmd, const char *leaf_name, int8_t leaf_val);

/**
 * \ingroup managed_objects
 * \brief Set 16-bit signed integer leaf value
 *
 * Creates a leaf node with the specified name containing an 16-bit signed
 * integer value. This is used for setting boolean flags, small counters,
 * or enumeration values in the managed object tree.
 *
 * \param cmd			Pointer to managed object command structure
 * \param leaf_name		String name of the leaf
 * \param leaf_val		16-bit signed integer value
 *
 * \return	0 on success, negative value on failure
 */
int genavb_mobj_cmd_set_leaf_s16(struct genavb_mobj_cmd *cmd, const char *leaf_name, int16_t leaf_val);

/**
 * \ingroup managed_objects
 * \brief Set 32-bit signed integer leaf value
 *
 * Creates a leaf node with the specified name containing an 32-bit signed
 * integer value. This is used for setting boolean flags, small counters,
 * or enumeration values in the managed object tree.
 *
 * \param cmd			Pointer to managed object command structure
 * \param leaf_name		String name of the leaf
 * \param leaf_val		32-bit signed integer value
 *
 * \return	0 on success, negative value on failure
 */
int genavb_mobj_cmd_set_leaf_s32(struct genavb_mobj_cmd *cmd, const char *leaf_name, int32_t leaf_val);

/**
 * \ingroup managed_objects
 * \brief Set 64-bit signed integer leaf value
 *
 * Creates a leaf node with the specified name containing an 64-bit signed
 * integer value. This is used for setting boolean flags, small counters,
 * or enumeration values in the managed object tree.
 *
 * \param cmd			Pointer to managed object command structure
 * \param leaf_name		String name of the leaf
 * \param leaf_val		64-bit signed integer value
 *
 * \return	0 on success, negative value on failure
 */
int genavb_mobj_cmd_set_leaf_s64(struct genavb_mobj_cmd *cmd, const char *leaf_name, int64_t leaf_val);

/**
 * \ingroup managed_objects
 * \brief Set PTP scaled nanoseconds leaf value
 *
 * Creates a leaf node with the specified name containing a ptp_scaled_ns value.
 *
 * \param cmd			Pointer to managed object command structure
 * \param leaf_name		String name of the leaf
 * \param leaf_val		Pointer to PTP scaled nanoseconds structure
 *
 * \return	0 on success, negative value on failure
 */
int genavb_mobj_cmd_set_leaf_scaled_ns(struct genavb_mobj_cmd *cmd, const char *leaf_name, struct ptp_scaled_ns *leaf_val);

/**
 * \ingroup managed_objects
 * \brief Set PTP unsigned scaled nanoseconds leaf value
 *
 * Creates a leaf node with the specified name containing a ptp_u_scaled_ns value.
 *
 * \param cmd			Pointer to managed object command structure
 * \param leaf_name		String name of the leaf
 * \param leaf_val		Pointer to PTP unsigned scaled nanoseconds structure
 *
 * \return	0 on success, negative value on failure
 */
int genavb_mobj_cmd_set_leaf_uscaled_ns(struct genavb_mobj_cmd *cmd, const char *leaf_name, struct ptp_u_scaled_ns *leaf_val);

/**
 * \ingroup managed_objects
 * \brief Set PTP port identity leaf value
 *
 * Creates a leaf node with the specified name containing a ptp_clock_identity
 * integer value.
 *
 * \param cmd			Pointer to managed object command structure
 * \param leaf_name		String name of the leaf
 * \param leaf_val		Pointer to PTP port identity structure
 *
 * \return	0 on success, negative value on failure
 */
int genavb_mobj_cmd_set_leaf_port_identity(struct genavb_mobj_cmd *cmd, const char *leaf_name, struct ptp_port_identity *leaf_val);

/**
 * \ingroup managed_objects
 * \brief Set PTP clock identity leaf value
 *
 * Creates a leaf node with the specified name containing a ptp_clock_identity
 * integer value.
 *
 * \param cmd			Pointer to managed object command structure
 * \param leaf_name		String name of the leaf
 * \param leaf_val		Pointer to PTP clock identity structure
 *
 * \return	0 on success, negative value on failure
 */
int genavb_mobj_cmd_set_leaf_clock_identity(struct genavb_mobj_cmd *cmd, const char *leaf_name, struct ptp_clock_identity *leaf_val);

/**
 * \ingroup managed_objects
 * \brief Set MAC address leaf value
 *
 * Creates a leaf node with the specified name containing a mac adress
 *
 * \param cmd			Pointer to managed object command structure
 * \param leaf_name		String name of the leaf
 * \param leaf_val		Pointer to 6-byte MAC address array
 *
 * \return	0 on success, negative value on failure
 */
int genavb_mobj_cmd_set_leaf_mac_address(struct genavb_mobj_cmd *cmd, const char *leaf_name, uint8_t *leaf_val);

/**
 * \ingroup managed_objects
 * \brief Set double precision floating point leaf value
 *
 * Creates a leaf node with the specified name containing a double value.
 *
 * \param cmd			Pointer to managed object command structure
 * \param leaf_name		String name of the leaf
 * \param leaf_val		Double precision floating point value
 *
 * \return	0 on success, negative value on failure
 */
int genavb_mobj_cmd_set_leaf_double(struct genavb_mobj_cmd *cmd, const char *leaf_name, double leaf_val);

/**
 * \ingroup managed_objects
 * \brief Set boolean leaf value
 *
 * Creates a leaf node with the specified name containing a bool value.
 *
 * \param cmd			Pointer to managed object command structure
 * \param leaf_name		String name of the leaf
 * \param leaf_val		Boolean value
 *
 * \return	0 on success, negative value on failure
 */
int genavb_mobj_cmd_set_leaf_bool(struct genavb_mobj_cmd *cmd, const char *leaf_name, bool leaf_val);

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

/**
 * @brief Structure defining a node in a managed object hierarchy
 */
struct genavb_mobj_node_descriptor {
	const char *self;
	const char **names;				/* Array of string names (this node's leaves names) */
	uint16_t max_id;						/* Maximum valid leaves ID */
	const struct genavb_mobj_node_descriptor *children;	/* Child nodes (nested containers) */
	uint16_t num_children;						/* Number of children */
};

#endif /* _GENAVB_PUBLIC_MANAGED_OBJECTS_H_ */
