/*
* Copyright 2014 Freescale Semiconductor, Inc.
* Copyright 2020 NXP
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
 @brief Linux specific IPC service implementation
 @details
*/

#ifndef _LINUX_OSAL_IPC_H_
#define _LINUX_OSAL_IPC_H_

#include "epoll.h"

#define DEFAULT_IPC_DATA_SIZE	1024

struct ipc_rx {
	int fd;
	void *mmap_baseaddr;
	unsigned long pool_size;
	int epoll_fd;
	void (*func)(struct ipc_rx const *, struct ipc_desc *);
	struct linux_epoll_data epoll_data;
};

struct ipc_tx {
	int fd;
	void *mmap_baseaddr;
	unsigned long pool_size;
};

#endif /* _LINUX_OSAL_IPC_H_ */
