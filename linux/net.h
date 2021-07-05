/*
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
 @brief Linux specific Network service implementation
 @details
*/

#ifndef _LINUX_NET_H_
#define _LINUX_NET_H_

#include "os/sys_types.h"

int net_init(void);
void net_exit(void);

int net_std_port_status(unsigned int port_id, bool *up, bool *point_to_point, unsigned int *rate);
int net_port_sr_config(unsigned int port_id, uint8_t *sr_class);

#endif /* _LINUX_NET_H_ */
