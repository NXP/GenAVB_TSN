/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016, 2019-2021, 2023-2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 \file ptp.h
 \brief GenAVB/TSN public API
 \details ptp header definitions.
*/
#ifndef _GENAVB_PUBLIC_PTP_H_
#define _GENAVB_PUBLIC_PTP_H_

#include "types.h"

/**
 * ScaledNs (802.1AS - 6.3.3.1)
 * \ingroup protocol
 */
struct __attribute__ ((packed)) ptp_scaled_ns {
	union {
		avb_u8 scaled_nanoseconds[12];
		struct __attribute__ ((packed)) {
			/* Keep all values has unsigned and do unsigned math */
			avb_u16 nanoseconds_msb;			/* Should remain 0 for about 500 years (2**64/NSECS_PER_SEC/SECS_PER_DAY/DAYS_PER_YEAR = 584), so will be ignored in all computations */
			avb_u64 nanoseconds;
			avb_u16 fractional_nanoseconds;
		} s;
	} u;
};

/**
 * UScaledNs (802.1AS - 6.3.3.2)
 * \ingroup protocol
 */
struct __attribute__ ((packed)) ptp_u_scaled_ns {
	union {
		avb_u8 u_scaled_nanoseconds[12];
		struct __attribute__ ((packed)) {
			avb_u16 nanoseconds_msb;			/* Should remain 0 for about 500 years (2**64/NSECS_PER_SEC/SECS_PER_DAY/DAYS_PER_YEAR = 584), so will be ignored in all computations */
			avb_u64 nanoseconds;
			avb_u16 fractional_nanoseconds;
		} s;
	} u;
};

/**
 * GM Clock Identity (802.1AS - Table 10.7)
 * \ingroup protocol
 */
struct __attribute__ ((packed)) ptp_clock_identity {
	avb_u8 identity[8];
};

/**
 * \ingroup protocol
 */
struct __attribute__ ((packed)) ptp_port_identity {
	avb_u8 clock_identity[8];
	avb_u16 port_number;
};

/**
 * PTP Message Header (802.1AS - Table 10.4)
 * \ingroup protocol
 */
struct __attribute__ ((packed)) ptp_hdr {
#ifdef __BIG_ENDIAN__
	avb_u8 transport_specific:4;
	avb_u8 msg_type:4;

	avb_u8 minor_version_ptp:4;
	avb_u8 version_ptp:4;
#else
	avb_u8 msg_type:4;
	avb_u8 transport_specific:4;

	avb_u8 version_ptp:4;
	avb_u8 minor_version_ptp:4;
#endif

	avb_u16 msg_length;
	avb_u8 domain_number;
	avb_u8 reserved1;
	avb_u16 flags;
	avb_s64 correction_field;
	avb_u32 reserved2;
	struct ptp_port_identity source_port_id;
	avb_u16 sequence_id;
	avb_u8 control;
	avb_s8 log_msg_interval;
};

#endif /* _GENAVB_PUBLIC_PTP_H_ */
