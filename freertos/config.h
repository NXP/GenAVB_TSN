/*
* Copyright 2017, 2020, 2023 NXP
*
* SPDX-License-Identifier: BSD-3-Clause
*/

/**
 @file
 @brief FreeRTOS static configuration
 @details Contains all compile time configuration options for FreeRTOS
*/

#ifndef _FREERTOS_CFG_H_
#define _FREERTOS_CFG_H_

#include "genavb/config.h"

#define freertos_CFG_LOG	CFG_LOG

#define freertos_COMPONENT_ID	os_COMPONENT_ID

#include "board.h"

#if defined(BOARD_NUM_PORTS)
#define CFG_PORTS		BOARD_NUM_PORTS
#else
#define CFG_PORTS		0
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

#if defined(BOARD_NUM_NETC_HW_CLOCK)
#define CFG_NUM_NETC_HW_CLOCK	BOARD_NUM_NETC_HW_CLOCK
#else
#define CFG_NUM_NETC_HW_CLOCK	0
#endif

#if defined(BOARD_NUM_GPT)
#define CFG_NUM_GPT		BOARD_NUM_GPT
#else
#define CFG_NUM_GPT		0
#endif

#if defined(BOARD_NUM_TPM)
#define CFG_NUM_TPM		BOARD_NUM_TPM
#else
#define CFG_NUM_TPM		0
#endif

#if defined(BOARD_NUM_MSGINTR)
#define CFG_NUM_MSGINTR		BOARD_NUM_MSGINTR
#else
#define CFG_NUM_MSGINTR		-1
#endif

#define CFG_ENDPOINT_NUM	(CFG_NUM_ENET_MAC + CFG_NUM_ENET_QOS_MAC + CFG_NUM_ENETC_EP_MAC)
#define CFG_BRIDGE_NUM		(CFG_NUM_NETC_SW)
#define CFG_BRIDGE_PORT_NUM	(CFG_NUM_NETC_SW_PORTS)

#if CFG_ENDPOINT_NUM > CFG_MAX_ENDPOINTS
#error Too many endpoint interfaces defined
#endif

#if CFG_BRIDGE_NUM > CFG_MAX_BRIDGES
#error Too many bridge interfaces defined
#endif

#if CFG_BRIDGE_PORT_NUM > CFG_MAX_NUM_PORT
#error Too many bridge ports defined
#endif

#if CFG_BRIDGE_NUM
#define CFG_LOGICAL_NUM_PORT	(CFG_MAX_ENDPOINTS + CFG_BRIDGE_PORT_NUM)
#else
#define CFG_LOGICAL_NUM_PORT	(CFG_ENDPOINT_NUM)
#endif

#endif /* _FREERTOS_CFG_H_ */
