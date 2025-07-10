/*
 * Copyright 2019, 2021, 2023, 2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief Generic Clock functions implementation
 @details
*/

#include "common/clock.h"
#include "common/log.h"

int clock_set_time64(os_clock_id_t clk_id, u64 ns)
{
	u64 curr;
	s64 off;
	int rc;

	rc = os_clock_gettime64(clk_id, &curr);
	if (rc)
		goto exit;

	off = ns - curr;

	rc = os_clock_setoffset(clk_id, off);
exit:
	return rc;
}

const char *os_clock_id2string(os_clock_id_t id)
{
	switch (id) {
	case2str(OS_CLOCK_SYSTEM_MONOTONIC);
	case2str(OS_CLOCK_SYSTEM_MONOTONIC_COARSE);
	case2str(OS_CLOCK_MEDIA_HW_0);
	case2str(OS_CLOCK_MEDIA_HW_1);
	case2str(OS_CLOCK_MEDIA_REC_0);
	case2str(OS_CLOCK_MEDIA_REC_1);
	case2str(OS_CLOCK_MEDIA_PTP_0);
	case2str(OS_CLOCK_MEDIA_PTP_1);
	case2str(OS_CLOCK_GPTP_EP_0_0);
	case2str(OS_CLOCK_GPTP_EP_0_1);
	case2str(OS_CLOCK_GPTP_EP_1_0);
	case2str(OS_CLOCK_GPTP_EP_1_1);
	case2str(OS_CLOCK_GPTP_BR_0_0);
	case2str(OS_CLOCK_GPTP_BR_0_1);
	case2str(OS_CLOCK_LOCAL_EP_0);
	case2str(OS_CLOCK_LOCAL_EP_1);
	case2str(OS_CLOCK_LOCAL_BR_0);
	case2str(OS_CLOCK_SYSTEM_MONOTONIC_1);
	case2str(OS_CLOCK_AVTP_MEDIA_0);
	case2str(OS_CLOCK_AVTP_MEDIA_1);
	default:
		return (char *) "Unknown clock ID";
	}
}
