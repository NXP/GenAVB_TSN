/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2021, 2023-2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 \file error.c
 \brief GenAVB public API
 \details API definition for the GenAVB library
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
	__ERR_NUM(GENAVB_ERR_NOT_SUPPORTED) = "Feature not supported",
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
	__ERR_NUM(GENAVB_ERR_STREAM_SET_ID) = "Stream invalid set_id configuration or allocation",
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
	__ERR_NUM(GENAVB_ERR_SOCKET_RX_SET_OPT) = "Socket receive set option error",
};

static const char *genavb_CLOCK_error[] = {
	__ERR_NUM(GENAVB_ERR_CLOCK) = "Clock error",
};

static const char *genavb_TIMER_error[] = {
	__ERR_NUM(GENAVB_ERR_TIMER) = "Timer error",
};

static const char *genavb_ST_error[] = {
	__ERR_NUM(GENAVB_ERR_ST_NOT_SUPPORTED) = "Scheduled Traffic not supported",
	__ERR_NUM(GENAVB_ERR_ST) = "Scheduled Traffic config error",
	__ERR_NUM(GENAVB_ERR_ST_HW_CONFIG) = "Scheduled Traffic hardware config error",
	__ERR_NUM(GENAVB_ERR_ST_MAX_SDU_NOT_SUPPORTED) = "Scheduled Traffic max SDU not supported",
	__ERR_NUM(GENAVB_ERR_ST_INVALID_GCL_SIZE) = "Scheduled Traffic invalid control list size",
	__ERR_NUM(GENAVB_ERR_ST_INVALID_CYCLE_PARAMS) = "Scheduled Traffic invalid cycle parameters",
	__ERR_NUM(GENAVB_ERR_ST_INVALID_CYCLE_TIME) = "Scheduled Traffic invalid cycle time",
	__ERR_NUM(GENAVB_ERR_ST_INVALID_BASETIME) = "Scheduled Traffic invalid base time",
	__ERR_NUM(GENAVB_ERR_ST_INVALID_GATE_OPERATION) = "Scheduled Traffic invalid gate operation",
	__ERR_NUM(GENAVB_ERR_ST_ENET_QOS_CONFIG) = "Scheduled Traffic invalid ENET_QOS configuration",
	__ERR_NUM(GENAVB_ERR_ST_MALLOC) = "Scheduled Traffic memory allocation error",
	__ERR_NUM(GENAVB_ERR_ST_HW_READ) = "Scheduled Traffic hardware read error",
	__ERR_NUM(GENAVB_ERR_ST_CONFIG_TYPE_NOT_SUPPORTED) = "Scheduled Traffic configuration type not supported",
	__ERR_NUM(GENAVB_ERR_ST_GETTIME) = "Scheduled Traffic gettime error",
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
	__ERR_NUM(GENAVB_ERR_SG_ADD) = "Stream Gate add entry error",
	__ERR_NUM(GENAVB_ERR_SG_UPDATE) = "Stream Gate update entry error",
	__ERR_NUM(GENAVB_ERR_SG_DELETE) = "Stream Gate delete entry error",
	__ERR_NUM(GENAVB_ERR_SG_MAX_ENTRIES) = "Stream Gate invalid entry id",
	__ERR_NUM(GENAVB_ERR_SG_ENTRY_NOT_FOUND) = "Stream Gate entry not found",
	__ERR_NUM(GENAVB_ERR_SG_GET_ENTRY) = "Stream Gate get entry error",
	__ERR_NUM(GENAVB_ERR_SG_GET_STATE_ENTRY) = "Stream Gate State get entry error",
	__ERR_NUM(GENAVB_ERR_SG_LIST_ADD) = "Stream Gate List add entry error",
	__ERR_NUM(GENAVB_ERR_SG_LIST_DELETE_ADMIN) = "Stream Gate List delete admin entry error",
	__ERR_NUM(GENAVB_ERR_SG_LIST_DELETE_OPER) = "Stream Gate List delete oper entry error",
	__ERR_NUM(GENAVB_ERR_SG_LIST_ENTRY_NOT_FOUND) = "Stream Gate List entry not found",
	__ERR_NUM(GENAVB_ERR_SG_LIST_GET_ADMIN_ENTRY) = "Stream Gate State get admin entry error",
	__ERR_NUM(GENAVB_ERR_SG_LIST_GET_OPER_ENTRY) = "Stream Gate State get oper entry error",
	__ERR_NUM(GENAVB_ERR_SG_LIST_ENTRY_ALLOC_EID) = "Stream Gate List can not be allocated",
	__ERR_NUM(GENAVB_ERR_SG_RESET_IRX_OEX) = "Stream Gate reset IRX OEX error",
	__ERR_NUM(GENAVB_ERR_SG_INVALID_GCL_SIZE) = "Stream Gate invalid control list size",
};

