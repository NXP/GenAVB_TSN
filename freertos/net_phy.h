/*
* Copyright 2017-2020, 2022 NXP
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

typedef void (*phy_callback_t)(void *data, unsigned int event, unsigned int speed, unsigned int duplex);

void phy_port_status(unsigned int phy_id, struct net_port_status *status);
void phy_get_ts_latency(unsigned int phy_id, uint32_t *rx_latency, uint32_t *tx_latency);
int phy_connect(unsigned int phy_id, phy_callback_t callback, void *data);
void phy_disconnect(unsigned int phy_id);
int phy_init(void);
void phy_exit(void);

#endif /* _FREERTOS_NET_PHY_H_ */
