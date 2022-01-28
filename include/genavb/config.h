/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2021 NXP
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
 \file config.h
 \brief GenAVB public API
 \details config header definitions.

 \copyright Copyright 2015 Freescale Semiconductor, Inc.
*/
#ifndef _GENAVB_PUBLIC_CONFIG_H_
#define _GENAVB_PUBLIC_CONFIG_H_

#include "autoconf.h"
#include "os/config_os.h"

#define CFG_MAX_NUM_PORT	5	/* Absolute maximum number of ports supported by the stack components
					   (though, some may support less) */

#define CFG_MAX_ENDPOINTS	2	/* Absolute maximum number of endpoints supported by the stack */
#define CFG_MAX_BRIDGES		1	/* Absolute maximum number of bridges supported by the stack */

#define CFG_MAX_GPTP_DOMAINS	2	/* Absolute maximum number of gptp domains supported by the stack */

#define CFG_MAX_LOGICAL_PORTS	(CFG_MAX_ENDPOINTS + CFG_MAX_BRIDGES * CFG_MAX_NUM_PORT)

/* Endpoint configuration for both legacy endpoint and hybrid setup */
#define CFG_EP_DEFAULT_NUM_PORTS	(1)
#define CFG_EP_LOGICAL_PORT_LIST	{0}

/* Bridge stack for regular bridge and hybrid setup */
#define CFG_BR_DEFAULT_NUM_PORTS        (5)
#define CFG_BR_LOGICAL_PORT_LIST	{2, 3, 4, 5, 6}

#define CFG_DEFAULT_PORT_ID     0	/* internal default port id */

#define CFG_DEFAULT_GPTP_DOMAINS	1
#define CFG_DEFAULT_GPTP_DOMAIN     	0

#define __LITTLE_ENDIAN__ 1

#define CFG_TRAFFIC_CLASS_QUEUE_MAX	16
#define CFG_SR_CLASS_STREAM_MAX		16	/* must be lower or equal to 32, to avoid overflowing several bitfields */


#define CFG_SR_CLASS_HIGH_STREAM_MAX	8	/* should be lower or equal to CFG_SR_CLASS_STREAM_MAX */
#define CFG_SR_CLASS_LOW_STREAM_MAX	8	/* should be lower or equal to CFG_SR_CLASS_STREAM_MAX */

#define CFG_RX_BEST_EFFORT		1 /* must match fec driver configuration */
#define CFG_TX_BEST_EFFORT		(CFG_TRAFFIC_CLASS_MAX - CFG_SR_CLASS_MAX)

#define CFG_RX_STREAM_MAX		8 /* Max class HIGH+LOW listener streams for all ports, should be lower or equal to CFG_SR_CLASS_HIGH_STREAM_MAX and CFG_SR_CLASS_LOW_STREAM_MAX */
#define CFG_TX_STREAM_MAX		8 /* Max class HIGH+LOW talker streams For all ports, should be lower or equal to CFG_SR_CLASS_HIGH_STREAM_MAX and CFG_SR_CLASS_LOW_STREAM_MAX */

#define CFG_STREAM_MAX			8 /* Max Class HIGH+LOW listener+talker streams for all ports, should be lower or equal to CFG_RX_STREAMS_MAX and CFG_TX_STREAMS_MAX */

#define CFG_TX_PROTO			1 /* protocols that can "connect", except avtp class A/B/C/D. Check PTYPE_XXXX. */
#define CFG_RX_PROTO			4 /* protocols that can "bind", except avtp class A/B/C/D. Check PTYPE_XXXX. */

#define CFG_MEDIA_QUEUE_EXTRA_ENTRIES	512
#define CFG_TX_CLEANUP_EXTRA_ENTRIES	256
#define CFG_RX_EXTRA_ENTRIES		256
#define CFG_NET_TX_EXTRA_ENTRIES	32

#define CFG_AVTP_1722A	1
#define CFG_1733	1

#define CFG_AVDECC_NUM_ENTITIES 		2


#define CFG_CVF_MJPEG_SALSA_WA 1

#define CFG_AVB_IPV4_1733	1

#endif /* _GENAVB_PUBLIC_CONFIG_H_ */
