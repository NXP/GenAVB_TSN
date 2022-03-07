/*
 * Copyright 2021 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <sys/poll.h>
#include <errno.h>
#include <genavb/genavb.h>

#include "../common/helpers.h"

#define DEFAULT_PORT			0
#define DEFAULT_MAC_ADDR		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00}

static const char *maap_status_str[] = {
	[MAAP_STATUS_SUCCESS] = "MAAP_SUCCESS",
	[MAAP_STATUS_FREE] = "MAAP_FREE",
	[MAAP_STATUS_CONFLICT] = "MAAP_CONFLICT",
	[MAAP_STATUS_ERROR] = "MAAP_ERROR"
};

void usage (void)
{
	printf("\nUsage:\nmaap-ctrl-app [options]\n");
	printf("\nOptions:\n"
		"\nCommon options:\n"
		"\t-n                         Create new range, -c and -p required\n"
		"\t-d                         Delete a range, -c -p and -a required\n"
		"\t-f <int>                   Flag, 1 if you want to use a prefered first mac address for the new range otherwise 0 (default)\n"
		"\t-a <first MAC address>     First prefered mac address of the new range or first mac address of the range to delete, check IEEE1722-2016 for the standard ranges\n"
		"\t-c <int>                   Number of mac address to allocate/allocated in the range, max = 255 (default = 1)\n"
		"\t-p <port>                  (Logical) Port where the new range should be allocated\n"
		"\t-h                         Print this help text\n"
		"\t-l                         Listen to status messages/responses from MAAP\n");
}

static int new_range(struct genavb_control_handle *ctrl_h, uint16_t port, uint8_t flag, uint8_t *addr, uint8_t count)
{
	struct genavb_maap_command command_msg;
	unsigned int msg_type;
	int rc;

	command_msg.flag = flag;
	command_msg.port_id = port;
	command_msg.count = count;

	if (flag != 1) {
		memset(command_msg.base_address, 0, sizeof(uint8_t) * 6);
	} else {
		memcpy(command_msg.base_address, addr, sizeof(uint8_t) * 6);
	}

	msg_type = GENAVB_MSG_MAAP_CREATE_RANGE;
	rc = genavb_control_send(ctrl_h, msg_type, &command_msg, sizeof(command_msg));

	if (rc < 0) {
		printf("genavb_control_send_sync() failed: %s\n", genavb_strerror(rc));

		return -1;
	}

	return 0;
}

static int delete_range(struct genavb_control_handle *ctrl_h, uint16_t port, uint8_t flag, uint8_t *addr, uint8_t count)
{
	struct genavb_maap_command command_msg;
	unsigned int msg_type;
	int rc;

	command_msg.flag = flag;
	command_msg.port_id = port;
	command_msg.count = count;
	memcpy(command_msg.base_address, addr, sizeof(uint8_t) * 6);

	msg_type = GENAVB_MSG_MAAP_DELETE_RANGE;
	rc = genavb_control_send(ctrl_h, msg_type, &command_msg, sizeof(command_msg));

	if (rc < 0) {
		printf("genavb_control_send_sync() failed: %s\n", genavb_strerror(rc));

		return -1;
	}

	return 0;
}

static void maap_listen(struct genavb_control_handle *ctrl_h, int ctrl_fd)
{
	unsigned int msg_type;
	struct genavb_maap_response msg;
	unsigned int msg_len;
	fd_set set;
	int rc;

	while (1) {
		FD_ZERO(&set);
		FD_SET(ctrl_fd, &set);

		rc = select(ctrl_fd + 1, &set, NULL, NULL, NULL);
		if (rc < 0) {
			printf ("select() failed: %s\n", strerror(errno));
			break;
		}

		if (!FD_ISSET(ctrl_fd, &set))
			continue;

		msg_len = sizeof(msg);

		rc = genavb_control_receive(ctrl_h, &msg_type, &msg, &msg_len);
		if (rc < 0) {
			printf ("genavb_control_receive() failed: %s\n", genavb_strerror(rc));
			break;
		}

		switch (msg_type) {
		case GENAVB_MSG_MAAP_STATUS:
			printf ("(%u) First MAC address: %02x-%02x-%02x-%02x-%02x-%02x, number of addresses: %u, port: %u, status: %u, %s\n",
				getpid(),
				msg.base_address[0], msg.base_address[1], msg.base_address[2], msg.base_address[3],
				msg.base_address[4], msg.base_address[5], msg.count,
				msg.port_id, msg.status, maap_status_str[msg.status]);
			break;

		default:
			printf ("Unexpected message type %d\n", msg_type);
			break;
		}
	}
}

int main(int argc, char *argv[])
{
	struct genavb_handle *avb_h;
	struct genavb_control_handle *ctrl_h;
	unsigned long port = DEFAULT_PORT;
	unsigned long flag = 0;
	unsigned long count = 1;
	uint8_t mac_addr[6] = DEFAULT_MAC_ADDR;
	int ctrl_fd;
	int option;
	int rc = 0;

	setlinebuf(stdout);

	printf("NXP's GenAVB MAAP control application\n");

	/*
	* setup the avb stack
	*/

	rc = genavb_init(&avb_h, 0);
	if (rc != GENAVB_SUCCESS) {
		printf("genavb_init() failed: %s\n", genavb_strerror(rc));
		rc = -1;
		goto error_avb_init;
	}

	rc = genavb_control_open(avb_h, &ctrl_h, GENAVB_CTRL_MAAP);
	if (rc != GENAVB_SUCCESS) {
		printf("genavb_control_open() failed: %s\n", genavb_strerror(rc));
		rc = -1;
		goto error_control_open;
	}

	ctrl_fd = genavb_control_rx_fd(ctrl_h);

	/*
	* retrieve user's configuration parameters
	*/
	while ((option = getopt(argc, argv,"f:a:c:p:hlnd")) != -1) {
		switch (option) {
		case 'f':
			if (h_strtoul(&flag, optarg, NULL, 0) < 0) {
				printf("invalid -f %s option\n", optarg);
				rc = -1;
				goto exit;
			}

			break;

		case 'a':
			if (sscanf(optarg, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &mac_addr[0], &mac_addr[1], &mac_addr[2], &mac_addr[3], &mac_addr[4], &mac_addr[5]) < 6) {
				printf("invalid -a %s option\n", optarg);
				rc = -1;
				goto exit;
			}

			break;

		case 'c':
			if (h_strtoul(&count, optarg, NULL, 0) < 0) {
				printf("invalid -c %s option\n", optarg);
				rc = -1;
				goto exit;
			}

			break;

		case 'p':
			if (h_strtoul(&port, optarg, NULL, 0) < 0) {
				printf("invalid -p %s option\n", optarg);
				rc = -1;
				goto exit;
			}

			break;

		case 'l':
			maap_listen(ctrl_h, ctrl_fd);
			break;

		case 'n':
			rc = new_range(ctrl_h, port, flag, mac_addr, count);
			break;

		case 'd':
			rc = delete_range(ctrl_h, port, flag, mac_addr, count);
			break;

		case 'h':
		default:
			usage();
			rc = -1;
			goto exit;
		}
	}

exit:
	genavb_control_close(ctrl_h);

error_control_open:
	genavb_exit(avb_h);

error_avb_init:
	return rc;
}
