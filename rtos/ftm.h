/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief AVB FTM driver
 @details
*/

#ifndef _FTM_H_
#define _FTM_H_

#include "config.h"

__init int ftm_driver_init(void);
__exit void ftm_driver_exit(void);

#endif /* _FTM_H_ */