static const char *genavb_FP_error[] = {
	__ERR_NUM(GENAVB_ERR_FP_NOT_SUPPORTED) = "Frame Preemption not supported",
	__ERR_NUM(GENAVB_ERR_FP) = "Frame Preemption config error",
	__ERR_NUM(GENAVB_ERR_FP_HW_CONFIG) = "Frame Preemption hardware config error",
	__ERR_NUM(GENAVB_ERR_FP_INVALID_FRAG_SIZE) = "Frame Preemption invalid fragment size",
	__ERR_NUM(GENAVB_ERR_FP_INVALID_VERIFY_TIME) = "Frame Preemption invalid verify time",
	__ERR_NUM(GENAVB_ERR_FP_INVALID_ENET_QOS_CONFIG) = "Frame Preemption invalid ENET_QOS configuration",
};

static const char *genavb_FM_error[] = {
	__ERR_NUM(GENAVB_ERR_FM_NOT_SUPPORTED) = "Flow Meter not supported",
	__ERR_NUM(GENAVB_ERR_FM_ADD) = "Flow Meter add entry error",
	__ERR_NUM(GENAVB_ERR_FM_UPDATE) = "Flow Meter update entry error",
	__ERR_NUM(GENAVB_ERR_FM_DELETE) = "Flow Meter delete entry error",
	__ERR_NUM(GENAVB_ERR_FM_MAX_ENTRIES) = "Flow Meter invalid entry id",
	__ERR_NUM(GENAVB_ERR_FM_ENTRY_NOT_FOUND) = "Flow Meter entry not found",
	__ERR_NUM(GENAVB_ERR_FM_RESET_MR) = "Flow Meter reset mark red error",
};

static const char *genavb_VLAN_error[] = {
	__ERR_NUM(GENAVB_ERR_VLAN_VID) = "Invalid VID number",
	__ERR_NUM(GENAVB_ERR_VLAN_CONTROL) = "Invalid VLAN control field",
	__ERR_NUM(GENAVB_ERR_VLAN_ENTRY_NOT_FOUND) = "VLAN entry not found",
	__ERR_NUM(GENAVB_ERR_VLAN_HW_CONFIG) = "VLAN hardware config error",
	__ERR_NUM(GENAVB_ERR_VLAN_DEFAULT_NOT_SUPPORTED) = "VLAN port default (PVID) not supported"
};

static const char *genavb_FRER_error[] = {
	__ERR_NUM(GENAVB_ERR_FRER_NOT_SUPPORTED) = "FRER not supported",
	__ERR_NUM(GENAVB_ERR_FRER_MAX_ENTRIES) = "FRER invalid entry id",
	__ERR_NUM(GENAVB_ERR_FRER_ENTRY_NOT_FOUND) = "FRER entry not found",
	__ERR_NUM(GENAVB_ERR_FRER_MAX_STREAM) = "FRER stream number invalid",
	__ERR_NUM(GENAVB_ERR_FRER_PARAMS) = "FRER parameters invalid",
	__ERR_NUM(GENAVB_ERR_FRER_ENTRY_USED) = "FRER entry already used",
	__ERR_NUM(GENAVB_ERR_FRER_HW_CONFIG) = "FRER hardware config error",
	__ERR_NUM(GENAVB_ERR_FRER_HW_READ) = "FRER hardware read error",
};

