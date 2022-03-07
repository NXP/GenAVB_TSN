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
 @brief AVDECC stack component entry points
 @details
 */

#ifndef _AVDECC_ENTRY_H_
#define _AVDECC_ENTRY_H_

#include "genavb/init.h"

void *avdecc_init(struct avdecc_config *cfg, unsigned long priv);
int avdecc_exit(void *avdecc_h);

#endif /* _AVDECC_ENTRY_H_ */
