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
 \file ipv4.h
 \brief GenAVB public API
 \details IPv4 header definitions.

 \copyright Copyright 2015 Freescale Semiconductor, Inc.
*/
#ifndef _GENAVB_PUBLIC_IPV4_H_
#define _GENAVB_PUBLIC_IPV4_H_

#include "types.h"

struct ipv4_hdr {
#if defined(__BIG_ENDIAN__)
	avb_u8	version:4,
		ihl:4;
#else
	avb_u8	ihl:4,
		version:4;
#endif
	avb_u8	tos;
	avb_u16	tot_len;
	avb_u16	id;
	avb_u16	frag_off;
	avb_u8	ttl;
	avb_u8	protocol;
	avb_u16	checksum;
	avb_u32	saddr;
	avb_u32	daddr;
};

#define IPV4_GET_FRAG_IND(hdr) \
	((ntohs((hdr)->frag_off) >> 13) & 0x7)

#define IPV4_GET_FRAG_OFFSET(hdr) \
	(ntohs((hdr)->frag_off) & 0x1FFF)

#endif /* _GENAVB_PUBLIC_IPV4_H_ */
