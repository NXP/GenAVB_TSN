 /*
 * Copyright 2025-2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file managed_objects.c
 @brief Managed objects handling functions
 @details API definition for the GenAVB library
*/

#include "config.h"

#include <stdio.h>
#include <string.h>

#include <genavb/managed_objects.h>

extern const struct genavb_mobj_node_descriptor gptp_top_nodes_descriptor;

/**
 * @brief Find a child by name in a given node descriptor
 * @param descriptor Node descriptor containing names and metadata
 * @param name String name to decode
 * @return Node ID if found, -1 otherwise
 */
static int node_find_child_by_name(const struct genavb_mobj_node_descriptor *descriptor, const char *name)
{
	int i;

	if (!descriptor || !name)
		goto err;

	for (i = 0; i < descriptor->max_id; i++) {
		if (!strcmp(descriptor->names[i], name))
			return i;
	}

err:
	return -1;
}


static int cmd_check_headroom(struct genavb_mobj_cmd *cmd, uint16_t len)
{
	if ((cmd->top + len) > (cmd->end - cmd->buf))
		return -1;

	return 0;
}

int genavb_mobj_cmd_len(struct genavb_mobj_cmd *cmd)
{
	if (!cmd || (cmd->sp != 1))
		return -1;

	return cmd->top;
}

uint8_t *genavb_mobj_cmd_buf(struct genavb_mobj_cmd *cmd)
{
	if (!cmd)
		return NULL;

	return cmd->buf;
}

int genavb_mobj_cmd_init(struct genavb_mobj_cmd *cmd, const char *module_name, uint8_t *buf, unsigned int size)
{
	struct genavb_mobj_node_descriptor *descriptor;

	if (!strcmp("ptp", module_name))
		descriptor = (struct genavb_mobj_node_descriptor *)&gptp_top_nodes_descriptor;
	else
		return -1;

	if (!cmd || !buf)
		return -1;

	cmd->buf = buf;
	cmd->end = buf + size;

	memset(buf, 0, size);
	memset(cmd->stack, 0, GENAVB_MOBJ_STACK_DEPTH * sizeof(struct genavb_mobj_node_stack *));

	cmd->stack[0].desc = descriptor;

	cmd->sp = 1;
	cmd->cur = &cmd->stack[cmd->sp];
	cmd->top = 0;

	return 0;
}

static int _genavb_mobj_cmd_start_node(struct genavb_mobj_cmd *cmd, uint16_t node_id)
{
	if (!cmd)
		return -1;

	if ((!cmd->sp) || (cmd->sp > (GENAVB_MOBJ_STACK_DEPTH - 1)))
		return - 1;

	if (cmd_check_headroom(cmd, sizeof(struct genavb_mobj_cmd_node_header)) < 0)
		return -1;

	if ((node_id > cmd->stack[cmd->sp - 1].desc->max_id))
		return -1;

	cmd->stack[cmd->sp].hdr = (struct genavb_mobj_cmd_node_header *)&cmd->buf[cmd->top]; /* save node position */
	cmd->stack[cmd->sp].desc = &cmd->stack[cmd->sp - 1].desc->children[node_id];

	cmd->cur = &cmd->stack[cmd->sp];
	cmd->cur->hdr->id = node_id;
	cmd->cur->hdr->length = 0; /* placeholder for length */

	cmd->sp++;

	cmd->top += sizeof(struct genavb_mobj_cmd_node_header);

	return 0;
}

int genavb_mobj_cmd_start_node(struct genavb_mobj_cmd *cmd, const char *name)
{
	int id;

	if (cmd->sp == 0)
		return -1;

	if ((id = node_find_child_by_name(cmd->stack[cmd->sp - 1].desc, name)) < 0)
		return -1;

	return _genavb_mobj_cmd_start_node(cmd, id);
}

int genavb_mobj_cmd_end_node(struct genavb_mobj_cmd *cmd)
{
	struct genavb_mobj_cmd_node_header *parent_node;

	if (!cmd || cmd->sp <= 1)
		return -1;

	cmd->sp--;

	cmd->cur = &cmd->stack[cmd->sp];

	if (cmd->sp > 1) {
		parent_node = cmd->stack[cmd->sp - 1].hdr;

		if ((cmd->cur->hdr->length + sizeof(struct genavb_mobj_cmd_node_header)) > (UINT16_MAX - parent_node->length))
			return -1;

		parent_node->length += cmd->cur->hdr->length + sizeof(struct genavb_mobj_cmd_node_header);
	}

	return 0;
}

