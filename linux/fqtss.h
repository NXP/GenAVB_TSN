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
 @brief Linux specific FQTSS service implementation
 @details
*/

#ifndef _LINUX_FQTSS_H_
#define _LINUX_FQTSS_H_

#include "os/sys_types.h"

struct fqtss_ops_cb {
	void (*fqtss_exit)(void);
	int (*fqtss_set_oper_idle_slope)(unsigned int, uint8_t, unsigned int);
	int (*fqtss_stream_add)(unsigned int, void *, uint16_t, uint8_t, unsigned int);
	int (*fqtss_stream_remove)(unsigned int, void *, uint16_t, uint8_t, unsigned int);
};

#endif /* _LINUX_FQTSS_H_ */
