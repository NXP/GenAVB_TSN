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
  @file		srp.h
  @brief		SRP common definitions
  @details	Public interface to the SRP module (MSRP, MVRP, MMRP)

  Copyright 2015 Freescale Semiconductor, Inc.
  All Rights Reserved.
*/

#ifndef _GENAVB_PUBLIC_SRP_H_
#define _GENAVB_PUBLIC_SRP_H_

#define MRP_DEFAULT_VID			htons(0x0002)

/**
* MSRP Failure Information values (802.1Qat 35.2.2.8.7)
*/
typedef enum
{
	INSUFFICIENT_BANDWIDTH = 1,
	INSUFFICIENT_BRIDGE_RESOURCES,
	INSUFFICIENT_BANDWIDTH_FOR_TRAFFIC_CLASS,
	STREAM_ID_ALREADY_IN_USE,
	STREAM_DESTINATION_ADDRESS_ALREADY_IN_USE,
	STREAM_PREEMPTED_BY_HIGHER_RANK,
	REPORTED_LATENCY_HAS_CHANGED,
	EGRESS_PORT_IS_NOT_AVB_CAPABLE,
	USE_DIFFERENT_DESTINATION_ADDRESS,
	OUT_OF_MSRP_RESOURCES,
	OUT_OF_MMRP_RESOURCES,
	CANNOT_STORE_DESTINATION_ADDRESS,
	REQUESTED_PRIORITY_IS_NOT_AN_SR_CLASS_PRIORITY,
	MAX_FRAME_SIZE_TOO_LARGE_FOR_MEDIA,
	FAN_IN_PORT_LIMIT_REACHED,
	CHANGE_IN_FIRST_VALUE_FOR_REGISTED_STREAM_ID,
	VLAN_BLOCKED_ON_EGRESS_PORT,
	VLAN_TAGGING_DISABLED_ON_EGRESS_PORT,
	SR_CLASS_PRIORITY_MISMATCH
} msrp_reservation_failure_code_t;

typedef enum {
	EMERGENCY = 0,
	NORMAL = 1
} msrp_rank_t;

struct msrp_failure_information {
	avb_u8 bridge_id[8];
	avb_u8 failure_code;
};

#endif /* _GENAVB_PUBLIC_SRP_H_ */
