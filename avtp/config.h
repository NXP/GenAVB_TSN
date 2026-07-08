/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016, 2018-2019, 2021-2023, 2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief AVTP static configuration
 @details Contains all compile time configuration options for avtp
*/

#ifndef _AVTP_CFG_H_
#define _AVTP_CFG_H_

#ifndef _COMPONENT_ID_
#define _COMPONENT_ID_ avtp_COMPONENT_ID
#define _COMPONENT_STR_ "avtp"
#endif

#include "common/config.h"

#define CFG_AVTP_MAX_TIMERS	2	/* one per CRF stream */

#define CFG_AVTP_61883_6_MAX_CHANNELS	32
#define CFG_AVTP_AAF_PCM_MAX_CHANNELS	32
#define CFG_AVTP_AAF_PCM_MAX_SAMPLES	256  /* Matches 1 packet per interval for SR Class C at 192KHz and SR Class D at 176.4KHz */
#define CFG_AVTP_AAF_AES3_MAX_STREAMS	10
#define CFG_AVTP_AAF_AES3_MAX_FRAMES	256  /* Matches 1 packet per interval for SR Class C at 192KHz and SR Class D at 176.4KHz */

#endif /* _AVTP_CFG_H_ */
