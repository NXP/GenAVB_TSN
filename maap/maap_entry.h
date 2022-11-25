/*
* Copyright 2021 NXP
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
 @brief MAAP stack component entry points
 @details
*/

#ifndef _MAAP_ENTRY_H_
#define _MAAP_ENTRY_H_

void *maap_init(struct maap_config *cfg, unsigned long priv);
int maap_exit(void *maap_ctx);

#endif /* _MAAP_ENTRY_H_ */
