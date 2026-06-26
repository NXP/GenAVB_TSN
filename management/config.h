/*
 * Copyright 2019-2021, 2023, 2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief Management static configuration
 @details Contains all compile time configuration options for Management
*/

#ifndef _MANAGEMENT_CFG_H_
#define _MANAGEMENT_CFG_H_

#ifndef _COMPONENT_ID_
#define _COMPONENT_ID_ management_COMPONENT_ID
#define _COMPONENT_STR_ "management"
#endif

#include "common/config.h"

#define CFG_MANAGEMENT_MAX_TIMERS	1

#endif /* _MANAGEMENT_CFG_H_ */
