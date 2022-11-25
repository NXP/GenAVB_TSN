/*
* Copyright 2017, 2020 NXP
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
 @brief FreeRTOS static configuration
 @details Contains all compile time configuration options for FreeRTOS
*/

#ifndef _FREERTOS_CFG_H_
#define _FREERTOS_CFG_H_

#define freertos_CFG_LOG	CFG_LOG

#define freertos_COMPONENT_ID	os_COMPONENT_ID

#include "board.h"

#if defined(BOARD_NUM_PORTS)
#define CFG_PORTS		BOARD_NUM_PORTS
#define CFG_LOGICAL_NUM_PORT	BOARD_NUM_PORTS
#else
#define CFG_PORTS		0
#define CFG_LOGICAL_NUM_PORT	0
#endif

#if defined(BOARD_NUM_PHY)
#define CFG_NUM_PHY		BOARD_NUM_PHY
#else
#define CFG_NUM_PHY		0
#endif

#if defined(BOARD_NUM_MDIO)
#define CFG_NUM_MDIO		BOARD_NUM_MDIO
#else
#define CFG_NUM_MDIO		0
#endif

#if defined(BOARD_NUM_BRIDGES)
#define CFG_BRIDGE_NUM 	BOARD_NUM_BRIDGES
#else
#define CFG_BRIDGE_NUM		0
#endif

#if defined(BOARD_NUM_ENET_QOS_PORTS)
#define CFG_NUM_ENET_QOS_MAC	BOARD_NUM_ENET_QOS_PORTS
#else
#define CFG_NUM_ENET_QOS_MAC	0
#endif

#if defined(BOARD_NUM_ENET_PORTS)
#define CFG_NUM_ENET_MAC	BOARD_NUM_ENET_PORTS
#else
#define CFG_NUM_ENET_MAC	0
#endif

#if defined(BOARD_NUM_ENETC_PORTS)
#define CFG_NUM_ENETC_EP_MAC	BOARD_NUM_ENETC_PORTS
#else
#define CFG_NUM_ENETC_EP_MAC	0
#endif

#if defined(BOARD_NUM_NETC_SWITCHES)
#define CFG_NUM_NETC_SW		BOARD_NUM_NETC_SWITCHES
#define CFG_NUM_NETC_SW_PORTS	BOARD_NUM_NETC_PORTS
#else
#define CFG_NUM_NETC_SW		0
#define CFG_NUM_NETC_SW_PORTS	0
#endif

#if defined(BOARD_NUM_GPT)
#define CFG_NUM_GPT		BOARD_NUM_GPT
#else
#define CFG_NUM_GPT		2
#endif

#endif /* _FREERTOS_CFG_H_ */
