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

#include "fqtss_sja.h"
#include "net_logical_port.h"

#define NET_TX_DEV	"/dev/net_tx"

static int fd = -1;

int fqtss_set_oper_idle_slope(unsigned int port_id, uint8_t traffic_class, unsigned int idle_slope)
{
	if (!logical_port_valid(port_id))
		return -1;

	if (!logical_port_is_bridge(port_id))
		return -1;

	return fqtss_sja_set_oper_idle_slope(logical_port_to_bridge_port(port_id), traffic_class, idle_slope);
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

int fqtss_stream_add(unsigned int port_id, void *stream_id, uint16_t vlan_id, uint8_t priority, unsigned int idle_slope)
{
	if (!logical_port_valid(port_id))
		return -1;

	if (logical_port_is_bridge(port_id))
		return -1;

	return net_sr_config(port_id, stream_id, vlan_id, priority, idle_slope);
}

int fqtss_stream_remove(unsigned int port_id, void *stream_id, uint16_t vlan_id, uint8_t priority, unsigned int idle_slope)
{
	if (!logical_port_valid(port_id))
		return -1;

	if (logical_port_is_bridge(port_id))
		return -1;

	return net_sr_config(port_id, stream_id, vlan_id, priority, 0);
}

int fqtss_init(void)
{
	fd = open(NET_TX_DEV, O_RDWR);
	if (fd < 0) {
		os_log(LOG_ERR, "open(%s) failed: %s\n", NET_TX_DEV, strerror(errno));
		goto err_open;
	}

	if (fqtss_sja_init() < 0)
		goto err_sja;

	return 0;

err_sja:
	close(fd);
	fd = -1;

err_open:
	return -1;
}

void fqtss_exit(void)
{
	fqtss_sja_exit();
	close(fd);
	fd = -1;
}

