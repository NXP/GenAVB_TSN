/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: GPL-2.0-or-later WITH Linux-syscall-note
 */

/**
 @file
 @brief  Linux API implementation in case of toolchain is missing them
 @details
*/

#ifndef _LINUX_HEADERS_EXTENSIONS_H_
#define _LINUX_HEADERS_EXTENSIONS_H_

#include <linux/ptp_clock.h>
#include <linux/version.h>

#if !defined(PTP_CONVERT_TIMESTAMPS)

#pragma message "Building with kernel headers without support to PTP_CONVERT_TIMESTAMPS: add custom definitions"

/* Keep this aligned with the target kernel's include/uapi/linux/ptp_clock.h */
#define PTP_CONVERT_TIMESTAMPS  _IOW(PTP_CLK_MAGIC, 21, struct ptp_convert_timestamps)

#define PTP_MAX_CONVERT_TS_NUM 16 /* Maximum allowed number of timestamps to convert. */

struct ptp_convert_timestamps {
	/* src timestamp to convert in the ptp device clock domain. */
	struct ptp_clock_time src_ts[PTP_MAX_CONVERT_TS_NUM];
	/* converted timestamp in the destination ptp device clock domain. */
	struct ptp_clock_time dst_ts[PTP_MAX_CONVERT_TS_NUM];
	unsigned int dst_phc_index;  /* destination ptp clock domain. */
	unsigned int n_ts; /* Number of timestamps in src_ts array. */
};
#endif

#endif /* _LINUX_HEADERS_EXTENSIONS_H_ */