static inline int _genavb_managed_cmd_leaf_set(struct genavb_mobj_cmd *cmd, uint16_t leaf_id, void *val, unsigned int size)
{
	if (!cmd)
		return -1;

	if (cmd_check_headroom(cmd, sizeof(struct genavb_mobj_cmd_node_header) + size) < 0)
		return -1;

	cmd->buf[cmd->top] = leaf_id;
	cmd->buf[cmd->top + 2] = size;
	cmd->top += sizeof(struct genavb_mobj_cmd_node_header);

	if (size) {
		memcpy(&cmd->buf[cmd->top], val, size);
		cmd->top += size;
	}

	cmd->cur->hdr->length += sizeof(struct genavb_mobj_cmd_node_header) + size;

	return 0;
}

int genavb_mobj_cmd_get_leaf(struct genavb_mobj_cmd *cmd, const char *name)
{
	uint16_t val = 0;
	int id;

	if (cmd->sp == 0)
		return -1;

	if ((id = node_find_child_by_name(cmd->cur->desc, name)) < 0)
		return -1;

	return _genavb_managed_cmd_leaf_set(cmd, id, &val, 0);
}

int genavb_mobj_cmd_set_list_index(struct genavb_mobj_cmd *cmd, const char *name, uint16_t index)
{
	int id;

	if (cmd->sp == 0)
		return -1;

	if ((id = node_find_child_by_name(cmd->cur->desc, name)) < 0)
		return -1;

	return _genavb_managed_cmd_leaf_set(cmd, id, &index, sizeof(uint16_t));
}

int genavb_mobj_cmd_set_leaf(struct genavb_mobj_cmd *cmd, const char *name, void *leaf_val, unsigned int size)
{
	int id;

	if (cmd->sp == 0)
		return -1;

	if ((id = node_find_child_by_name(cmd->cur->desc, name)) < 0)
		return -1;

	return _genavb_managed_cmd_leaf_set(cmd, id, leaf_val, size);
}

int genavb_mobj_cmd_set_leaf_u8(struct genavb_mobj_cmd *cmd, const char *name, uint8_t leaf_val)
{
	int id;

	if (cmd->sp == 0)
		return -1;

	if ((id = node_find_child_by_name(cmd->cur->desc, name)) < 0)
		return -1;

	return _genavb_managed_cmd_leaf_set(cmd, id, &leaf_val, sizeof(uint8_t));
}

int genavb_mobj_cmd_set_leaf_u16(struct genavb_mobj_cmd *cmd, const char *name, uint16_t leaf_val)
{
	int id;

	if (cmd->sp == 0)
		return -1;

	if ((id = node_find_child_by_name(cmd->cur->desc, name)) < 0)
		return -1;

	return _genavb_managed_cmd_leaf_set(cmd, id, &leaf_val, sizeof(uint16_t));
}

int genavb_mobj_cmd_set_leaf_u32(struct genavb_mobj_cmd *cmd, const char *name, uint32_t leaf_val)
{
	int id;

	if (cmd->sp == 0)
		return -1;

	if ((id = node_find_child_by_name(cmd->cur->desc, name)) < 0)
		return -1;

	return _genavb_managed_cmd_leaf_set(cmd, id, &leaf_val, sizeof(uint32_t));
}

int genavb_mobj_cmd_set_leaf_u64(struct genavb_mobj_cmd *cmd, const char *name, uint64_t leaf_val)
{
	int id;

	if (cmd->sp == 0)
		return -1;

	if ((id = node_find_child_by_name(cmd->cur->desc, name)) < 0)
		return -1;

	return _genavb_managed_cmd_leaf_set(cmd, id, &leaf_val, sizeof(uint64_t));
}

int genavb_mobj_cmd_set_leaf_s8(struct genavb_mobj_cmd *cmd, const char *name, int8_t leaf_val)
{
	int id;

	if (cmd->sp == 0)
		return -1;

	if ((id = node_find_child_by_name(cmd->cur->desc, name)) < 0)
		return -1;

	return _genavb_managed_cmd_leaf_set(cmd, id, &leaf_val, sizeof(int8_t));
}

int genavb_mobj_cmd_set_leaf_s16(struct genavb_mobj_cmd *cmd, const char *name, int16_t leaf_val)
{
	int id;

	if (cmd->sp == 0)
		return -1;

	if ((id = node_find_child_by_name(cmd->cur->desc, name)) < 0)
		return -1;

	return _genavb_managed_cmd_leaf_set(cmd, id, &leaf_val, sizeof(int16_t));
}

int genavb_mobj_cmd_set_leaf_s32(struct genavb_mobj_cmd *cmd, const char *name, int32_t leaf_val)
{
	int id;

	if (cmd->sp == 0)
		return -1;

	if ((id = node_find_child_by_name(cmd->cur->desc, name)) < 0)
		return -1;

	return _genavb_managed_cmd_leaf_set(cmd, id, &leaf_val, sizeof(int32_t));
}

