/*
* Copyright 2017-2018, 2020-2021 NXP
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
 @brief Linux specific AVB implementation
 @details
*/

#ifndef _LINUX_AVB_H_
#define _LINUX_AVB_H_

#include <pthread.h>

#include "genavb/init.h"

struct avb_ctx {
	void *avtp;
	void *srp;
	void *maap;
	void *avdecc;

	struct avdecc_config avdecc_cfg;
	struct avtp_config avtp_cfg;
	struct srp_config srp_cfg;

	pthread_t avtp_thread;
	int avtp_status;
	pthread_cond_t avtp_cond;

	int srp_status;
	pthread_cond_t srp_cond;

	pthread_t maap_thread;
	int maap_status;
	pthread_cond_t maap_cond;

	pthread_t avdecc_thread;
	int avdecc_status;
	pthread_cond_t avdecc_cond;

	pthread_mutex_t status_mutex;
};

#endif /* _LINUX_AVB_H_ */
