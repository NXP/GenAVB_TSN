/*
* Copyright 2019-2020, 2022 NXP
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
 @brief Linux specific TSN (gPTP + SRP) implementation
 @details
*/

#ifndef _LINUX_TSN_H_
#define _LINUX_TSN_H_

#include <pthread.h>

#include "genavb/init.h"
#include "genavb/sr_class.h"
#include "os/clock.h"

struct gptp_linux_config {
	struct fgptp_config gptp_cfg;
	os_clock_id_t clock_log;
	char nvram_file[256];
};

struct tsn_ctx {
	struct gptp_linux_config gptp_linux_cfg;
	struct management_config management_cfg;
	struct srp_config srp_cfg;

	uint8_t sr_class[CFG_SR_CLASS_MAX];

	int gptp_status;
	pthread_cond_t gptp_cond;

	int management_status;
	pthread_cond_t management_cond;

	int srp_status;
	pthread_cond_t srp_cond;

	pthread_mutex_t status_mutex;
};

#endif /* _LINUX_TSN_H_ */