int genavb_mobj_cmd_set_leaf_s64(struct genavb_mobj_cmd *cmd, const char *name, int64_t leaf_val)
{
	int id;

	if (cmd->sp == 0)
		return -1;

	if ((id = node_find_child_by_name(cmd->cur->desc, name)) < 0)
		return -1;

	return _genavb_managed_cmd_leaf_set(cmd, id, &leaf_val, sizeof(int64_t));
}

int genavb_mobj_cmd_set_leaf_scaled_ns(struct genavb_mobj_cmd *cmd, const char *name, struct ptp_scaled_ns *leaf_val)
{
	int id;

	if (cmd->sp == 0)
		return -1;

	if ((id = node_find_child_by_name(cmd->cur->desc, name)) < 0)
		return -1;

	return _genavb_managed_cmd_leaf_set(cmd, id, leaf_val, sizeof(struct ptp_scaled_ns));
}

int genavb_mobj_cmd_set_leaf_uscaled_ns(struct genavb_mobj_cmd *cmd, const char *name, struct ptp_u_scaled_ns *leaf_val)
{
	int id;

	if (cmd->sp == 0)
		return -1;

	if ((id = node_find_child_by_name(cmd->cur->desc, name)) < 0)
		return -1;

	return _genavb_managed_cmd_leaf_set(cmd, id, leaf_val, sizeof(struct ptp_u_scaled_ns));
}

int genavb_mobj_cmd_set_leaf_port_identity(struct genavb_mobj_cmd *cmd, const char *name, struct ptp_port_identity *leaf_val)
{
	int id;

	if (cmd->sp == 0)
		return -1;

	if ((id = node_find_child_by_name(cmd->cur->desc, name)) < 0)
		return -1;

	return _genavb_managed_cmd_leaf_set(cmd, id, leaf_val, sizeof(struct ptp_port_identity));
}

int genavb_mobj_cmd_set_leaf_clock_identity(struct genavb_mobj_cmd *cmd, const char *name, struct ptp_clock_identity *leaf_val)
{
	int id;

	if (cmd->sp == 0)
		return -1;

	if ((id = node_find_child_by_name(cmd->cur->desc, name)) < 0)
		return -1;

	return _genavb_managed_cmd_leaf_set(cmd, id, leaf_val, sizeof(struct ptp_clock_identity));
}

int genavb_mobj_cmd_set_leaf_mac_address(struct genavb_mobj_cmd *cmd, const char *name, uint8_t *leaf_val)
{
	int id;

	if (cmd->sp == 0)
		return -1;

	if ((id = node_find_child_by_name(cmd->cur->desc, name)) < 0)
		return -1;

	return _genavb_managed_cmd_leaf_set(cmd, id, leaf_val, sizeof(uint8_t)*6);
}

int genavb_mobj_cmd_set_leaf_double(struct genavb_mobj_cmd *cmd, const char *name, double leaf_val)
{
	int id;

	if (cmd->sp == 0)
		return -1;

	if ((id = node_find_child_by_name(cmd->cur->desc, name)) < 0)
		return -1;

	return _genavb_managed_cmd_leaf_set(cmd, id, &leaf_val, sizeof(double));
}

int genavb_mobj_cmd_set_leaf_bool(struct genavb_mobj_cmd *cmd, const char *name, bool leaf_val)
{
	int id;

	if (cmd->sp == 0)
		return -1;

	if ((id = node_find_child_by_name(cmd->cur->desc, name)) < 0)
		return -1;

	return _genavb_managed_cmd_leaf_set(cmd, id, &leaf_val, sizeof(uint8_t));
}

static uint8_t *genavb_mobj_get_node_header(uint8_t *buf, uint16_t *id, uint16_t *length, uint16_t *status)
{
	*id = ((uint16_t *)buf)[0];
	*length = ((uint16_t *)buf)[1];
	*status = ((uint16_t *)buf)[2];

	buf += 6;

	return buf;
}

static uint8_t *genavb_mobj_get_node_next(uint8_t *buf, uint16_t length)
{
	buf += length - 2;

	return buf;
}

int genavb_mobj_rsp_check(uint8_t *response)
{
	uint16_t total_len, id, len, status;
	uint8_t *buf;

	if (!response)
		return -1;

	buf = genavb_mobj_get_node_header(response, &id, &total_len, &status);
	total_len -= 2;

	while (total_len) {
		buf = genavb_mobj_get_node_header(buf, &id, &len, &status);

		/* fixme: invalid status could be related to unknown or not supported
		 managed object. In such case the whole response could be still considered
		 as valid, and the corresponding node just ignored by the upper layer when
		 parsing the response to retrieve node's data */
		if (status != 0)
			return -1;

		/* the sum of all nodes length should not exceed the response total length */
		if (total_len < (len + 4))
			return -1;

		total_len -= len + 4;

		buf = genavb_mobj_get_node_next(buf, len);
	}

	return 0;
}
