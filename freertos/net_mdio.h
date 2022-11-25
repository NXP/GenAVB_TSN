/*
* Copyright 2022 NXP
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

#ifndef __FREERTOS_NET_MDIO_H_
#define __FREERTOS_NET_MDIO_H_

struct net_mdio {
	int drv_type;
	unsigned int drv_index;
	void *handle;
	int (*read)(void *, uint8_t, uint8_t, uint16_t *);
	int (*write)(void *, uint8_t, uint8_t, uint16_t);
	void (*exit)(struct net_mdio *);
};

void *mdio_read(unsigned int id);
void *mdio_write(unsigned int id);
int mdio_init(void);
void mdio_exit(void);

#endif /* __FREERTOS_NET_MDIO_H_ */
