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
 @file		aecp.h
 @brief  	AECP protocol common definitions
 @details	PDU and protocol definitions for all AECP applications
*/

#ifndef _PROTO_AECP_H_
#define _PROTO_AECP_H_

#include "common/types.h"
#include "common/avtp.h"
#include "genavb/aecp.h"

#define OFFSET_TO_AECP (sizeof(struct eth_hdr) + sizeof(struct avtp_ctrl_hdr))
#define OFFSET_TO_AECP_SPECIFIC (sizeof(struct eth_hdr) + sizeof(struct avtp_ctrl_hdr) + sizeof(struct aecp_aem_pdu))

/* 250 ms timeout for all AECP commands (1722.1-2013 section 9.2.1.2.5) */
#define AECP_COMMANDS_TIMEOUT	250
#define AECP_IN_PROGRESS_TIMEOUT	120


#endif /* _PROTO_AECP_H_ */

