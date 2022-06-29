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
 \file net_types.h
 \brief GenAVB public API
 \details net_types header definitions.

 \copyright Copyright 2015 Freescale Semiconductor, Inc.
*/
#ifndef _GENAVB_PUBLIC_NET_TYPES_H_
#define _GENAVB_PUBLIC_NET_TYPES_H_

#include "config.h"
#include "types.h"
#include "os/net_types.h"

#define NET_RX_BATCH	16
#define NET_TX_BATCH	32

#define NET_TX_FLAGS_HW_TS	(1 << 0)
#define NET_TX_FLAGS_HW_CSUM	(1 << 1)
#define NET_TX_FLAGS_TS		(1 << 2)
#define NET_TX_FLAGS_PARTIAL	(1 << 3)
#define NET_TX_FLAGS_END_FRAME	(1 << 6)

#define NULL_MAC {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}

#ifndef ntohs

#if defined(__BIG_ENDIAN__)

#define ntohs(x) (x)
#define ntohl(x) (x)
#define htons(x) (x)
#define htonl(x) (x)

#elif defined(__LITTLE_ENDIAN__)

#define ntohs(x) ((((x) & 0xFF00) >> 8) | (((x) & 0xFF) << 8))
#define ntohl(x) ((((x) & 0xFF000000) >> 24) | (((x) & 0xFF) << 24) | \
		(((x) & 0xFF0000) >> 8) | (((x) & 0xFF00) << 8))
#define htons(x) ((((x) & 0xFF00) >> 8) | (((x) & 0xFF) << 8))
#define htonl(x) ((((x) & 0xFF000000) >> 24) | (((x) & 0xFF) << 24) | \
		(((x) & 0xFF0000) >> 8) | (((x) & 0xFF00) << 8))
#else
	#error Machine endianness is not properly defined, it shall be either BIG or LITTLE
#endif

#endif /* ntohs */


#if defined(__BIG_ENDIAN__)

#define ntohll(x) (x)
#define htonll(x) (x)

#elif defined(__LITTLE_ENDIAN__)

#define htonll(x) (((avb_u64)htonl((avb_u32)(x)))<<32 | htonl((avb_u32)((avb_u64)(x)>>32)))
#define ntohll(x) (((avb_u64)ntohl((avb_u32)(x)))<<32 | ntohl((avb_u32)((avb_u64)(x)>>32)))

#endif /* __X_ENDIAN__ */


struct net_ts_desc {
	avb_u64 ts;
	avb_u32 priv;
};



/*  NOTE: l2_offset should be at least equal to the size of  net_rx_desc  structure
 *  These fields are common cross layers and should not be overriden while transferred*/
#define NET_DESC_COMMON \
	avb_u16 l2_offset; \
	avb_u16 len; \
	avb_u32 ts; \
	avb_u32 flags; \
	avb_u32 priv

/** Network receive descriptor
 *
 */
struct net_rx_desc {
	NET_DESC_COMMON;
	avb_u16 port;
	avb_u16 l3_offset; /**< offset of mrp, avtp, ptp header (with vlan skipped) */
	avb_u16 l4_offset; /**< offset of udp header */
	avb_u16 l5_offset; /**< offset of rtp header (with vlan) */
	avb_u16 ethertype;
	avb_u16 vid;
	avb_u64 ts64;
};

/** Network transmit descriptor
 *
 */
struct net_tx_desc {
	NET_DESC_COMMON;
	avb_u16 port;
	avb_u64 ts64;
};

/** Network address
 * \ingroup socket
 * The protocol type determines the format of the network address
 *
 */
struct net_address {
	avb_u8 ptype;	/**< protocol type */
	avb_u16 port;	/**< port */
	avb_u16 vlan_id;	/**< vlan id (network order), one of [VLAN_VID_MIN, VLAN_VID_MAX], VLAN_VID_NONE or VLAND_ID_DEFAULT */
	avb_u8 priority;	/**< traffic priority */

	union {
		struct avtp_address {
			int subtype; /* AECP, ADP, ACMP, MAAP, 611883-IDC, ... */
			avb_u8 sr_class;
			avb_u8 stream_id[8];
		} avtp;

		struct {
			avb_u8 version;
		} ptp;

		struct {
			avb_u8 proto; /* UDP */
			avb_u32 saddr;
			avb_u32 daddr;
			avb_u16 sport;
			avb_u16 dport;

			int l5_proto; /* RTP, RTCP, ALL */
			union {
				struct {
					avb_u8 sr_class;
					avb_u8 stream_id[8];
					avb_u8 pt;
					avb_u32 ssrc;
				} rtp;

				struct {
					avb_u8 pt;
					avb_u32 ssrc;
				} rtcp;
			} u;
		} ipv4;

		struct {
			avb_u8 proto; /* UDP */
			avb_u32 saddr[4];
			avb_u32 daddr[4];
			avb_u16 sport;
			avb_u16 dport;

			int l5_proto; /* RTP, RTCP, ALL */

			union {
				struct {
					avb_u8 sr_class;
					avb_u8 stream_id[8];
					avb_u8 pt;
					avb_u32 ssrc;
				} rtp;

				struct {
					avb_u8 pt;
					avb_u32 ssrc;
				} rtcp;
			} u;
		} ipv6;

		struct {
			avb_u16 protocol;
			avb_u8 dst_mac[6];
		} l2;
	} u;
};

struct net_mc_address {
	avb_u16 port;
	avb_u8 hw_addr[6];
};

/**
 * \ingroup socket
 * Protocol types
 */
enum NET_PTYPE {
	PTYPE_NONE = 0,
	PTYPE_MRP,
	PTYPE_PTP,
	PTYPE_AVTP,
	PTYPE_IPV4,
	PTYPE_IPV6,
	PTYPE_META,
	PTYPE_OTHER, /**< Non handled by genAVB stack */
	PTYPE_L2,    /**< Layer 2 */
	PTYPE_MAX,
};

#define PORT_ANY	0xffff

#endif /* _GENAVB_PUBLIC_NET_TYPES_H_ */
