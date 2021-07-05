/*
* Copyright 2014 Freescale Semiconductor, Inc.
* Copyright 2018, 2020 NXP
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
 \file init.c
 \brief GenAVB public initialization API for linux
 \details API definition for the GenAVB library
 \copyright Copyright 2014 Freescale Semiconductor, Inc.
            Copyright 2018, 2020 NXP
*/

#define _POSIX_C_SOURCE 200809L

#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <pthread.h>

#include "linux/init.h"

#include "common/log.h"
#include "common/ipc.h"

#include "api/init.h"

#ifdef CONFIG_AVTP
void streaming_exit(struct genavb_handle *genavb);
#endif

pthread_mutex_t avb_mutex = PTHREAD_MUTEX_INITIALIZER;
struct genavb_handle *genavb_handle = NULL;


int genavb_init(struct genavb_handle **genavb, unsigned int flags)
{
	int rc;

	pthread_mutex_lock(&avb_mutex);

	if (genavb_handle) {
		rc = -GENAVB_ERR_ALREADY_INITIALIZED;
		goto err_genavb_handle;
	}

	*genavb = malloc(sizeof(struct genavb_handle));
	if (!*genavb) {
		rc = -GENAVB_ERR_NO_MEMORY;
		goto err_alloc;
	}

	memset(*genavb, 0, sizeof(struct genavb_handle));

	(*genavb)->flags = flags;
	(*genavb)->flags &= ~AVTP_INITIALIZED;

	list_head_init(&(*genavb)->streams);

	/* reduce stack debug print */
	log_level_set(api_COMPONENT_ID, LOG_CRIT);

	if (os_init() < 0) {
		rc = -GENAVB_ERR_CLOCK;
		goto err_osal;
	}

	genavb_handle = *genavb;

	pthread_mutex_unlock(&avb_mutex);

	return GENAVB_SUCCESS;

err_osal:
err_alloc:
err_genavb_handle:
	pthread_mutex_unlock(&avb_mutex);

	return rc;
}

int genavb_exit(struct genavb_handle *genavb)
{
	int rc;

	if (!genavb) {
		rc = -GENAVB_ERR_INVALID;
		goto err_genavb_handle_null;
	}

	pthread_mutex_lock(&avb_mutex);

	if (genavb_handle != genavb) {
		rc = -GENAVB_ERR_INVALID;
		goto err_genavb_handle_invalid;
	}

	os_exit();

	genavb_handle = NULL;

#ifdef CONFIG_AVTP
	streaming_exit(genavb);
#endif

	pthread_mutex_unlock(&avb_mutex);

	free(genavb);

	return GENAVB_SUCCESS;

err_genavb_handle_invalid:
	pthread_mutex_unlock(&avb_mutex);

err_genavb_handle_null:
	return rc;
}
