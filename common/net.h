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
 @brief Networking services
 @details Networking services implementation
*/

#ifndef _NET_H_
#define _NET_H_

#include "os/string.h"
#include "os/net.h"

#include "common/types.h"
#include "common/net_types.h"
#include "common/ether.h"

#define NTOH_MAC_VALUE(x)	ntohll(get_48(x))

#define NET_DATA_START(desc) ((void *)((char *)(desc) + (desc)->l2_offset))

static inline u16 net_add_eth_header(void *buf, const u8 *mac_dst, u16 ethertype)
{
	struct eth_hdr *eth = (struct eth_hdr *)buf;

	eth->type = htons(ethertype);

	/*The source mac address will be added by the kernel network layer itself */
	os_memcpy(eth->dst, mac_dst, 6);

	return sizeof(struct eth_hdr);
}

static inline u16 net_add_vlan_header(void *buf, u16 ethertype, u16 vid, u8 pcp, u8 cfi)
{
	struct vlan_hdr *vlan = (struct vlan_hdr *)buf;

	vlan->type = htons(ethertype);
	vlan->label = VLAN_LABEL(vid, pcp, cfi);

	return sizeof(struct vlan_hdr);
}

static inline void net_eui64_from_mac(u8 *eui, u8 *mac_addr, u8 mod)
{
	eui[0] = mac_addr[0];
	eui[1] = mac_addr[1];
	eui[2] = mac_addr[2];
	eui[3] = 0xff;
	eui[4] = 0xfe;
	eui[5] = mac_addr[3];
	eui[6] = mac_addr[4];
	eui[7] = mac_addr[5] + mod;
}

#endif /* _NET_H_ */
