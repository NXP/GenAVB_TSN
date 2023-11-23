/*
* Copyright 2020, 2022-2023 NXP
*
* SPDX-License-Identifier: BSD-3-Clause
*/

/**
 @file
 @brief FreeRTOS specific Network service implementation
 @details
*/

#ifndef _FREERTOS_NET_PORT_ENET_QOS_H_
#define _FREERTOS_NET_PORT_ENET_QOS_H_

#include "net_port.h"

int enet_qos_init(struct net_port *port);

#endif /* _FREERTOS_NET_PORT_ENET_QOS_H_ */
