 /*
 * Copyright 2025 NXP
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

static int cmd_check_headroom(struct genavb_mobj_cmd *cmd, uint16_t len)
{
	if ((cmd->top + len) > (cmd->end - cmd->buf))
		return -1;

	return 0;
}

int genavb_mobj_cmd_len(struct genavb_mobj_cmd *cmd)
{
	if (!cmd || (cmd->sp != 0))
		return -1;

	return cmd->top;
}

uint8_t *genavb_mobj_cmd_buf(struct genavb_mobj_cmd *cmd)
{
	if (!cmd)
		return NULL;

	return cmd->buf;
}

int genavb_mobj_cmd_init(struct genavb_mobj_cmd *cmd, uint8_t *buf, unsigned int size)
{
	if (!cmd || !buf)
		return -1;

	cmd->buf = buf;
	cmd->end = buf + size;

	memset(buf, 0, size);
	memset(cmd->stack, 0, STACK_DEPTH * sizeof(struct genavb_mobj_cmd_node_header *));
	cmd->sp = 0;
	cmd->cur = cmd->stack[0];
	cmd->top = 0;

	return 0;
}

int genavb_mobj_cmd_start_node(struct genavb_mobj_cmd *cmd, uint16_t node_id)
{
	if (!cmd)
		return -1;

	if (cmd->sp > (STACK_DEPTH - 1))
		return - 1;

	if (cmd_check_headroom(cmd, sizeof(struct genavb_mobj_cmd_node_header)) < 0)
		return -1;

	cmd->stack[cmd->sp] = (struct genavb_mobj_cmd_node_header *)&cmd->buf[cmd->top]; /* save node position */

	cmd->cur = cmd->stack[cmd->sp];
	cmd->cur->id = node_id;
	cmd->cur->length = 0; /* placeholder for length */

	cmd->sp++;

	cmd->top += sizeof(struct genavb_mobj_cmd_node_header);

	return 0;
}

