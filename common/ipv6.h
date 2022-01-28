/*
* Copyright 2015 Freescale Semiconductor, Inc.
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
 @file		ipv6.h
 @brief   	IPv6 protocol common definitions
 @details	Protocol and Header definitions
*/

#ifndef _IPV6_H_
#define _IPV6_H_

#include "genavb/ipv6.h"

static inline u16 net_add_ipv6_header(void *buf, u32 const *saddr, u32 const *daddr, u8 protocol)
{
	struct ipv6_hdr *ipv6 = (struct ipv6_hdr *)buf;

	IPV6_SET_VERSION(ipv6, 6);
	IPV6_SET_TRAFFIC_CLASS(ipv6, 0);
	IPV6_SET_FLOW_LABEL(ipv6, 0);
	ipv6->payload_len = htons(0);
	ipv6->nexthdr = protocol;
	ipv6->hop_limit = 1;
	os_memcpy(ipv6->saddr, saddr, 16);
	os_memcpy(ipv6->daddr, daddr, 16);

	return sizeof(struct ipv6_hdr);
}

#endif /* _IPV6_H_ */
