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
 @brief Linux specific epoll implementation
 @details
*/

#ifndef _LINUX_EPOLL_H_
#define _LINUX_EPOLL_H_

#include "osal/epoll.h"
#include "sys/epoll.h"


int epoll_ctl_add(int epoll_fd, int fd, epoll_type_t type, void *ptr, struct linux_epoll_data *data, unsigned int event_type);
int epoll_ctl_del(int epoll_fd, int fd);

#endif /* _LINUX_EPOLL_H_ */