int genavb_mobj_cmd_end_node(struct genavb_mobj_cmd *cmd)
{
	struct genavb_mobj_cmd_node_header *cur_node;
	struct genavb_mobj_cmd_node_header *sub_node;

	if (!cmd || !cmd->sp)
		return -1;

	cmd->sp--;

	if (cmd->sp > 0) {
		sub_node = cmd->stack[cmd->sp];
		cur_node = cmd->stack[cmd->sp - 1];
		cur_node->length += sub_node->length + sizeof(struct genavb_mobj_cmd_node_header);
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

	cmd->cur->length += sizeof(struct genavb_mobj_cmd_node_header) + size;

	return 0;
}

int genavb_mobj_cmd_get_leaf(struct genavb_mobj_cmd *cmd, uint16_t leaf_id)
{
	uint16_t val = 0;

	return _genavb_managed_cmd_leaf_set(cmd, leaf_id, &val, 0);
}

int genavb_mobj_cmd_set_list_index(struct genavb_mobj_cmd *cmd, uint16_t node_id, uint16_t index)
{
	return _genavb_managed_cmd_leaf_set(cmd, node_id, &index, sizeof(uint16_t));
}

int genavb_mobj_cmd_set_leaf(struct genavb_mobj_cmd *cmd, uint16_t leaf_id, void *leaf_val, unsigned int size)
{
	return _genavb_managed_cmd_leaf_set(cmd, leaf_id, leaf_val, size);
}

int genavb_mobj_cmd_set_leaf_u8(struct genavb_mobj_cmd *cmd, uint16_t leaf_id, uint8_t leaf_val)
{
	return _genavb_managed_cmd_leaf_set(cmd, leaf_id, &leaf_val, sizeof(uint8_t));
}

int genavb_mobj_cmd_set_leaf_u16(struct genavb_mobj_cmd *cmd, uint16_t leaf_id, uint16_t leaf_val)
{
	return _genavb_managed_cmd_leaf_set(cmd, leaf_id, &leaf_val, sizeof(uint16_t));
}

int genavb_mobj_cmd_set_leaf_u32(struct genavb_mobj_cmd *cmd, uint16_t leaf_id, uint32_t leaf_val)
{
	return _genavb_managed_cmd_leaf_set(cmd, leaf_id, &leaf_val, sizeof(uint32_t));
}

int genavb_mobj_cmd_set_leaf_u64(struct genavb_mobj_cmd *cmd, uint16_t leaf_id, uint64_t leaf_val)
{
	return _genavb_managed_cmd_leaf_set(cmd, leaf_id, &leaf_val, sizeof(uint64_t));
}

int genavb_mobj_cmd_set_leaf_s8(struct genavb_mobj_cmd *cmd, uint16_t leaf_id, int8_t leaf_val)
{
	return _genavb_managed_cmd_leaf_set(cmd, leaf_id, &leaf_val, sizeof(int8_t));
}

int genavb_mobj_cmd_set_leaf_s16(struct genavb_mobj_cmd *cmd, uint16_t leaf_id, int16_t leaf_val)
{
	return _genavb_managed_cmd_leaf_set(cmd, leaf_id, &leaf_val, sizeof(int16_t));
}

int genavb_mobj_cmd_set_leaf_s32(struct genavb_mobj_cmd *cmd, uint16_t leaf_id, int32_t leaf_val)
{
	return _genavb_managed_cmd_leaf_set(cmd, leaf_id, &leaf_val, sizeof(int32_t));
}

int genavb_mobj_cmd_set_leaf_s64(struct genavb_mobj_cmd *cmd, uint16_t leaf_id, int64_t leaf_val)
{
	return _genavb_managed_cmd_leaf_set(cmd, leaf_id, &leaf_val, sizeof(int64_t));
}

int genavb_mobj_cmd_set_leaf_scaled_ns(struct genavb_mobj_cmd *cmd, uint16_t leaf_id, struct ptp_scaled_ns *leaf_val)
{
	return _genavb_managed_cmd_leaf_set(cmd, leaf_id, leaf_val, sizeof(struct ptp_scaled_ns));
}

int genavb_mobj_cmd_set_leaf_uscaled_ns(struct genavb_mobj_cmd *cmd, uint16_t leaf_id, struct ptp_u_scaled_ns *leaf_val)
{
	return _genavb_managed_cmd_leaf_set(cmd, leaf_id, leaf_val, sizeof(struct ptp_u_scaled_ns));
}

int genavb_mobj_cmd_set_leaf_port_identity(struct genavb_mobj_cmd *cmd, uint16_t leaf_id, struct ptp_port_identity *leaf_val)
{
	return _genavb_managed_cmd_leaf_set(cmd, leaf_id, leaf_val, sizeof(struct ptp_port_identity));
}

int genavb_mobj_cmd_set_leaf_clock_identity(struct genavb_mobj_cmd *cmd, uint16_t leaf_id, struct ptp_clock_identity *leaf_val)
{
	return _genavb_managed_cmd_leaf_set(cmd, leaf_id, leaf_val, sizeof(struct ptp_clock_identity));
}

int genavb_mobj_cmd_set_leaf_mac_address(struct genavb_mobj_cmd *cmd, uint16_t leaf_id, uint8_t *leaf_val)
{
	return _genavb_managed_cmd_leaf_set(cmd, leaf_id, leaf_val, sizeof(uint8_t)*6);
}

int genavb_mobj_cmd_set_leaf_double(struct genavb_mobj_cmd *cmd, uint16_t leaf_id, double leaf_val)
{
	return _genavb_managed_cmd_leaf_set(cmd, leaf_id, &leaf_val, sizeof(double));
}

int genavb_mobj_cmd_set_leaf_bool(struct genavb_mobj_cmd *cmd, uint16_t leaf_id, bool leaf_val)
{
	return _genavb_managed_cmd_leaf_set(cmd, leaf_id, &leaf_val, sizeof(uint8_t));
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
		if (status != STATUS_OK)
			return -1;

		/* the sum of all nodes length should not exceed the response total length */
		if (total_len < (len + 4))
			return -1;

		total_len -= len + 4;

		buf = genavb_mobj_get_node_next(buf, len);
	}

	return 0;
}
