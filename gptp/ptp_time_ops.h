/*
* Copyright 2015 Freescale Semiconductor, Inc.
* Copyright 2016-2021, 2023, 2025 NXP
*
* SPDX-License-Identifier: BSD-3-Clause
*/

/**
 @file		ptp_time_ops.h
 @brief	PTP Time operations definitions
*/

#ifndef _PTP_TIME_OPS_H_
#define _PTP_TIME_OPS_H_

#include "common/log.h"
#include "common/net.h"
#include "ptp.h"
#include "common/timer.h"
#include "common/types.h"
#include "os/clock.h"

#define PTP_TS_UNSET_U64_VALUE 0xffffffffffffffffULL

#define POW_2_80	(1.208925819614629e24)
#define POW_2_79	(6.044629098073146e23)
#define POW_2_64	(18446744073709551616.0)
#define POW_2_41	(2199023255552.0)
#define POW_2_32	(4294967296.0)
#define POW_2_16	(65536.0)

#define PTP_TIMESTAMP_MAX_SECONDS_MSB	4
u64 pdu_ptp_timestamp_to_u64(struct ptp_timestamp ptp_ts);

void ntoh_ptp_timestamp(struct ptp_timestamp *h_ptp_ts, const struct ptp_timestamp *n_ptp_ts);

void ntoh_scaled_ns(struct ptp_scaled_ns *h_scaled_ns, const struct ptp_scaled_ns *n_scaled_ns);

void hton_scaled_ns(struct ptp_scaled_ns *n_scaled_ns, const struct ptp_scaled_ns *h_scaled_ns);

void scaled_ns_to_u_scaled_ns(struct ptp_u_scaled_ns *u_scaled_ns, const struct ptp_scaled_ns *scaled_ns);

void u_scaled_ns_to_scaled_ns(struct ptp_scaled_ns *scaled_ns, const struct ptp_u_scaled_ns *u_scaled_ns);

/* Two's complement of a 96bit unsigned integer */
void minus_u_scaled_ns(struct ptp_u_scaled_ns *result, const struct ptp_u_scaled_ns *a);

void u64_to_pdu_ptp_timestamp(struct ptp_timestamp *ts, u64 nanoseconds);

void u64_to_u_scaled_ns(struct ptp_u_scaled_ns *u_scaled_ns, u64 u64_ts_ns);

/* This function truncates fractional nanoseconds */
void ptp_extended_timestamp_to_ptp_timestamp(struct ptp_timestamp *ptp_ts, const struct ptp_extended_timestamp *ptp_ext_ts);

void ptp_timestamp_to_ptp_extended_timestamp(struct ptp_extended_timestamp *ptp_ext_ts, const struct ptp_timestamp *ptp_ts);

void ptp_extended_timestamp_to_u_scaled_ns(struct ptp_u_scaled_ns *u_scaled_ns, const struct ptp_extended_timestamp *ptp_ext_ts);

void u_scaled_ns_to_ptp_extended_timestamp(struct ptp_extended_timestamp *ptp_ext_ts, const struct ptp_u_scaled_ns *u_scaled_ns);

void u64_to_ptp_extended_timestamp(struct ptp_extended_timestamp *ptp_ext_ts, u64 u64_ts_ns);

void ptp_timestamp_to_u_scaled_ns(struct ptp_u_scaled_ns *u_scaled_ns, const struct ptp_timestamp *ptp_ts);

void hton_ptp_timestamp(struct ptp_timestamp *pdu_ptp_ts, const struct ptp_timestamp *ptp_ts);

void ptp_double_to_u_scaled_ns(struct ptp_u_scaled_ns *u_scaled_ns, ptp_double double_ts_ns);

void ptp_double_to_scaled_ns(struct ptp_scaled_ns *scaled_ns, ptp_double double_ts_ns);

void ptp_double_to_ptp_extended_timestamp(struct ptp_extended_timestamp *ptp_ext_ts, ptp_double double_ts_ns);

void pdu_correction_field_to_scaled_ns(struct ptp_scaled_ns *scaled_ns, s64 correction_field);

void scaled_ns_to_pdu_correction_field(s64 *correction_field, struct ptp_scaled_ns *scaled_ns);

void u_scaled_ns_add(struct ptp_u_scaled_ns *result, const struct ptp_u_scaled_ns *a, const struct ptp_u_scaled_ns *b);

/* Subtracts two 96bit unsigned integers with correct support for "negative" values */
void u_scaled_ns_sub(struct ptp_u_scaled_ns *result, const struct ptp_u_scaled_ns *a, const struct ptp_u_scaled_ns *b);

/* This function does rounding as well */
void u_scaled_ns_to_u64(u64 *u64_ts_ns, const struct ptp_u_scaled_ns *u_scaled_ns);

/* This function may loose precision */
void u_scaled_ns_to_ptp_double( ptp_double *double_ts_ns, const struct ptp_u_scaled_ns *u_scaled_ns);

/* This function compares two ptp_scaled_ns values a and b. It returns an interger
less than, equal to, or greater than 0 if a is found respectively, to be less, to match, or be greater
than b */
int u_scaled_ns_cmp(const struct ptp_u_scaled_ns *a, const struct ptp_u_scaled_ns *b);

/* This function may loose precision */
void scaled_ns_to_ptp_double( ptp_double *double_ts_ns, const struct ptp_scaled_ns *scaled_ns);

/* This function may loose precision */
void ptp_timestamp_to_ptp_double_ns(ptp_double *double_ts_ns, struct ptp_timestamp ptp_ts);

void scaled_ns_add(struct ptp_scaled_ns *result, struct ptp_scaled_ns a, struct ptp_scaled_ns b);

void double_scaled_ns_mul(struct ptp_scaled_ns *result, ptp_double a, struct ptp_scaled_ns b);

u64 log_to_ns (signed char log_val);

u32 log_to_ms (signed char log_val);

void ptp_time_ops_unit_test(void);

#endif /* _PTP_TIME_OPS_H_ */
