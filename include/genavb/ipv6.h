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
 \file ipv6.h
 \brief GenAVB public API
 \details IPv6 header definitions.

 \copyright Copyright 2015 Freescale Semiconductor, Inc.
*/
#ifndef _GENAVB_PUBLIC_IPV6_H_
#define _GENAVB_PUBLIC_IPV6_H_

#include "types.h"

struct ipv6_hdr {
	avb_u32		version_tc_flowlbl;
	avb_u16		payload_len;
	avb_u8		nexthdr;
	avb_u8		hop_limit;
	avb_u32		saddr[4];
	avb_u32		daddr[4];
};

#define IPV6_GET_VERSION(hdr) \
	((ntohl((hdr)->version_tc_flowlbl) >> 28) & 0xf)

#define IPV6_SET_VERSION(hdr, v) \
	(hdr)->version_tc_flowlbl = (htonl(((v) & 0xf) << 28) | ((hdr)->version_tc_flowlbl & htonl(0x0fffffff)))

#define IPV6_GET_TRAFFIC_CLASS(hdr) \
	((ntohl((hdr)->version_tc_flowlbl) >> 20) & 0xff)

#define IPV6_SET_TRAFFIC_CLASS(hdr, tc) \
	(hdr)->version_tc_flowlbl = (htonl(((tc) & 0xff) << 20) | ((hdr)->version_tc_flowlbl & htonl(0xf00fffff)))

#define IPV6_GET_FLOW_LABEL(hdr) \
	(ntohl((hdr)->version_tc_flowlbl) & 0xfffff)

#define IPV6_SET_FLOW_LABEL(hdr, fl) \
	(hdr)->version_tc_flowlbl = (htonl(((fl) & 0xfffff)) | ((hdr)->version_tc_flowlbl & htonl(0x000fffff)))


#endif /* _GENAVB_PUBLIC_IPV6_H_ */
