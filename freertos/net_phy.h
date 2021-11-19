/*
* Copyright 2017-2020 NXP
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

#ifndef _FREERTOS_NET_PHY_H_
#define _FREERTOS_NET_PHY_H_

#include "net_port.h"

#include "fsl_phy.h"
#include "fsl_mdio.h"

struct net_phy {
	phy_handle_t handle;
	phy_config_t config;

	mdio_handle_t mdio_handle;

	unsigned int rx_tstamp_latency_100M;
	unsigned int tx_tstamp_latency_100M;
	unsigned int rx_tstamp_latency_1G;
	unsigned int tx_tstamp_latency_1G;
};

void phy_port_status(struct net_port *port, struct net_port_status *status);
void phy_set_ts_latency(struct net_port *port);
int phy_init(struct net_port *port);
void phy_exit(struct net_port *port);

#endif /* _FREERTOS_NET_PHY_H_ */
