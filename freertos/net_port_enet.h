/*
* Copyright 2019-2020 NXP
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
 @brief FreeRTOS specific Network service implementation
 @details
*/

#ifndef _FREERTOS_NET_PORT_ENET_H_
#define _FREERTOS_NET_PORT_ENET_H_

#include "board.h"
#include "net_port.h"

#ifndef BOARD_USE_ENET_QOS
#define BOARD_USE_ENET_QOS 0
#endif

#define NUM_ENET_MAC (MIN(FSL_FEATURE_SOC_ENET_COUNT, CFG_PORTS) - BOARD_USE_ENET_QOS)

#if NUM_ENET_MAC
int enet_init(struct net_port *port);
#else
static inline int enet_init(struct net_port *port) { return -1; };
#endif

#endif /* _FREERTOS_NET_PORT_ENET_H_ */
