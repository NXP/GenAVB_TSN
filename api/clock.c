/*
* Copyright 2019-2020 NXP
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
 \file clock.c
 \brief GenAVB public API
 \details API definition for the GenAVB library

 \copyright Copyright 2019-2020 NXP
*/

#include "clock.h"

#include "genavb/error.h"

static const os_clock_id_t public_clock_to_os_clock[] = {
	[GENAVB_CLOCK_MONOTONIC] = OS_CLOCK_SYSTEM_MONOTONIC,
	[GENAVB_CLOCK_GPTP_0_0] = OS_CLOCK_GPTP_EP_0_0,
	[GENAVB_CLOCK_GPTP_0_1] = OS_CLOCK_GPTP_EP_0_1,
	[GENAVB_CLOCK_GPTP_1_0] = OS_CLOCK_GPTP_EP_1_0,
	[GENAVB_CLOCK_GPTP_1_1] = OS_CLOCK_GPTP_EP_1_1,
};

os_clock_id_t genavb_clock_to_os_clock(genavb_clock_id_t id)
{
	if (id >= GENAVB_CLOCK_MAX)
		return OS_CLOCK_MAX;

	return public_clock_to_os_clock[id];
}

int genavb_clock_gettime64(genavb_clock_id_t id, uint64_t *ns)
{
	if (id >= GENAVB_CLOCK_MAX || !ns)
		return -GENAVB_ERR_INVALID;

	if (os_clock_gettime64(public_clock_to_os_clock[id], ns) < 0)
		return -GENAVB_ERR_CLOCK;

	return GENAVB_SUCCESS;
}

