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
 \file media.h
 \brief GenAVB public API
 \details media header definitions.
 \copyright Copyright 2015 Freescale Semiconductor, Inc.
*/
#ifndef _GENAVB_PUBLIC_MEDIA_H_
#define _GENAVB_PUBLIC_MEDIA_H_

#include "types.h"
#include "net_types.h"

#define MEDIA_TS_PER_PACKET	7

struct media_desc {
	NET_DESC_COMMON;

	avb_u8 n_ts;			/**< number of valid timestamps in the array below*/
	avb_u32 bytes_lost;		/**< Number of bytes lost between previous packet and current */
	struct {
		avb_u32 val;
		avb_u16 flags;
		avb_u16 offset;
	} avtp_ts[MEDIA_TS_PER_PACKET];	/**< array of AVTP timestamps (in gPTP time), their offsets in bytes, starting from the start of AVTP payload data and
					* AVTP_TIMESTAMP_* flags (from include/avtp.h). */
};

#define AVTP_FLAGS_TO_MEDIA_DESC(x)		((x))
#define MEDIA_DESC_FLAGS_TO_AVTP(x)		((x))

struct media_rx_desc {
	struct net_tx_desc net;

	avb_u8 ts_n;
	struct {
		avb_u32 val;	/**< AVTP timestamps (in gPTP time) */
		avb_u16 offset;	/**< offsets in bytes for the timestamps, starting from the start of AVTP payload data. */
	} avtp_ts[MEDIA_TS_PER_PACKET];
};

#endif /* _GENAVB_PUBLIC_MEDIA_H_ */
