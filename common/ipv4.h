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
 @file		ipv4.h
 @brief   	IPv4 protocol common definitions
 @details	Protocol and Header definitions
*/

#ifndef _IPV4_H_
#define _IPV4_H_

#include "genavb/ipv4.h"
#include "common/net.h"

static inline u16 net_add_ipv4_header(void *buf, u32 saddr, u32 daddr, u8 protocol)
{
	struct ipv4_hdr *ipv4 = (struct ipv4_hdr *)buf;

	ipv4->version = 4;
	ipv4->ihl = 5;
	ipv4->tos = 0;
	ipv4->tot_len = htons(0);
	ipv4->id = htons(0);
	ipv4->frag_off = htons(0);
	ipv4->ttl = 1;
	ipv4->protocol = protocol;
	ipv4->checksum = htons(0);
	ipv4->saddr = saddr;
	ipv4->daddr = daddr;

	return sizeof(struct ipv4_hdr);
}

#endif /* _IPV4_H_ */
