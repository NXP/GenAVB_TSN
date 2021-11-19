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
 @file		udp.h
 @brief	UDP protocol common definitions
 @details	Protocol and Header definitions
*/

#ifndef _UDP_H_
#define _UDP_H_

#include "genavb/udp.h"
#include "common/net.h"

static inline u16 net_add_udp_header(void *buf, u16 sport, u16 dport)
{
	struct udp_hdr *udp = (struct udp_hdr *)buf;

	udp->sport = sport;
	udp->dport = dport;
	udp->length = htons(0);
	udp->checksum = htons(0);

	return sizeof(struct udp_hdr);
}

#endif /* _UDP_H_ */
