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
 @brief Linux epoll implementation
 @details
*/

#include <string.h>
#include <errno.h>

#include "epoll.h"
#include "common/log.h"

int epoll_ctl_add(int epoll_fd, int fd, epoll_type_t type, void *ptr, struct linux_epoll_data *data, unsigned int event_type)
{
	struct epoll_event event;

	data->type = type;
	data->ptr = ptr;

	event.events = event_type;
	event.data.ptr = data;

	if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &event) < 0) {
		os_log(LOG_ERR, "epoll_ctl() failed with errno(%d): %s\n", errno, strerror(errno));
		return -1;
	}

	return 0;
}

int epoll_ctl_del(int epoll_fd, int fd)
{
	if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL) < 0) {
		os_log(LOG_ERR, "epoll_ctl() failed with errno(%d): %s\n", errno, strerror(errno));
		return -1;
	}

	return 0;
}

