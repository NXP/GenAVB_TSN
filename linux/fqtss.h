/*
* Copyright 2021, 2023-2025 NXP
*
* SPDX-License-Identifier: BSD-3-Clause
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
	int (*fqtss_set_oper_idle_slope)(unsigned int port_id, uint8_t traffic_class, uint64_t idle_slope);
	int (*fqtss_stream_add)(unsigned int port_id, void *stream_id, uint16_t vlan_id, uint8_t priority, uint64_t idle_slope);
	int (*fqtss_stream_remove)(unsigned int port_id, void *stream_id, uint16_t vlan_id, uint8_t priority, uint64_t idle_slope);
};

#endif /* _LINUX_FQTSS_H_ */
