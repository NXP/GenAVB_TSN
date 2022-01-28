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
 \file qos.c
 \brief qos public API for freertos
 \details
 \copyright Copyright 2020 NXP
*/

#include "api/clock.h"
#include "genavb/qos.h"
#include "genavb/error.h"

#include "os/qos.h"

int genavb_st_set_admin_config(unsigned int port_id, genavb_clock_id_t clk_id,
			       struct genavb_st_config *config)
{
	os_clock_id_t os_id;
	int rc = GENAVB_SUCCESS;

	if (!config) {
		rc = -GENAVB_ERR_INVALID;
		goto out;
	}

	if (config->enable &&
	   (!config->list_length || !config->control_list)) {
		rc = -GENAVB_ERR_INVALID;
		goto out;
	}

	if (clk_id >= GENAVB_CLOCK_MAX) {
		rc = -GENAVB_ERR_CLOCK;
		goto out;
	}

	os_id = genavb_clock_to_os_clock(clk_id);
	if (os_id >= OS_CLOCK_MAX) {
		rc = -GENAVB_ERR_CLOCK;
		goto out;
	}

	if (qos_st_set_admin_config(port_id, os_id, config) < 0) {
		rc = -GENAVB_ERR_ST;
		goto out;
	}

out:
	return rc;
}

int genavb_st_get_config(unsigned int port_id, genavb_st_config_type_t type,
			 struct genavb_st_config *config, unsigned int list_length)
{
	int rc = GENAVB_SUCCESS;

	if (!config || !config->control_list) {
		rc = -GENAVB_ERR_INVALID;
		goto out;
	}

	if (qos_st_get_config(port_id, type, config, list_length) < 0) {
		rc = -GENAVB_ERR_ST;
		goto out;
	}

out:
	return rc;
}

int genavb_fp_set(unsigned int port_id, genavb_fp_config_type_t type, struct genavb_fp_config *config)
{
	int rc = GENAVB_SUCCESS;

	if (!config) {
		rc = -GENAVB_ERR_INVALID;
		goto out;
	}

	if (qos_fp_set(port_id, type, config) < 0) {
		rc = -GENAVB_ERR_INVALID;
		goto out;
	}

out:
	return rc;
}

int genavb_fp_get(unsigned int port_id, genavb_fp_config_type_t type, struct genavb_fp_config *config)
{
	int rc = GENAVB_SUCCESS;

	if (!config) {
		rc = -GENAVB_ERR_INVALID;
		goto out;
	}

	if (qos_fp_get(port_id, type, config) < 0) {
		rc = -GENAVB_ERR_INVALID;
		goto out;
	}

out:
	return rc;
}
