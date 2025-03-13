/*
 * Copyright 2019-2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief GPTP stack component entry points
 @details Handles all GPTP
*/


#ifndef _GPTP_ENTRY_H_
#define _GPTP_ENTRY_H_

#include "genavb/init.h"

void *gptp_init(struct fgptp_config *cfg, unsigned long priv);
int gptp_exit(void *gptp_ctx);
void gptp_stats_dump(void *gptp_ctx);

#endif /* _GPTP_ENTRY_H_ */
