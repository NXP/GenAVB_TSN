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
 @brief SRP stack component entry points
 @details
 */

#ifndef _SRP_ENTRY_H_
#define _SRP_ENTRY_H_

#include "genavb/init.h"

void *srp_init(struct srp_config *cfg, unsigned long priv);
int srp_exit(void *srp_h);

#endif /* _SRP_ENTRY_H_ */
