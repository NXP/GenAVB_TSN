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

#include "os/sys_types.h"

#include "config.h"

#include "net_mdio.h"
#include "net_port.h"
#include "net_port_netc_mdio.h"

#if CFG_NUM_MDIO

static struct net_mdio mdio[CFG_NUM_MDIO] = {
	[0] = {
		.drv_type = BOARD_MDIO0_DRV_TYPE,
		.drv_index = BOARD_MDIO0_DRV_INDEX,
	},
};

static int mdio0_write(uint8_t phyAddr, uint8_t regAddr, uint16_t data)
{
	return mdio[0].write(mdio[0].handle, phyAddr, regAddr, data);
}

static int mdio0_read(uint8_t phyAddr, uint8_t regAddr, uint16_t *data)
{
	return mdio[0].read(mdio[0].handle, phyAddr, regAddr, data);
}

void *mdio_read(unsigned int id)
{
	return mdio0_read;
}

void *mdio_write(unsigned int id)
{
	return mdio0_write;
}

__exit static void __mdio_exit(struct net_mdio *mdio)
{
	mdio->exit(mdio->handle);
}

__init static int __mdio_init(struct net_mdio *mdio)
{
	int rc;

	switch (mdio->drv_type) {
	case NETC_SW_t:
		rc = netc_mdio_init(mdio);
		break;
	case ENET_t:
	case ENET_1G_t:
	case ENET_QOS_t:
	default:
		rc = -1;
		break;
	}

	return rc;
}

__init int mdio_init(void)
{
	int i, j;

	for (i = 0; i < CFG_NUM_MDIO; i++) {
		if (__mdio_init(&mdio[i]) < 0)
			goto err;
	}

	return 0;

err:
	for (j = 0; j < i; j++)
		__mdio_exit(&mdio[j]);

	return -1;
}

__exit void mdio_exit(void)
{
	int i;

	for (i = 0; i < CFG_NUM_MDIO; i++)
		__mdio_exit(&mdio[i]);
}

#else
__init int mdio_init(void) { return 0; }
__exit void mdio_exit(void) { }
#endif /* CFG_NUM_MDIO */
