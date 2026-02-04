/*
 * Copyright 2017-2018, 2021-2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief RTOS static configuration
 @details Contains all compile time configuration options for RTOS
*/

#ifndef _RTOS_CFG_H_
#define _RTOS_CFG_H_

#ifndef _COMPONENT_ID_
#define _COMPONENT_ID_ os_COMPONENT_ID
#define _COMPONENT_STR_ "os"
#endif

#include "genavb/config.h"

#include "genavb_sdk.h"

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

#if defined(BOARD_NUM_NETC_EMDIO)
#define CFG_NUM_NETC_EMDIO		BOARD_NUM_NETC_EMDIO
#else
#define CFG_NUM_NETC_EMDIO		0
#endif

#if defined(BOARD_NUM_NETC_PORT_EMDIO)
#define CFG_NUM_NETC_PORT_EMDIO		BOARD_NUM_NETC_PORT_EMDIO
#else
#define CFG_NUM_NETC_PORT_EMDIO		0
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

#if !defined(FSL_FEATURE_NETC_HAS_NO_SWITCH) || FSL_FEATURE_NETC_HAS_NO_SWITCH
#define CFG_NETC_HAS_PSEUDO	0
#else
#define CFG_NETC_HAS_PSEUDO	1
#endif
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

#if defined(BOARD_NUM_STM)
#define CFG_NUM_STM		BOARD_NUM_STM
#else
#define CFG_NUM_STM		0
#endif

#if defined(BOARD_NUM_FTM)
#define CFG_NUM_FTM		BOARD_NUM_FTM
#else
#define CFG_NUM_FTM		0
#endif

#if defined(BOARD_FTM_NUM_IRQS)
#define CFG_FTM_NUM_IRQS		BOARD_FTM_NUM_IRQS
#else
#define CFG_FTM_NUM_IRQS		4
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

#if defined(BOARD_DSA_CPU_PORT_NETC_SWITCH)
#define CFG_DSA_CPU_PORT_NETC_SWITCH
#endif

#if defined(BOARD_DSA_CPU_PORT_ENETC)
#define CFG_DSA_CPU_PORT_ENETC
#endif

#if defined(BOARD_NET_RX_PACKETS)
#define CFG_NET_RX_PACKETS BOARD_NET_RX_PACKETS
#else
#define CFG_NET_RX_PACKETS		20	/* 100Mbit/s * 125us ~= 18.6 small packets,
					 1Gbit/s * 125us ~= 10 big packets */
#endif

#if defined(BOARD_NET_RX_PERIOD_MULT)
#define CFG_NET_RX_PERIOD_MULT BOARD_NET_RX_PERIOD_MULT
#endif

#endif /* _RTOS_CFG_H_ */
