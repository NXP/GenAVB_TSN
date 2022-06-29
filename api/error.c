/*
* Copyright 2014 Freescale Semiconductor, Inc.
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
 \file error.c
 \brief GenAVB public API
 \details API definition for the GenAVB library

 \copyright Copyright 2014 Freescale Semiconductor, Inc.
*/

#include "genavb/error.h"

static const char *genavb_error[] = {
	[GENAVB_SUCCESS] = "Success",
	[GENAVB_ERR_NO_MEMORY] = "Out of memory",
	[GENAVB_ERR_ALREADY_INITIALIZED] = "Library already initialized",
	[GENAVB_ERR_INVALID_PARAMS] = "Invalid parameters",
	[GENAVB_ERR_INVALID] = "Invalid library handle",
	[GENAVB_ERR_STREAM_API_OPEN] = "Stream open error",
	[GENAVB_ERR_STREAM_BIND] = "Stream bind error",
	[GENAVB_ERR_STREAM_TX] = "Stream data write error",
	[GENAVB_ERR_STREAM_RX] = "Stream data read error",
	[GENAVB_ERR_STREAM_INVALID] = "Stream handle invalid",
	[GENAVB_ERR_STREAM_PARAMS] = "Invalid stream parameters",
	[GENAVB_ERR_STREAM_TX_NOT_ENOUGH_DATA] = "Stream data not enough write error",
	[GENAVB_ERR_STREAM_NO_CALLBACK] = "Stream callback not set",
	[GENAVB_ERR_CTRL_TRUNCATED] = "Control message truncated",
	[GENAVB_ERR_CTRL_INIT] = "Control init error",
	[GENAVB_ERR_CTRL_ALLOC] = "Control message allocation error",
	[GENAVB_ERR_CTRL_TX] = "Control message write error",
	[GENAVB_ERR_CTRL_RX] = "Control message read error",
	[GENAVB_ERR_CTRL_LEN] = "Invalid control message length",
	[GENAVB_ERR_CTRL_TIMEOUT] = "Control message timeout",
	[GENAVB_ERR_CTRL_INVALID] = "Control message incompatible with control channel",
	[GENAVB_ERR_CTRL_FAILED] = "Control command failed",
	[GENAVB_ERR_CTRL_UNKNOWN] = " Unknown control command",
	[GENAVB_ERR_STACK_NOT_READY] = "Stack not ready",
	[GENAVB_ERR_SOCKET_INIT] = "Socket open error",
	[GENAVB_ERR_SOCKET_PARAMS] = "Socket parameters invalid",
	[GENAVB_ERR_SOCKET_AGAIN] = "Socket no data available",
	[GENAVB_ERR_SOCKET_INVALID] = "Socket parameters invalid",
	[GENAVB_ERR_SOCKET_FAULT] = "Socket invalid address",
	[GENAVB_ERR_SOCKET_INTR] = "Socket blocking rx interrupted",
	[GENAVB_ERR_SOCKET_TX] = "Socket transmit error",
	[GENAVB_ERR_SOCKET_BUFLEN] = "Socket buffer length error",
	[GENAVB_ERR_CLOCK] = "Clock error",
	[GENAVB_ERR_TIMER] = "Timer error",
	[GENAVB_ERR_ST] = "Scheduled Traffic config error",
	[GENAVB_ERR_PTP_DOMAIN_INVALID] = "Invalid gPTP domain",
};

const char *genavb_strerror(int error)
{
	if (error < 0)
		error = -error;

	if (error < GENAVB_ERR_CTRL_MAX)
		return genavb_error[error];
	else
		return "Unknown error code";
}
