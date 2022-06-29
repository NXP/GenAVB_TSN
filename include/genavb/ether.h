/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *    Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *    Neither the name of NXP Semiconductors nor the
 *    names of its contributors may be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 \file ether.h
 \brief GenAVB public API
 \details Ethernet header definitions.

 \copyright Copyright 2015 Freescale Semiconductor, Inc.
*/
#ifndef _GENAVB_PUBLIC_ETHER_H_
#define _GENAVB_PUBLIC_ETHER_H_

#include "types.h"

/**
 * \ingroup protocol
 */
struct eth_hdr
{
	avb_u8 dst[6];
	avb_u8 src[6];
	avb_u16 type;
};

/**
 * \ingroup protocol
 */
struct vlanhdr
{
	avb_u16 label;
	avb_u16 type;
};

#ifdef __BIG_ENDIAN__
#define VLAN_PCP_SHIFT	13
#define VLAN_CFI_SHIFT	12
#else
#define VLAN_PCP_SHIFT	5
#define VLAN_CFI_SHIFT	4
#endif

#define VLAN_TCI_PCP_MASK	0x7
#define VLAN_TCI_CFI_MASK	0x1
#define VLAN_TCI_VID_MASK	0xfff

#define MAC_IS_MCAST(mac)  ((mac)[0] & 0x1)

#define VLAN_PCP(vlan)  (((vlan)->label >> VLAN_PCP_SHIFT) & VLAN_TCI_PCP_MASK)
#define VLAN_CFI(vlan)  (((vlan)->label >> VLAN_CFI_SHIFT) & VLAN_TCI_CFI_MASK)
#define VLAN_VID(vlan)  (ntohs((vlan)->label) & VLAN_TCI_VID_MASK)
#define VLAN_LABEL(vid, pcp, cfi)  (htons((vid) & VLAN_TCI_VID_MASK) | (((pcp) & VLAN_TCI_PCP_MASK) << VLAN_PCP_SHIFT) | (((cfi) & VLAN_TCI_CFI_MASK) << VLAN_CFI_SHIFT))

/**
 * \ingroup socket
 * @{
 */
/* 802.1Q-2011, 9.6 */
#define VLAN_VID_MIN		0
#define VLAN_VID_MAX		4094
#define VLAN_VID_DEFAULT	0xfffe /**< non standard value, used to indicate default vlan vid */
#define VLAN_VID_NONE		0xffff /**< non standard value, used to indicate no vlan tag */

/** @} */

#define ETHERTYPE_SJAMETA 0x0008  /* specifig ethertype (IEEE 802.3 - frame length) used by the SJA110/HAL platform */
#define ETHERTYPE_IPV4	0x0800
#define ETHERTYPE_MSRP	0x22ea
#define ETHERTYPE_AVTP	0x22f0
#define ETHERTYPE_VLAN	0x8100
#define ETHERTYPE_IPV6	0x86DD
#define ETHERTYPE_MVRP	0x88f5
#define ETHERTYPE_MMRP	0x88f6
#define ETHERTYPE_PTP	0x88f7

#define MC_ADDR_AVDECC_ADP_ACMP		{0x91, 0xe0, 0xf0, 0x01, 0x00, 0x00}
#define MC_ADDR_MAAP_BASE		{0x91, 0xe0, 0xf0, 0x00, 0xfe, 0x00}	/* MAAP locally administered pool, per IEEE1722-2016 Annex B section 4 */
#define MC_ADDR_MSRP			{0x01, 0x80, 0xc2, 0x00, 0x00, 0x0e}
#define MC_ADDR_MMRP			{0x01, 0x80, 0xc2, 0x00, 0x00, 0x20}
#define MC_ADDR_MVRP			{0x01, 0x80, 0xc2, 0x00, 0x00, 0x21}
#define MC_ADDR_PTP			{0x01, 0x80, 0xc2, 0x00, 0x00, 0x0e}

#define ETHER_MTU		1500
#define ETHER_MIN_FRAME_SIZE	64	/* including fcs */

#define ETHER_FCS              4

#define ETHER_IFG              12
#define ETHER_PREAMBLE         8

#define ETHER_ETYPE_OFFSET	12
#define ETHER_VLAN_LABEL_OFFSET	14

#endif /* _GENAVB_PUBLIC_ETHER_H_ */
