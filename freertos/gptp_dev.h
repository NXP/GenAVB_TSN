/*
* Copyright 2018, 2020 NXP
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
 @brief GPTP event generation
 @details
*/
#ifndef _GPTP_DEV_H_
#define _GPTP_DEV_H_

#include "common/log.h"

#include "net_port.h"

#define FEC_TMODE_PULSE_HIGH		0xBC
#define FEC_TMODE_PULSE_LOW		0xB8
#define FEC_TMODE_TOGGLE		0x94
#define FEC_TMODE_CAPTURE_RISING	0x84

#define GPTP_MAX_DEVICES		1

#define GPTP_ENET_DEV_INDEX		0

struct gptp_drv_ops {
	int (*start)(struct net_port *port, uint32_t ts_0, uint32_t ts_1);
	void (*stop)(struct net_port *port);
	int (*reload)(struct net_port *port, uint32_t ts);
	int (*init)(struct net_port *port);
	void (*exit)(struct net_port *port);
};

struct gptp_dev {
	struct gptp_drv_ops drv_ops;
	bool enabled;
	unsigned int net_port;
	struct net_port *port;
};

int gptp_event_start(struct gptp_dev *dev, uint32_t ts_0, uint32_t ts_1);
void gptp_stop(struct gptp_dev *dev);
int gptp_tc_reload(struct gptp_dev *dev, uint32_t ts);
struct gptp_dev *gptp_event_init(uint32_t gptp_dev_index);
void gptp_event_exit(struct gptp_dev *dev);

#endif /* _GPTP_DEV_H_ */

