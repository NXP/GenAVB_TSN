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
 \file ptp.h
 \brief GenAVB public API
 \details ptp header definitions.

 \copyright Copyright 2015 Freescale Semiconductor, Inc.
*/
#ifndef _GENAVB_PUBLIC_PTP_H_
#define _GENAVB_PUBLIC_PTP_H_

#include "types.h"

/**
 * \ingroup protocol
 */
struct __attribute__ ((packed)) ptp_port_identity {
	avb_u8 clock_identity[8];
	avb_u16 port_number;
};

/**
 * PTP Message Header (802.1AS - Table 10.4)
 * \ingroup protocol
 */
struct __attribute__ ((packed)) ptp_hdr {
#ifdef __BIG_ENDIAN__
	avb_u8 transport_specific:4;
	avb_u8 msg_type:4;

	avb_u8 reserved0:4;
	avb_u8 version_ptp:4;
#else
	avb_u8 msg_type:4;
	avb_u8 transport_specific:4;

	avb_u8 version_ptp:4;
	avb_u8 reserved0:4;
#endif

	avb_u16 msg_length;
	avb_u8 domain_number;
	avb_u8 reserved1;
	avb_u16 flags;
	avb_s64 correction_field;
	avb_u32 reserved2;
	struct ptp_port_identity source_port_id;
	avb_u16 sequence_id;
	avb_u8 control;
	avb_s8 log_msg_interval;
};

#endif /* _GENAVB_PUBLIC_PTP_H_ */
