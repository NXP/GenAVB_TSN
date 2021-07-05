/*
* Copyright 2014-2015 Freescale Semiconductor, Inc.
* Copyright 2016, 2020 NXP
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
 @file		adp.h
 @brief 	ADP protocol common definitions
 @details	PDU and protocol definitions for all ADP applications
*/

#ifndef _PROTO_ADP_H_
#define _PROTO_ADP_H_

#include "common/types.h"
#include "common/avtp.h"
#include "genavb/adp.h"

struct __attribute__ ((packed)) adp_pdu {
	u64 entity_id;
	u64 entity_model_id;
	u32 entity_capabilities;
	u16 talker_stream_sources;
	u16 talker_capabilities;
	u16 listener_stream_sinks;
	u16 listener_capabilities;
	u32 controller_capabilities;
	u32 available_index;
	u64 gptp_grandmaster_id;
	u8  gptp_domain_number;
	u8  rsvd0;
	u16 rsvd1;
	u16 identity_control_index;
	u16 interface_index;
	u64 association_id;
	u32 rsvd2;
};


#define ADP_PDU_LEN 56
#define OFFSET_TO_ADP (sizeof(struct eth_hdr) + sizeof(struct avtp_ctrl_hdr))
#define ADP_NET_DATA_SIZE (OFFSET_TO_ADP + sizeof(struct adp_pdu))

#endif /* _PROTO_ADP_H_ */
