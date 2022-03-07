/*
* Copyright 2018-2020 NXP
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
 @file
 @brief AVB logical port configuration
 @details
*/

#ifndef _PORT_CONFIG_H_
#define _PORT_CONFIG_H_

#include "board.h"

#define CFG_LOGICAL_NUM_PORT		1
#define CFG_PORTS			BOARD_NUM_PORTS

enum logical_port_type {
	LOGICAL_PORT_TYPE_PHYSICAL,
	LOGICAL_PORT_TYPE_BRIDGE
};

struct logical_port_config {
	unsigned int type;
	union {
		struct {
			unsigned int id;
			unsigned int port;
		} bridge;

		struct {
			unsigned int id;
		} physical;
	};
};

#define LOGICAL_PORT_CONFIG                                 \
	{                                                   \
		[0] = {                                     \
			.type = LOGICAL_PORT_TYPE_PHYSICAL, \
			.physical.id = 0,                   \
		},                                          \
	}

#endif /* _PORT_CONFIG_H_ */
