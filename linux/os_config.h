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

#ifndef _LINUX_OS_CONFIG_H_
#define _LINUX_OS_CONFIG_H_

#include "genavb/config.h"

struct os_config {
	struct os_logical_port_config {
		char endpoint[CFG_MAX_ENDPOINTS][32];
		char bridge[CFG_MAX_BRIDGES][32];
		char bridge_ports[CFG_MAX_BRIDGES][CFG_MAX_NUM_PORT][32];
	} logical_port_config;

	struct os_clock_config {
		char endpoint_gptp[CFG_MAX_GPTP_DOMAINS][CFG_MAX_ENDPOINTS][32];
		char endpoint_local[CFG_MAX_ENDPOINTS][32];
		char bridge_gptp[CFG_MAX_GPTP_DOMAINS][CFG_MAX_BRIDGES][32];
		char bridge_local[CFG_MAX_BRIDGES][32];
	} clock_config;
};

int os_config_get(struct os_config *config);

#endif /* _LINUX_OS_CONFIG_H_ */
