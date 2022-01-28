/*
* Copyright 2018, 2020 NXP
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
 @brief Linux specific media abstraction implementation
 @details
*/

#ifndef _LINUX_OSAL_MEDIA_H_
#define _LINUX_OSAL_MEDIA_H_

#include "epoll.h"

struct media_rx {
	int fd;
	int epoll_fd;
	struct linux_epoll_data epoll_data;
};

struct media_tx {
	int fd;
};

#endif /* _LINUX_OSAL_MEDIA_H_ */
