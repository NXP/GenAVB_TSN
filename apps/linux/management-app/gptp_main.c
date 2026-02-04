/*
 * Copyright 2018-2022, 2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <genavb/genavb.h>
#include <genavb/helpers.h>
#include <genavb/managed_objects.h>
#include "common.h"

static char *module_name = "ptp";
static char *device_name;

#define BUF_LEN 100

static int dump_stats(struct genavb_control_handle *ctrl_h, uint16_t instance, uint16_t port)
{
	struct genavb_msg_managed_set_response get_response;
	struct genavb_mobj_cmd cmd;
	uint8_t buf[BUF_LEN];
	uint8_t *data;
	int i;
	uint16_t id, length, status, total_length;
	char name[][64] = {
		"rxSyncCount",
		"rxFollowUpCount",
		"rxPdelayRequestCount",
		"rxPdelayResponseCount",
		"rxPdelayResponseFollowUpCount",
		"rxAnnounceCount",
		"rxPTPPacketDiscardCount",
		"syncReceiptTimeoutCount",
		"announceReceiptTimeoutCount",
		"pdelayAllowedLostResponsesExceededCount",
		"txSyncCount",
		"txFollowUpCount",
		"txPdelayRequestCount",
		"txPdelayResponseCount",
		"txPdelayResponseFollowUpCount",
		"txAnnounceCount"
	};

	genavb_mobj_cmd_init(&cmd, buf, BUF_LEN);

	genavb_mobj_cmd_start_node(&cmd, GPTP_NODE_INSTANCE_LIST);

	genavb_mobj_cmd_set_list_index(&cmd, GPTP_INSTANCE_INSTANCE_INDEX, instance);

	genavb_mobj_cmd_start_node(&cmd, GPTP_INSTANCE_PORT_STATS_DS);

	genavb_mobj_cmd_set_list_index(&cmd, GPTP_PORT_STATS_DS_PORT_ID, port);

	for (i = 0; i < 16; i++)
		genavb_mobj_cmd_get_leaf(&cmd, i + 1);

	genavb_mobj_cmd_end_node(&cmd);

	genavb_mobj_cmd_end_node(&cmd);

	if (managed_get(ctrl_h, genavb_mobj_cmd_buf(&cmd), genavb_mobj_cmd_len(&cmd), &get_response, sizeof(get_response)) < 0)
		goto err;

	if (genavb_mobj_rsp_check((u_int8_t *)&get_response) < 0)
		goto err;

	/* instance node header */
	data = get_node_header((uint8_t *)&get_response, &id, &total_length, &status);

	/* instance entry key header */
	data = get_node_header(data, &id, &length, &status);

	data = get_node_next(data, length);

	/* port stats node header */
	data = get_node_header(data, &id, &length, &status);

	/* port entry key header */
	data = get_node_header(data, &id, &length, &status);

	printf("%s %s port: %u stats\n", module_name, device_name, ((uint16_t *)data)[0]);

	data = get_node_next(data, length);

	for (i = 0; i < 16; i++) {
		data = get_node_header(data, &id, &length, &status);

		if (!status)
			printf("%-40s %8u\n", name[i], ((uint32_t *)data)[0]);

		data = get_node_next(data, length);
	}

	return 0;

err:
	printf("%s %s port: %u stats, error\n", module_name, device_name, port);

	return -1;
}

static int set_port_state(struct genavb_control_handle *ctrl_h, uint16_t instance, uint16_t port, unsigned int enable)
{
	struct genavb_msg_managed_set_response set_response;
	struct genavb_mobj_cmd cmd;
	uint8_t buf[BUF_LEN];

	genavb_mobj_cmd_init(&cmd, buf, BUF_LEN);

	genavb_mobj_cmd_start_node(&cmd, GPTP_NODE_INSTANCE_LIST);

	genavb_mobj_cmd_set_list_index(&cmd, GPTP_INSTANCE_INSTANCE_INDEX, instance);

	genavb_mobj_cmd_start_node(&cmd, GPTP_INSTANCE_PORT_DS);

	genavb_mobj_cmd_set_list_index(&cmd, GPTP_PORT_DS_PORT_ID, port);

	genavb_mobj_cmd_set_leaf_u8(&cmd, GPTP_PORT_DS_PTP_PORT_ENABLED, enable);

	genavb_mobj_cmd_end_node(&cmd);

	genavb_mobj_cmd_end_node(&cmd);

	if (managed_set(ctrl_h, genavb_mobj_cmd_buf(&cmd), genavb_mobj_cmd_len(&cmd), &set_response, sizeof(set_response)) < 0)
		goto err;

	if (genavb_mobj_rsp_check((u_int8_t *)&set_response) < 0)
		goto err;

	printf("%s %s set port: %u, state: %u, success\n", module_name, device_name, port, enable);

	return 0;

err:
	printf("%s %s set port: %u, state: %u, error\n", module_name, device_name, port, enable);

	return -1;
}

