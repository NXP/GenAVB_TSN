/*
* Copyright 2022-2023, 2025 NXP
*
* SPDX-License-Identifier: BSD-3-Clause
*/

/**
 @file
 @brief RTOS specific Network service implementation
 @details
*/

#ifndef __RTOS_NET_MDIO_H_
#define __RTOS_NET_MDIO_H_

struct net_mdio {
	int drv_type;
	unsigned int drv_index;
	void *handle;
	int (*read)(void *handle, uint8_t phyAddr, uint8_t regAddr, uint16_t *data);
	int (*write)(void *handle, uint8_t phyAddr, uint8_t regAddr, uint16_t data);
	void (*exit)(struct net_mdio *mdio);
};

void *mdio_read(unsigned int id);
void *mdio_write(unsigned int id);
int mdio_init(void);
void mdio_exit(void);

#endif /* __RTOS_NET_MDIO_H_ */
