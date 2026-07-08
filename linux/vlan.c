/*
 * Copyright 2022-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief Linux specific VLAN registration service implementation
 @details
*/

#include <stdint.h>
#include <stdbool.h>
#include <asm/types.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <sys/socket.h>
#include <linux/if_bridge.h>
#include <net/if.h>

#include "genavb/error.h"
#include "genavb/ether.h"
#include "os/vlan.h"
#include "net_logical_port.h"
#include "rtnetlink.h"

#define vlan_bridge_port(port_id) (logical_port_valid(port_id) && logical_port_is_bridge(port_id))

static inline bool vlan_invalid(uint16_t vid)
{
	return ((vid == VLAN_VID_MIN) || (vid > VLAN_VID_MAX));
}

#define vlan_ifinfomsg_init(ifi, ifidx) \
	(ifi)->ifi_family = PF_BRIDGE; \
	(ifi)->ifi_type = 0; \
	(ifi)->ifi_index = ifidx; \
	(ifi)->ifi_flags = 0; \
	(ifi)->ifi_change = 0;

int vlan_update(uint16_t vid, bool dynamic, struct genavb_vlan_port_map *map)
{
	struct {
		struct nlmsghdr nh;
		struct ifinfomsg ifi;
		char buf[1024];
	} req;
	struct rtattr *afspec_attr;

	uint16_t nlmsg_type, nlmsg_flags, br_vflags;
	struct bridge_vlan_info br_vinf;
	unsigned int ifindex;
	struct iovec iov;
	bool add = false;
	int rc = GENAVB_SUCCESS;

	if (vlan_invalid(vid)) {
		rc = -GENAVB_ERR_VLAN_VID;
		goto err;
	}

	if (!vlan_bridge_port(map->port_id)) {
		rc = -GENAVB_ERR_INVALID_PORT;
		goto err;
	}

	if (!dynamic) {
		rc = -GENAVB_ERR_VLAN_CONTROL;
		goto err;
	}

	ifindex = if_nametoindex(logical_port_name(map->port_id));
	if (!ifindex) {
		rc = -GENAVB_ERR_INVALID_PORT;
		goto err;
	}

	switch (map->control) {
	case GENAVB_VLAN_ADMIN_CONTROL_REGISTERED:
		nlmsg_type = RTM_SETLINK;
		nlmsg_flags = NLM_F_REQUEST;
		add = true;
		break;
	case GENAVB_VLAN_ADMIN_CONTROL_NOT_REGISTERED:
		nlmsg_type = RTM_DELLINK;
		nlmsg_flags = NLM_F_REQUEST;
		break;
	default:
		rc = -GENAVB_ERR_VLAN_CONTROL;
		goto err;
	}

	rtnetlink_nlmsghdr_init(&req.nh, NLMSG_LENGTH(sizeof(struct ifinfomsg)), nlmsg_type, nlmsg_flags);

	vlan_ifinfomsg_init(&req.ifi, ifindex);

	afspec_attr = (struct rtattr *)(((char *) &req) + NLMSG_ALIGN(req.nh.nlmsg_len));

	if (rtnetlink_attr_add(&req.nh, sizeof(req), IFLA_AF_SPEC, NULL, 0) < 0) {
		rc = -1;
		goto err;
	}

	if (add) {
		br_vflags = BRIDGE_FLAGS_MASTER;

		if (rtnetlink_attr_add(&req.nh, sizeof(req), IFLA_BRIDGE_FLAGS, &br_vflags, sizeof(u16)) < 0) {
			rc = -1;
			goto err;
		}
	}

	br_vinf.vid = vid;
	br_vinf.flags = 0;

	if (rtnetlink_attr_add(&req.nh, sizeof(req), IFLA_BRIDGE_VLAN_INFO, &br_vinf, sizeof(br_vinf)) < 0) {
		rc = -1;
		goto err;
	}

	/* Update the afspec nested attribute len */
	afspec_attr->rta_len = (char *) NLMSG_NEXT_DATA(&req.nh) - (char *)afspec_attr;

	iov.iov_base = &req;
	iov.iov_len = req.nh.nlmsg_len;
	if (rtnetlink_socket_send_iov(&iov, 1) < 0) {
		rc = -1;
		goto err;
	}

err:
	return rc;
}

int vlan_delete(uint16_t vid, bool dynamic)
{
	struct genavb_vlan_port_map vlan_port_map = {0};
	unsigned int i;
	int rc = GENAVB_SUCCESS;

	vlan_port_map.control = GENAVB_VLAN_ADMIN_CONTROL_NOT_REGISTERED;

	for (i = 0; i < logical_port_max(); i++) {
		if (!vlan_bridge_port(i))
			continue;

		vlan_port_map.port_id = i;

		if (vlan_update(vid, dynamic, &vlan_port_map) < 0)
			rc = -1;
	}

	return rc;
}

int vlan_read(uint16_t vid, bool *dynamic, struct genavb_vlan_port_map *map)
{
	return -1;
}

int vlan_dump(uint32_t *token, uint16_t *vid, bool *dynamic, struct genavb_vlan_port_map *map)
{
	return -1;
}

int vlan_port_set_default(unsigned int port_id, uint16_t vid)
{
	return -1;
}

int vlan_port_get_default(unsigned int port_id, uint16_t *vid)
{
	return -1;
}