static const char *genavb_DSA_error[] = {
	__ERR_NUM(GENAVB_ERR_DSA_NOT_SUPPORTED) = "DSA not supported",
	__ERR_NUM(GENAVB_ERR_DSA_NOT_FOUND) = "DSA hardware table entry not found",
	__ERR_NUM(GENAVB_ERR_DSA_HW_CONFIG) = "DSA hardware table config error",
};

static const char *genavb_SI_error[] = {
	__ERR_NUM(GENAVB_ERR_SI_NOT_SUPPORTED) = "Stream Identification not supported",
	__ERR_NUM(GENAVB_ERR_SI_ADD) = "Stream Identification add error",
	__ERR_NUM(GENAVB_ERR_SI_HW_CONFIG) = "Stream Identification hardware config error",
	__ERR_NUM(GENAVB_ERR_SI_UPDATE) = "Stream Identification update error",
	__ERR_NUM(GENAVB_ERR_SI_DELETE) = "Stream Identification delete error",
	__ERR_NUM(GENAVB_ERR_SI_READ) = "Stream Identification read error",
	__ERR_NUM(GENAVB_ERR_SI_MAX_ENTRIES) = "Stream Identification invalid entry id",
	__ERR_NUM(GENAVB_ERR_SI_MAX_HANDLE) = "Stream Identification invalid handle",
	__ERR_NUM(GENAVB_ERR_SI_TYPE_UPDATE_NOT_SUPPORTED) = "Stream Identification type update not supported",
	__ERR_NUM(GENAVB_ERR_SI_VLAN_TYPE_UPDATE_NOT_SUPPORTED) = "Stream Identification vlan type update not supported",
	__ERR_NUM(GENAVB_ERR_SI_HANDLE_UPDATE_NOT_SUPPORTED) = "Stream Identification handle update not supported",
	__ERR_NUM(GENAVB_ERR_SI_ENTRY_NOT_FOUND) = "Stream Identification entry not found",
	__ERR_NUM(GENAVB_ERR_SI_INVALID_STREAM_IDENTITY) = "Stream Identification invalid stream identity",
};

static const char *genavb_HSR_error[] = {
	__ERR_NUM(GENAVB_ERR_HSR_NOT_SUPPORTED) = "High-availability Seamless Redundancy not supported",
};

static const char *genavb_FDB_error[] = {
	__ERR_NUM(GENAVB_ERR_FDB_NOT_SUPPORTED) = "Forwarding Data Base not supported",
	__ERR_NUM(GENAVB_ERR_FDB_ENTRY_NOT_FOUND) = "Forwarding Data Base entry not found",
	__ERR_NUM(GENAVB_ERR_FDB_HW_CONFIG) = "Forwarding Data Base hardware config error",
	__ERR_NUM(GENAVB_ERR_FDB_INVALID_VID) = "Forwarding Data Base invalid VID",
	__ERR_NUM(GENAVB_ERR_FDB_INVALID_PORT) = "Forwarding Data Base invalid logical port",
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
	GENAVB_ERR_TYPE_STRINGS(VLAN),
	GENAVB_ERR_TYPE_STRINGS(FRER),
	GENAVB_ERR_TYPE_STRINGS(DSA),
	GENAVB_ERR_TYPE_STRINGS(FP),
	GENAVB_ERR_TYPE_STRINGS(SI),
	GENAVB_ERR_TYPE_STRINGS(HSR),
	GENAVB_ERR_TYPE_STRINGS(FDB),
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
