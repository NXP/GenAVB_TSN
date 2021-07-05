/*
* Copyright 2014-2015 Freescale Semiconductor, Inc.
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
 @file     	acmp.h
 @brief 	ACMP protocol common definitions
 @details	PDU and protocol definitions for all ACMP applications
*/

#ifndef _PROTO_ACMP_H_
#define _PROTO_ACMP_H_

#include "genavb/acmp.h"
#include "common/types.h"
#include "common/avtp.h"

struct __attribute__ ((packed)) acmp_pdu {
	u64 stream_id;
	u64 controller_entity_id;
	u64 talker_entity_id;
	u64 listener_entity_id;
	u16 talker_unique_id;
	u16 listener_unique_id;
	u8 stream_dest_mac[6];
	u16 connection_count;
	u16 sequence_id;
	u16 flags;
	u16 stream_vlan_id;
	u16 rsvd;
};

#define OFFSET_TO_ACMP (sizeof(struct eth_hdr) + sizeof(struct avtp_ctrl_hdr))
#define ACMP_NET_DATA_SIZE (OFFSET_TO_ACMP + sizeof(struct acmp_pdu))
#define ACMP_IS_LISTENER_COMMAND(msg_type) ((msg_type == ACMP_CONNECT_RX_COMMAND) || (msg_type == ACMP_GET_RX_STATE_COMMAND) || (msg_type == ACMP_DISCONNECT_RX_COMMAND))
#define ACMP_IS_TALKER_COMMAND(msg_type) ((msg_type == ACMP_CONNECT_TX_COMMAND) || (msg_type == ACMP_GET_TX_STATE_COMMAND) || (msg_type == ACMP_DISCONNECT_TX_COMMAND) || (msg_type == ACMP_GET_TX_CONNECTION_COMMAND))
#define ACMP_IS_COMMAND(msg_type) (!(msg_type & 0x1))



/*
 * ACMP_CONNECT_TX_COMMAND         2000 ms
 * ACMP_DISCONNECT_TX_COMMAND       200 ms
 * ACMP_GET_TX_STATE_COMMAND        200 ms
 * ACMP_CONNECT_RX_COMMAND         4500 ms
 * ACMP_DISCONNECT_RX_COMMAND       500 ms
 * ACMP_GET_RX_STATE_COMMAND        200 ms
 * ACMP_GET_TX_CONNECTION_COMMAND   200 ms
 */
#define ACMP_COMMANDS_TIMEOUT	{2000, 200, 200, 4500, 500, 200, 200};

#define ACMP_PDU_LEN 44

#endif /* _PROTO_ACMP_H_ */
