/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2023, 2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file		ptp.h
 @brief	PTP protocol common definitions
 @details	PDU and protocol definitions for all PTP applications
*/

#ifndef _COMMON_PTP_H_
#define _COMMON_PTP_H_

#include "genavb/ptp.h"
#include "common/types.h"
#include "common/filter.h"
#include "common/timer.h"

/*
* PTP domain (802.1AS-2020 -8.1)
*/
#define PTP_DOMAIN_0 0
#define PTP_DOMAIN_NUMBER_MAX	127
#define PTP_DOMAIN_MINOR_SDOID	0x00
#define PTP_DOMAIN_MAJOR_SDOID	0x1

typedef double ptp_double;

/*
* Port Roles (802.1AS - Table 10.1 and Table 14.5) */
typedef enum {
	DISABLED_PORT = 3,	/** any port of the time-aware system for which portEnabled, ptpPortEnabled, and asCapable are not all TRUE */
	MASTER_PORT = 6,	/** any port, P, of the time aware system that is closer than any other port of the gptp communication path connected to P */
	PASSIVE_PORT = 7,	/** any port of the time-aware system whose port role is not MasterPort, SlavePort or DisabledPort */
	SLAVE_PORT = 9		/** the one port of the time-aware system that is closest to the root time-aware system. Does not transmit announce or sync messages */
} ptp_port_role_t;


/*
* Delay mechanism (802.1AS-2020 - Table 14.8)
*/
typedef enum {
	P2P = 2, /* The port uses instance-specific peer-top-peer delay mechanism*/
	COMMON_P2P = 3, /* The port uses CMLDS */
	SPECIAL = 4 /*The port uses transport with native time transfer mechanism. No peer-to-peer delay meachism*/
} ptp_delay_mechanism_t;

#endif /* _COMMON_PTP_H_ */
