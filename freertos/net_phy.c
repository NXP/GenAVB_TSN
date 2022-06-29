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
#include "common/log.h"

#include "net_phy.h"
#include "net_rx.h"
#include "net_tx.h"

#include "dev_itf.h"
#include "board.h"

#define PHY_TASK_STACK_DEPTH		(configMINIMAL_STACK_SIZE + 160)
#define PHY_TASK_PRIORITY		1
#define PHY_TASK_NAME			"phy"
#define PHY_TASK_PERIOD_MS		100

extern const mdio_operations_t enet_qos_ops;
extern const mdio_operations_t enet_ops;
extern const phy_operations_t phyksz8081_ops;
extern const phy_operations_t phyrtl8211f_ops;

struct net_phy phy_devices[BOARD_NUM_PHY] = {
	[0] = {
		.mdio_handle = {
			.ops = BOARD_PHY0_MDIO_OPS,
			.resource = {
				.base = (void *)BOARD_PHY0_MDIO_BASE,
			},
		},
		.handle = {
			.ops = BOARD_PHY0_OPS,
		},
		.config = {
			.phyAddr = BOARD_PHY0_ADDRESS,
			.autoNeg = true,
		},
		.rx_tstamp_latency_100M = BOARD_PHY0_RX_LATENCY_100M,
		.tx_tstamp_latency_100M = BOARD_PHY0_TX_LATENCY_100M,
#ifdef BOARD_PHY0_RX_LATENCY_1G
		.rx_tstamp_latency_1G = BOARD_PHY0_RX_LATENCY_1G,
#endif
#ifdef BOARD_PHY0_TX_LATENCY_1G
		.tx_tstamp_latency_1G = BOARD_PHY0_TX_LATENCY_1G,
#endif
	},
#if BOARD_NUM_PHY > 1
	[1] = {
		.mdio_handle = {
			.ops = BOARD_PHY1_MDIO_OPS,
			.resource = {
				.base = (void *)BOARD_PHY1_MDIO_BASE,
			},
		},
		.handle = {
			.ops = BOARD_PHY1_OPS,
		},
		.config = {
			.phyAddr = BOARD_PHY1_ADDRESS,
			.autoNeg = true,
		},
		.rx_tstamp_latency_100M = BOARD_PHY1_RX_LATENCY_100M,
		.tx_tstamp_latency_100M = BOARD_PHY1_TX_LATENCY_100M,
#ifdef BOARD_PHY1_RX_LATENCY_1G
		.rx_tstamp_latency_1G = BOARD_PHY1_RX_LATENCY_1G,
#endif
#ifdef BOARD_PHY1_TX_LATENCY_1G
		.tx_tstamp_latency_1G = BOARD_PHY1_TX_LATENCY_1G,
#endif
	},
#endif
};

#define port_to_phy(port) (&phy_devices[port->phy_index])

void phy_port_status(struct net_port *port, struct net_port_status *status)
{
	if (port->phy_duplex == kPHY_FullDuplex)
		status->full_duplex = true;
	else
		status->full_duplex = false;

	switch (port->phy_speed) {
	default:
	case kPHY_Speed10M:
		status->rate = 10 * 1000000;
		break;

	case kPHY_Speed100M:
		status->rate = 100 * 1000000;
		break;
#if FSL_FEATURE_ENET_HAS_AVB
	case kPHY_Speed1000M:
		status->rate = 1000 * 1000000;
		break;
#endif
	}
}

void phy_set_ts_latency(struct net_port *port)
{
	struct net_phy *phy = port_to_phy(port);

	switch (port->phy_speed) {
	case kPHY_Speed100M:
		port->rx_tstamp_latency = phy->rx_tstamp_latency_100M;
		port->tx_tstamp_latency = phy->tx_tstamp_latency_100M;
		break;
#if FSL_FEATURE_ENET_HAS_AVB
	case kPHY_Speed1000M:
		port->rx_tstamp_latency = phy->rx_tstamp_latency_1G;
		port->tx_tstamp_latency = phy->tx_tstamp_latency_1G;
		break;
#endif
	default:
		port->rx_tstamp_latency = 0;
		port->tx_tstamp_latency = 0;
		break;
	}
}

