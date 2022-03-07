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
 @brief SJA specific FQTSS service implementation
 @details
*/

#define _GNU_SOURCE

#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include "common/log.h"

#include <sja1105_fops.h>

#define SJA_DEVICE "/dev/sja1105"

static int fd = -1;

int fqtss_sja_set_oper_idle_slope(unsigned int port_id, uint8_t traffic_class, unsigned int idle_slope)
{
	struct _SJA_CBS_CONF cbs;

	cbs.port = port_id;
	cbs.vlanPrio = traffic_class;
	cbs.idleSlope = idle_slope;

	if (ioctl(fd, SJA_IOC_SET_IDLE_SLOPE_CBS, &cbs) < 0)
		return -1;

	return 0;
}

int fqtss_sja_init(void)
{
	fd = open(SJA_DEVICE, O_RDWR);
	if (fd < 0) {
		os_log(LOG_ERR, "open(%s) failed: %s\n", SJA_DEVICE, strerror(errno));
		goto err;
	}

	if (ioctl(fd, SJA_IOC_FLUSH_CBS, NULL) < 0) {
		os_log(LOG_ERR, "ioctl(SJA_IOC_FLUSH_CBS) failed: %s\n", strerror(errno));
		goto err;
	}

	return 0;

err:
	return -1;
}

void fqtss_sja_exit(void)
{
	close(fd);
	fd = -1;
}
