/*
* Copyright 2014 Freescale Semiconductor, Inc.
* Copyright 2020, 2023 NXP
*
* SPDX-License-Identifier: BSD-3-Clause
*/

/**
 \file error.c
 \brief GenAVB public API
 \details API definition for the GenAVB library

 \copyright Copyright 2014 Freescale Semiconductor, Inc.
 \copyright Copyright 2020, 2023 NXP
*/

#include "genavb/error.h"

#define __ERR_NUM(err)	[GENAVB_ERR_NUM(err)]

static const char *genavb_GENERAL_error[] = {
	__ERR_NUM(GENAVB_SUCCESS) = "Success",
	__ERR_NUM(GENAVB_ERR_NO_MEMORY) = "Out of memory",
	__ERR_NUM(GENAVB_ERR_ALREADY_INITIALIZED) = "Library already initialized",
	__ERR_NUM(GENAVB_ERR_INVALID) = "Invalid parameters",
	__ERR_NUM(GENAVB_ERR_INVALID_PARAMS) = "Invalid library handle",
	__ERR_NUM(GENAVB_ERR_INVALID_PORT) = "Invalid port",
};

static const char *genavb_STREAM_error[] = {
	__ERR_NUM(GENAVB_ERR_STREAM_API_OPEN) = "Stream open error",
	__ERR_NUM(GENAVB_ERR_STREAM_BIND) = "Stream bind error",
	__ERR_NUM(GENAVB_ERR_STREAM_TX) = "Stream data write error",
	__ERR_NUM(GENAVB_ERR_STREAM_RX) = "Stream data read error",
	__ERR_NUM(GENAVB_ERR_STREAM_INVALID) = "Stream handle invalid",
	__ERR_NUM(GENAVB_ERR_STREAM_PARAMS) = "Invalid stream parameters",
	__ERR_NUM(GENAVB_ERR_STREAM_TX_NOT_ENOUGH_DATA) = "Stream data not enough write error",
	__ERR_NUM(GENAVB_ERR_STREAM_NO_CALLBACK) = "Stream callback not set",
};

static const char *genavb_CTRL_error[] = {
	__ERR_NUM(GENAVB_ERR_CTRL_TRUNCATED) = "Control message truncated",
	__ERR_NUM(GENAVB_ERR_CTRL_INIT) = "Control init error",
	__ERR_NUM(GENAVB_ERR_CTRL_ALLOC) = "Control message allocation error",
	__ERR_NUM(GENAVB_ERR_CTRL_TX) = "Control message write error",
	__ERR_NUM(GENAVB_ERR_CTRL_RX) = "Control message read error",
	__ERR_NUM(GENAVB_ERR_CTRL_LEN) = "Invalid control message length",
	__ERR_NUM(GENAVB_ERR_CTRL_TIMEOUT) = "Control message timeout",
	__ERR_NUM(GENAVB_ERR_CTRL_INVALID) = "Control message incompatible with control channel",
	__ERR_NUM(GENAVB_ERR_CTRL_FAILED) = "Control command failed",
	__ERR_NUM(GENAVB_ERR_CTRL_UNKNOWN) = "Unknown control command",
	__ERR_NUM(GENAVB_ERR_STACK_NOT_READY) = "Stack not ready",
	__ERR_NUM(GENAVB_ERR_PTP_DOMAIN_INVALID) = "Invalid gPTP domain",
};

static const char *genavb_SOCKET_error[] = {
	__ERR_NUM(GENAVB_ERR_SOCKET_INIT) = "Socket open error",
	__ERR_NUM(GENAVB_ERR_SOCKET_PARAMS) = "Socket parameters invalid",
	__ERR_NUM(GENAVB_ERR_SOCKET_AGAIN) = "Socket no data available",
	__ERR_NUM(GENAVB_ERR_SOCKET_INVALID) = "Socket parameters invalid",
	__ERR_NUM(GENAVB_ERR_SOCKET_FAULT) = "Socket invalid address",
	__ERR_NUM(GENAVB_ERR_SOCKET_INTR) = "Socket blocking rx interrupted",
	__ERR_NUM(GENAVB_ERR_SOCKET_TX) = "Socket transmit error",
	__ERR_NUM(GENAVB_ERR_SOCKET_BUFLEN) = "Socket buffer length error",
};

static const char *genavb_CLOCK_error[] = {
	__ERR_NUM(GENAVB_ERR_CLOCK) = "Clock error",
};

static const char *genavb_TIMER_error[] = {
	__ERR_NUM(GENAVB_ERR_TIMER) = "Timer error",
};

static const char *genavb_ST_error[] = {
	__ERR_NUM(GENAVB_ERR_ST) = "Scheduled Traffic config error",
	__ERR_NUM(GENAVB_ERR_ST_NOT_SUPPORTED) = "Scheduled Traffic not supported error",
	__ERR_NUM(GENAVB_ERR_ST_HW_CONFIG) = "Scheduled Traffic hardware config error",
	__ERR_NUM(GENAVB_ERR_ST_MAX_SDU_NOT_SUPPORTED) = "Scheduled Traffic max SDU config error",
};

static const char *genavb_SF_error[] = {
	__ERR_NUM(GENAVB_ERR_SF_NOT_SUPPORTED) = "Stream Filter not supported",
};

static const char *genavb_SG_error[] = {
	__ERR_NUM(GENAVB_ERR_SG_NOT_SUPPORTED) = "Stream Gate not supported",
	__ERR_NUM(GENAVB_ERR_SG_INVALID_CYCLE_PARAMS) = "Stream Gate invalid cycle parameters",
	__ERR_NUM(GENAVB_ERR_SG_INVALID_CYCLE_TIME) = "Stream Gate invalid cycle time",
	__ERR_NUM(GENAVB_ERR_SG_INVALID_BASETIME) = "Stream Gate invalid base time",
	__ERR_NUM(GENAVB_ERR_SG_GETTIME) = "Stream Gate gettime error",
};

static const char *genavb_FM_error[] = {
	__ERR_NUM(GENAVB_ERR_FM_NOT_SUPPORTED) = "Flow Meter not supported",
};

#define GENAVB_ERR_TYPE_STRINGS(type) \
	[GENAVB_ERR_TYPE_ ## type] = { \
		.str = genavb_ ## type ## _error, \
		.num_max = sizeof(genavb_ ## type ## _error), \
	}

static const struct  {
	const char **str;
	unsigned int num_max;
} genavb_error_type_strings[GENAVB_ERR_TYPE_MAX] = {
	GENAVB_ERR_TYPE_STRINGS(GENERAL),
	GENAVB_ERR_TYPE_STRINGS(STREAM),
	GENAVB_ERR_TYPE_STRINGS(CTRL),
	GENAVB_ERR_TYPE_STRINGS(SOCKET),
	GENAVB_ERR_TYPE_STRINGS(CLOCK),
	GENAVB_ERR_TYPE_STRINGS(TIMER),
	GENAVB_ERR_TYPE_STRINGS(ST),
	GENAVB_ERR_TYPE_STRINGS(SF),
	GENAVB_ERR_TYPE_STRINGS(SG),
	GENAVB_ERR_TYPE_STRINGS(FM),
};

const char *genavb_strerror(int error)
{
	unsigned int err, type, num;

	if (error < 0)
		err = -error;
	else
		err = error;

	type = GENAVB_ERR_TYPE(err);
	num = GENAVB_ERR_NUM(err);

	if ((type < GENAVB_ERR_TYPE_MAX) && (num < genavb_error_type_strings[type].num_max))
		return genavb_error_type_strings[type].str[num];
	else
		return "Unknown error code";
}