static int phy_poll(struct net_port *port, phy_speed_t *speed, phy_duplex_t *duplex)
{
	bool link;
	struct net_phy *phy = port_to_phy(port);

	/* poll phy status and get up/down/speed duplex. Required to configure MAC with correct speed/duplex */
	if (PHY_GetLinkStatus(&phy->handle, &link) != kStatus_Success)
		goto err_status;

	if (link) {
		/* Get the actual PHY link speed. */
		if (PHY_GetLinkSpeedDuplex(&phy->handle, speed, duplex) != kStatus_Success)
			goto err_speed;
	}

	return link;

err_speed:
err_status:
	return -1;
}

static void phy_event(unsigned int type, struct net_port *port)
{
	struct event e;

	e.type = type;
	e.data = port;

	if (type == EVENT_PHY_DOWN) {
		/* Stop software tx first */
		xQueueSendToBack(net_tx_ctx.queue_handle, &e, pdMS_TO_TICKS(10));
		xEventGroupWaitBits(port->event_group_handle, PORT_TX_SUCCESS, pdTRUE, pdFALSE, pdMS_TO_TICKS(10));

		/* Stop driver rx/tx and flush software tx */
		xQueueSendToBack(net_rx_ctx.queue_handle, &e, pdMS_TO_TICKS(10));
		xEventGroupWaitBits(port->event_group_handle, PORT_SUCCESS, pdTRUE, pdFALSE, pdMS_TO_TICKS(10));
	} else {
		xQueueSendToBack(net_rx_ctx.queue_handle, &e, pdMS_TO_TICKS(10));
		xEventGroupWaitBits(port->event_group_handle, PORT_SUCCESS, pdTRUE, pdFALSE, pdMS_TO_TICKS(10));

		xQueueSendToBack(net_tx_ctx.queue_handle, &e, pdMS_TO_TICKS(10));
		xEventGroupWaitBits(port->event_group_handle, PORT_TX_SUCCESS, pdTRUE, pdFALSE, pdMS_TO_TICKS(10));
	}
}

static void phy_task(void *pvParameters)
{
	struct net_port *port = (struct net_port *)pvParameters;
	struct net_phy *phy = port_to_phy(port);
	status_t status;

	os_log(LOG_INIT, "phy(%u) task started\n", port->phy_index);

	phy->mdio_handle.resource.csrClock_Hz = dev_get_enet_core_freq(phy->mdio_handle.resource.base);
	phy->handle.mdioHandle = &phy->mdio_handle;

	if (PHY_Init(&phy->handle, &phy->config) != kStatus_Success) {
		os_log(LOG_ERR, "phy(%u) PHY_Init() failed\n", port->phy_index);
		goto exit;
	}

	os_log(LOG_INIT, "phy(%u) initialized\n", port->phy_index);

	for (;;) {
		vTaskDelay(pdMS_TO_TICKS(PHY_TASK_PERIOD_MS));

		status = phy_poll(port, (phy_speed_t *)&port->phy_speed, (phy_duplex_t *)&port->phy_duplex);
		if (status < 0)
			continue;

		if (status != port->up) {
			if (status == true)
				phy_event(EVENT_PHY_UP, port);
			else
				phy_event(EVENT_PHY_DOWN, port);

		} else if ((status == true) && ((port->phy_speed != port->old_phy_speed) || (port->phy_duplex != port->old_phy_duplex))) {
			phy_event(EVENT_PHY_DOWN, port);
			phy_event(EVENT_PHY_UP, port);
		}

		port->old_phy_speed = port->phy_speed;
		port->old_phy_duplex = port->phy_duplex;
	}
exit:
	os_log(LOG_INIT, "phy(%u) task exited\n", port->phy_index);

	vTaskDelete(NULL);
}

__init int phy_init(struct net_port *port)
{
	if (xTaskCreate(phy_task, PHY_TASK_NAME, PHY_TASK_STACK_DEPTH, (void *)port, PHY_TASK_PRIORITY, &port->phy_task_handle) != pdPASS) {
		os_log(LOG_ERR, "xTaskCreate(%s) failed\n", PHY_TASK_NAME);
		goto err;
	}

	return 0;

err:
	return -1;
}

__exit void phy_exit(struct net_port *port)
{
	vTaskDelete(port->phy_task_handle);

	os_log(LOG_INIT, "phy(%u) exit\n", port->index);
}