static int get_port_state(struct genavb_control_handle *ctrl_h, uint16_t instance, uint16_t port)
{
	struct genavb_msg_managed_get_response get_response;
	struct genavb_mobj_cmd cmd;
	uint8_t buf[BUF_LEN];

	genavb_mobj_cmd_init(&cmd, buf, BUF_LEN);

	genavb_mobj_cmd_start_node(&cmd, GPTP_NODE_INSTANCE_LIST);

	genavb_mobj_cmd_set_list_index(&cmd, GPTP_INSTANCE_INSTANCE_INDEX, instance);

	genavb_mobj_cmd_start_node(&cmd, GPTP_INSTANCE_PORT_DS);

	genavb_mobj_cmd_set_list_index(&cmd, GPTP_PORT_DS_PORT_ID, port);

	genavb_mobj_cmd_get_leaf(&cmd, GPTP_PORT_DS_PTP_PORT_ENABLED);

	genavb_mobj_cmd_end_node(&cmd);

	genavb_mobj_cmd_end_node(&cmd);

	if (managed_get(ctrl_h, genavb_mobj_cmd_buf(&cmd), genavb_mobj_cmd_len(&cmd), &get_response, sizeof(get_response)) < 0)
		goto err;

	if (genavb_mobj_rsp_check((u_int8_t *)&get_response) < 0)
		goto err;

	printf("%s %s get port: %u, state: %u\n", module_name, device_name, port, get_response.data[17 * 2]);

	return 0;

err:
	printf("%s %s get port: %u, state: error\n", module_name, device_name, port);

	return -1;
}


static int set_priority1(struct genavb_control_handle *ctrl_h, uint16_t instance, unsigned int priority1)
{
	struct genavb_msg_managed_set_response set_response;
	struct genavb_mobj_cmd cmd;
	uint8_t buf[BUF_LEN];

	genavb_mobj_cmd_init(&cmd, buf, BUF_LEN);

	genavb_mobj_cmd_start_node(&cmd, GPTP_NODE_INSTANCE_LIST);

	genavb_mobj_cmd_set_list_index(&cmd, GPTP_INSTANCE_INSTANCE_INDEX, instance);

	genavb_mobj_cmd_start_node(&cmd, GPTP_INSTANCE_DEFAULT_DS);

	genavb_mobj_cmd_set_leaf_u8(&cmd, GPTP_DEFAULT_DS_PRIORITY1, priority1);

	genavb_mobj_cmd_end_node(&cmd);

	genavb_mobj_cmd_end_node(&cmd);

	if (managed_set(ctrl_h, genavb_mobj_cmd_buf(&cmd), genavb_mobj_cmd_len(&cmd), &set_response, sizeof(set_response)) < 0)
		goto err;

	if (genavb_mobj_rsp_check((u_int8_t *)&set_response) < 0)
		goto err;

	printf("%s %s set instance: %u, priority1: %u, success\n", module_name, device_name, instance, priority1);

	return 0;

err:
	printf("%s %s set instance: %u, priority1: %u, error\n", module_name, device_name, instance, priority1);

	return -1;
}

