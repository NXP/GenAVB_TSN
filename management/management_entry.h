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
 @file
 @brief Management stack component entry points
 @details
 */

#ifndef _MANAGEMENT_ENTRY_H_
#define _MANAGEMENT_ENTRY_H_

#include "genavb/init.h"

void *management_init(struct management_config *cfg, unsigned long priv);
int management_exit(void *management_ctx);

#endif /* _MANAGEMENT_ENTRY_H_ */
