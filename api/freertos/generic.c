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
 \file generic.c
 \brief generic public API for freertos
 \details
 \copyright Copyright 2020 NXP
*/

#include "genavb/log.h"
#include "genavb/error.h"

#include "common/log.h"
#include "os/net.h"

int genavb_log_level_set(genavb_log_component_id_t id, genavb_log_level_t level)
{
	if (log_level_set(id, level) < 0)
		return -GENAVB_ERR_INVALID;

	return GENAVB_SUCCESS;
}

int genavb_port_stats_get_number(unsigned int port_id)
{
	int size;

	size = net_port_stats_get_number(port_id);
	if (size < 0)
		return -GENAVB_ERR_INVALID;

	return size;
}

int genavb_port_stats_get_strings(unsigned int port_id, char *buf, unsigned int buf_len)
{
	if (!buf)
		return -GENAVB_ERR_INVALID;

	if (net_port_stats_get_strings(port_id, buf, buf_len) < 0)
		return -GENAVB_ERR_INVALID;

	return GENAVB_SUCCESS;
}

int genavb_port_stats_get(unsigned int port_id, uint64_t *buf, unsigned int buf_len)
{
	if (!buf)
		return -GENAVB_ERR_INVALID;

	if (net_port_stats_get(port_id, buf, buf_len) < 0)
		return -GENAVB_ERR_INVALID;

	return GENAVB_SUCCESS;
}