static int get_priority1(struct genavb_control_handle *ctrl_h, uint16_t instance)
{
	struct genavb_msg_managed_get_response get_response;
	struct genavb_mobj_cmd cmd;
	uint8_t buf[BUF_LEN];

	genavb_mobj_cmd_init(&cmd, buf, BUF_LEN);

	genavb_mobj_cmd_start_node(&cmd, GPTP_NODE_INSTANCE_LIST);

	genavb_mobj_cmd_set_list_index(&cmd, GPTP_INSTANCE_INSTANCE_INDEX, instance);

	genavb_mobj_cmd_start_node(&cmd, GPTP_INSTANCE_DEFAULT_DS);

	genavb_mobj_cmd_get_leaf(&cmd, GPTP_DEFAULT_DS_PRIORITY1);

	genavb_mobj_cmd_end_node(&cmd);

	genavb_mobj_cmd_end_node(&cmd);

	if (managed_get(ctrl_h, genavb_mobj_cmd_buf(&cmd), genavb_mobj_cmd_len(&cmd), &get_response, sizeof(get_response)) < 0)
		goto err;

	if (genavb_mobj_rsp_check((u_int8_t *)&get_response) < 0)
		goto err;

	printf("%s %s get instance: %u, priority1: %u\n", module_name, device_name, instance, ((uint8_t *)&get_response)[26]);

	return 0;

err:
	printf("%s %s get instance: %u, priority1: error\n", module_name, device_name, instance);

	return -1;
}

int gptp_main(struct genavb_handle *avb_h, int argc, char *argv[])
{
	struct genavb_control_handle *ctrl_h;
	struct genavb_control_handle *endpoint_ctrl_h;
	struct genavb_control_handle *bridge_ctrl_h;
	uint16_t instance;
	unsigned int priority1;
	uint16_t port;
	unsigned int set;
	unsigned int state;
	int option;
	int rc;
	unsigned long optval_ul;

	rc = genavb_control_open(avb_h, &endpoint_ctrl_h, GENAVB_CTRL_GPTP);
	if (rc != GENAVB_SUCCESS)
		endpoint_ctrl_h = NULL;

	rc = genavb_control_open(avb_h, &bridge_ctrl_h, GENAVB_CTRL_GPTP_BRIDGE);
	if (rc != GENAVB_SUCCESS)
		bridge_ctrl_h = NULL;

	if ((endpoint_ctrl_h == NULL) && (bridge_ctrl_h == NULL))
		goto err_control_open;

	/* default options */
	ctrl_h = endpoint_ctrl_h;
	device_name = "endpoint";
	instance = 0;
	port = 0;
	set = 0;

	while ((option = getopt(argc, argv, "EBGSI:P:psdh")) != -1) {
		/* common options */
		switch (option) {
		case 'E':
			ctrl_h = endpoint_ctrl_h;
			device_name = "endpoint";
			break;

		case 'B':
			ctrl_h = bridge_ctrl_h;
			device_name = "bridge";
			break;

		case 'G':
			set = 0;
			break;

		case 'S':
			set = 1;
			break;

		case 'I':
			if (h_strtoul(&optval_ul, optarg, NULL, 0) < 0) {
				usage();
				rc = -1;
				goto exit;
			}
			instance = (uint16_t)optval_ul;
			break;

		case 'P':
			if (h_strtoul(&optval_ul, optarg, NULL, 0) < 0) {
				usage();
				rc = -1;
				goto exit;
			}
			port = (uint16_t)optval_ul;
			break;

		case 'p':
			if (set) {
				if (!argv[optind] || argv[optind][0] == '-' || (h_strtoul(&optval_ul, argv[optind], NULL, 0) < 0)) {
					usage();
					rc = -1;
					goto exit;
				}
				priority1 = (unsigned int)optval_ul;
				rc = set_priority1(ctrl_h, instance, priority1);
			} else {
				rc = get_priority1(ctrl_h, instance);
			}
			break;

		case 's':
			if (set) {
				if (!argv[optind] || argv[optind][0] == '-' || (h_strtoul(&optval_ul, argv[optind], NULL, 0) < 0)) {
					usage();
					rc = -1;
					goto exit;
				}

				state = (unsigned int)optval_ul;
				rc = set_port_state(ctrl_h, instance, port, state);
			} else {
				rc = get_port_state(ctrl_h, instance, port);
			}
			break;

		case 'd':
			rc = dump_stats(ctrl_h, instance, port);
			break;

		case 'h':
		default:
			usage();
			rc = -1;
			goto exit;
		}
	}

exit:
	if (endpoint_ctrl_h)
		genavb_control_close(endpoint_ctrl_h);

	if (bridge_ctrl_h)
		genavb_control_close(bridge_ctrl_h);

err_control_open:
	return rc;
}
