/*
* Copyright 2021 NXP
*
* NXP Confidential. This software is owned or controlled by NXP and may only
* be used strictly in accordance with the applicable license terms.  By expressly
* accepting such terms or by downloading, installing, activating and/or otherwise
* using the software, you are agreeing that you have read, and that you agree to
* comply with and are bound by, such license terms.  If you do not agree to be
* bound by the applicable license terms, then you may not retain, install, activate
* or otherwise use the software.
*/

/**
 @file
 @brief Linux specific FDB service implementation
 @details
*/
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <asm/types.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <linux/if_bridge.h>
#include <sys/socket.h>
#include <net/if.h>
#include "rtnetlink.h"

#include "os/fdb.h"

#include "common/log.h"

#include "net_logical_port.h"
#include "fdb.h"

#if defined(CONFIG_SJA1105)
#define ndmsg_fdb_init(ndm, ifindex) \
	(ndm)->ndm_family = PF_BRIDGE; \
	(ndm)->ndm_ifindex = ifindex; \
	(ndm)->ndm_flags = NTF_SELF; \
	(ndm)->ndm_state = NUD_PERMANENT; \
	(ndm)->ndm_type = 0;

static int sja_bridge_rtnetlink(u8 *mac_addr, u16 vid, unsigned int port_id, bool add)
{
	unsigned int ifindex;
	struct iovec iov;
	u16 nlmsg_type, nlmsg_flags;
	struct {
		struct nlmsghdr nh;
		struct ndmsg ndm;
		char buf[128];
	} req;

	if (!logical_port_valid(port_id)) {
		os_log(LOG_ERR, "logical_port(%u) invalid\n", port_id);
		goto err;
	}

	if (!logical_port_is_bridge(port_id)) {
		os_log(LOG_ERR, "logical_port(%u) is not a valid bridge port\n", port_id);
		goto err;
	}

	ifindex = if_nametoindex(logical_port_name(port_id));
	if (!ifindex) {
		os_log(LOG_ERR, "if_nametoindex() failed: %s\n", strerror(errno));
		goto err;
	}

	if (add) {
		nlmsg_type = RTM_NEWNEIGH;	/* add port to fdb entry */
		nlmsg_flags = NLM_F_REQUEST | NLM_F_REPLACE | NLM_F_CREATE;
	} else {
		nlmsg_type = RTM_DELNEIGH;	/* delete port from fdb entry */
		nlmsg_flags = NLM_F_REQUEST;
	}

	rtnetlink_nlmsghdr_init(&req.nh, NLMSG_LENGTH(sizeof(struct ndmsg)), nlmsg_type, nlmsg_flags);

	ndmsg_fdb_init(&req.ndm, ifindex);

	if (rtnetlink_attr_add(&req.nh, sizeof(req), NDA_LLADDR, mac_addr, 6) < 0)
		goto err;

	if (rtnetlink_attr_add(&req.nh, sizeof(req), NDA_VLAN, &vid, sizeof(vid)) < 0)
		goto err;

	iov.iov_base = &req;
	iov.iov_len = req.nh.nlmsg_len;

	if (rtnetlink_socket_send_iov(&iov, 1) < 0)
		goto err;

	os_log(LOG_INFO, "%s FDB: logical_port(%u) mac_addr(%02x:%02x:%02x:%02x:%02x:%02x) vlan_id(%u) ifindex(%u)\n",
			add ? "add" : "remove", port_id, mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5], vid, ifindex);

	return 0;

err:
	os_log(LOG_ERR, "%s failed: logical_port(%u) mac_addr(%02x:%02x:%02x:%02x:%02x:%02x) vlan_id(%u)\n",
			add ? "add" : "remove", port_id, mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5], vid);
	return -1;
}
#else
static int sja_bridge_rtnetlink(u8 *mac_addr, u16 vid, unsigned int port_id, bool add)
{
	return -1;
}
#endif

const static struct fdb_ops_cb fdb_sja_ops = {
		.bridge_rtnetlink = sja_bridge_rtnetlink,
};

int fdb_sja_init(struct fdb_ops_cb *fdb_ops)
{
	/* We copy the entire struct rather than just point to it, to reduce the number of
	 * indirections in performance-sensitive code.
	 */
	memcpy(fdb_ops, &fdb_sja_ops, sizeof(struct fdb_ops_cb));

	return 0;
}

