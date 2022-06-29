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
 @brief Linux specific epoll data stuctures
 @details
*/

#ifndef _LINUX_OSAL_EPOLL_H_
#define _LINUX_OSAL_EPOLL_H_

typedef enum {
	EPOLL_TYPE_TIMER,
	EPOLL_TYPE_DESC,
	EPOLL_TYPE_NET_RX,
	EPOLL_TYPE_IPC,
	EPOLL_TYPE_MEDIA,
	EPOLL_TYPE_NET_TX_TS,
	EPOLL_TYPE_NET_TX_EVENT,
} epoll_type_t;


struct linux_epoll_data {
	epoll_type_t type;
	void *ptr;
};

#endif /* _LINUX_OSAL_EPOLL_H_ */
