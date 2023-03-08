/*
* Copyright 2022-2023 NXP
*
* SPDX-License-Identifier: BSD-3-Clause
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
