/*
* Copyright 2020 NXP
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
 @brief Linux specific FQTSS service implementation
 @details
*/

#define _GNU_SOURCE

#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>


#include "common/log.h"
#include "common/types.h"
#include "modules/avbdrv.h"

#include "fqtss.h"
#include "net_logical_port.h"

#define NET_TX_DEV	"/dev/net_tx"

static int fd = -1;

static int fqtss_avb_set_oper_idle_slope(unsigned int port_id, uint8_t traffic_class, unsigned int idle_slope)
{
	return -1;
}

static int net_sr_config(unsigned int port_id, void *stream_id, uint16_t vlan_id, uint8_t priority, unsigned int idle_slope)
{
	struct net_sr_config sr_config;

	sr_config.port = port_id;
	sr_config.vlan_id = vlan_id;
	sr_config.priority = priority;
	copy_64(&sr_config.stream_id, stream_id);
	sr_config.idle_slope = idle_slope;

	if (ioctl(fd, NET_IOC_SR_CONFIG, &sr_config) < 0) {
		os_log(LOG_ERR, "ioctl(NET_IOC_SR_CONFIG) failed: %s\n", strerror(errno));
		return -1;
	}

	return 0;
}

static int fqtss_avb_stream_add(unsigned int port_id, void *stream_id, uint16_t vlan_id, uint8_t priority, unsigned int idle_slope)
{
	if (!logical_port_valid(port_id))
		return -1;

	if (logical_port_is_bridge(port_id))
		return -1;

	return net_sr_config(port_id, stream_id, vlan_id, priority, idle_slope);
}

static int fqtss_avb_stream_remove(unsigned int port_id, void *stream_id, uint16_t vlan_id, uint8_t priority, unsigned int idle_slope)
{
	if (!logical_port_valid(port_id))
		return -1;

	if (logical_port_is_bridge(port_id))
		return -1;

	return net_sr_config(port_id, stream_id, vlan_id, priority, 0);
}

static void fqtss_avb_exit(void)
{
	close(fd);
	fd = -1;
}

const static struct fqtss_ops_cb fqtss_avb_ops = {
		.fqtss_set_oper_idle_slope = fqtss_avb_set_oper_idle_slope,
		.fqtss_stream_add = fqtss_avb_stream_add,
		.fqtss_stream_remove = fqtss_avb_stream_remove,
		.fqtss_exit = fqtss_avb_exit,
};

int fqtss_avb_init(struct fqtss_ops_cb *fqtss_ops)
{
	fd = open(NET_TX_DEV, O_RDWR);
	if (fd < 0) {
		os_log(LOG_ERR, "open(%s) failed: %s\n", NET_TX_DEV, strerror(errno));
		goto err_open;
	}

	/* We copy the entire struct rather than just point to it, to reduce the number of
	 * indirections in performance-sensitive code.
	 */
	memcpy(fqtss_ops, &fqtss_avb_ops, sizeof(struct fqtss_ops_cb));
	return 0;

err_open:
	return -1;
}

