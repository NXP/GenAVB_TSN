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
 @brief Linux Standard BPF socket filters definitions
 @details
*/
#ifndef _LINUX_SOCKET_FILTERS_H_
#define _LINUX_SOCKET_FILTERS_H_

#include "genavb/net_types.h"

#define BPF_FILTER_MAX_ARRAY_SIZE			16 /* Keep it greater or equal to the max of the filter arrays size */

int sock_filter_get_bpf_code(struct net_address *addr, void *buf, unsigned int *inst_count);

#endif /* _LINUX_SOCKET_FILTERS_H_ */
