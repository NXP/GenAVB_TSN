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
#include "common/log.h"

#include "net_phy.h"
#include "net_rx.h"
#include "net_tx.h"

#if CFG_NUM_PHY

#include "fsl_phy.h"
#ifdef CFG_PHY_OLD_INTERFACE
#include "fsl_mdio.h"
#include "dev_itf.h"
#else
#include "net_mdio.h"
#endif

#define PHY_TASK_STACK_DEPTH		(configMINIMAL_STACK_SIZE + 160)
#define PHY_TASK_PRIORITY		1
#define PHY_TASK_NAME			"phy"
#define PHY_TASK_PERIOD_MS		100

struct net_phy {
	phy_handle_t handle;
	phy_config_t config;

	bool enabled;
#ifdef CFG_PHY_OLD_INTERFACE
	mdio_handle_t mdio_handle;
#else
	unsigned int mdio_id;

	struct {
		mdioWrite write;
		mdioRead read;
	} resource;
#endif
	int (*fixup)(phy_handle_t *handle);

	phy_callback_t callback;
	void *data;

	bool up;
	phy_speed_t speed;
	phy_duplex_t duplex;

	unsigned int rx_tstamp_latency_100M;
	unsigned int tx_tstamp_latency_100M;
	unsigned int rx_tstamp_latency_1G;
	unsigned int tx_tstamp_latency_1G;
};

struct net_phy_ctx {
	TaskHandle_t task_handle;
	struct net_phy devices[CFG_NUM_PHY];
};

#ifdef CFG_PHY_OLD_INTERFACE
extern const mdio_operations_t enet_qos_ops;
extern const mdio_operations_t enet_ops;
extern const phy_operations_t phyksz8081_ops;
extern const phy_operations_t phyrtl8211f_ops;
#else
extern const phy_operations_t BOARD_PHY0_OPS;
#if CFG_NUM_PHY > 1
extern const phy_operations_t BOARD_PHY1_OPS;
#endif
#if CFG_NUM_PHY > 2
extern const phy_operations_t BOARD_PHY2_OPS;
#endif
#if CFG_NUM_PHY > 3
extern const phy_operations_t BOARD_PHY3_OPS;
#endif
#if CFG_NUM_PHY > 4
extern const phy_operations_t BOARD_PHY4_OPS;
#endif
#endif

static struct net_phy_ctx phy_ctx = {
	.devices = {
		[0] = {
#ifdef CFG_PHY_OLD_INTERFACE
			.mdio_handle = {
				.ops = BOARD_PHY0_MDIO_OPS,
				.resource = {
					.base = (void *)BOARD_PHY0_MDIO_BASE,
				},
			},
			.handle = {
				.ops = BOARD_PHY0_OPS,
			},
#else
			.mdio_id = BOARD_PHY0_MDIO_ID,
#endif
			.config = {
				.phyAddr = BOARD_PHY0_ADDRESS,
				.autoNeg = true,
#ifndef CFG_PHY_OLD_INTERFACE
				.ops = &BOARD_PHY0_OPS,
#endif
			},
#ifdef BOARD_PHY0_FIXUP
			.fixup = BOARD_PHY0_FIXUP,
#endif
			.rx_tstamp_latency_100M = BOARD_PHY0_RX_LATENCY_100M,
			.tx_tstamp_latency_100M = BOARD_PHY0_TX_LATENCY_100M,
#ifdef BOARD_PHY0_RX_LATENCY_1G
			.rx_tstamp_latency_1G = BOARD_PHY0_RX_LATENCY_1G,
#endif
#ifdef BOARD_PHY0_TX_LATENCY_1G
			.tx_tstamp_latency_1G = BOARD_PHY0_TX_LATENCY_1G,
#endif
		},

#if CFG_NUM_PHY > 1
		[1] = {
#ifdef CFG_PHY_OLD_INTERFACE
			.mdio_handle = {
				.ops = BOARD_PHY1_MDIO_OPS,
				.resource = {
					.base = (void *)BOARD_PHY1_MDIO_BASE,
				},
			},
			.handle = {
				.ops = BOARD_PHY1_OPS,
			},
#else
			.mdio_id = BOARD_PHY1_MDIO_ID,
#endif
			.config = {
				.phyAddr = BOARD_PHY1_ADDRESS,
				.autoNeg = true,
#ifndef CFG_PHY_OLD_INTERFACE
				.ops = &BOARD_PHY1_OPS,
#endif
			},
#ifdef BOARD_PHY1_FIXUP
			.fixup = BOARD_PHY1_FIXUP,
#endif
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

#if defined (CFG_PHY_OLD_INTERFACE) && (CFG_NUM_PHY > 2)
#error phy configuration not supported
#else
#if CFG_NUM_PHY > 2
		[2] = {
			.mdio_id = BOARD_PHY2_MDIO_ID,
			.config = {
				.phyAddr = BOARD_PHY2_ADDRESS,
				.autoNeg = true,
				.ops = &BOARD_PHY2_OPS,
			},
#ifdef BOARD_PHY2_FIXUP
			.fixup = BOARD_PHY2_FIXUP,
#endif
			.rx_tstamp_latency_100M = BOARD_PHY2_RX_LATENCY_100M,
			.tx_tstamp_latency_100M = BOARD_PHY2_TX_LATENCY_100M,
#ifdef BOARD_PHY2_RX_LATENCY_1G
			.rx_tstamp_latency_1G = BOARD_PHY2_RX_LATENCY_1G,
#endif
#ifdef BOARD_PHY2_TX_LATENCY_1G
			.tx_tstamp_latency_1G = BOARD_PHY2_TX_LATENCY_1G,
#endif
		},
#endif

#if CFG_NUM_PHY > 3
		[3] = {
			.mdio_id = BOARD_PHY3_MDIO_ID,
			.config = {
				.phyAddr = BOARD_PHY3_ADDRESS,
				.autoNeg = true,
				.ops = &BOARD_PHY3_OPS,
			},
#ifdef BOARD_PHY3_FIXUP
			.fixup = BOARD_PHY3_FIXUP,
#endif
			.rx_tstamp_latency_100M = BOARD_PHY3_RX_LATENCY_100M,
			.tx_tstamp_latency_100M = BOARD_PHY3_TX_LATENCY_100M,
#ifdef BOARD_PHY3_RX_LATENCY_1G
			.rx_tstamp_latency_1G = BOARD_PHY3_RX_LATENCY_1G,
#endif
#ifdef BOARD_PHY3_TX_LATENCY_1G
			.tx_tstamp_latency_1G = BOARD_PHY3_TX_LATENCY_1G,
#endif
		},
#endif

#if CFG_NUM_PHY > 4
		[4] = {
			.mdio_id = BOARD_PHY4_MDIO_ID,
			.config = {
				.phyAddr = BOARD_PHY4_ADDRESS,
				.autoNeg = true,
				.ops = &BOARD_PHY4_OPS,
			},
#ifdef BOARD_PHY4_FIXUP
			.fixup = BOARD_PHY4_FIXUP,
#endif
			.rx_tstamp_latency_100M = BOARD_PHY4_RX_LATENCY_100M,
			.tx_tstamp_latency_100M = BOARD_PHY4_TX_LATENCY_100M,
#ifdef BOARD_PHY4_RX_LATENCY_1G
			.rx_tstamp_latency_1G = BOARD_PHY4_RX_LATENCY_1G,
#endif
#ifdef BOARD_PHY4_TX_LATENCY_1G
			.tx_tstamp_latency_1G = BOARD_PHY4_TX_LATENCY_1G,
#endif
		},
#endif

#endif
	},
};

static struct net_phy *__phy_get(unsigned int id)
{
	return &phy_ctx.devices[id];
}

static struct net_phy *phy_get(unsigned int id)
{
	if (id >= CFG_NUM_PHY)
		return NULL;

	return __phy_get(id);
}

void phy_port_status(unsigned int phy_id, struct net_port_status *status)
{
	struct net_phy *phy = __phy_get(phy_id);

	if (phy->duplex == kPHY_FullDuplex)
		status->full_duplex = true;
	else
		status->full_duplex = false;

	switch (phy->speed) {
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

void phy_get_ts_latency(unsigned int phy_id, uint32_t *rx_latency, uint32_t *tx_latency)
{
	struct net_phy *phy = __phy_get(phy_id);

	switch (phy->speed) {
	case kPHY_Speed100M:
		*rx_latency = phy->rx_tstamp_latency_100M;
		*tx_latency = phy->tx_tstamp_latency_100M;
		break;
#if FSL_FEATURE_ENET_HAS_AVB
	case kPHY_Speed1000M:
		*rx_latency = phy->rx_tstamp_latency_1G;
		*tx_latency = phy->tx_tstamp_latency_1G;
		break;
#endif
	default:
		*rx_latency = 0;
		*tx_latency = 0;
		break;
	}
}

static int phy_poll(struct net_phy *phy, bool *link, phy_speed_t *speed, phy_duplex_t *duplex)
{
	/* poll phy status and get up/down/speed duplex. Required to configure MAC with correct speed/duplex */
	if (PHY_GetLinkStatus(&phy->handle, link) != kStatus_Success)
		goto err_status;

	if (*link) {
		/* Get the actual PHY link speed. */
		if (PHY_GetLinkSpeedDuplex(&phy->handle, speed, duplex) != kStatus_Success)
			goto err_speed;
	}

	return 0;

err_speed:
err_status:
	return -1;
}

__init int phy_connect(unsigned int phy_id, phy_callback_t callback, void *data)
{
	struct net_phy *phy = phy_get(phy_id);

	if (!phy)
		goto err;

#ifdef CFG_PHY_OLD_INTERFACE
	phy->mdio_handle.resource.csrClock_Hz = dev_get_enet_core_freq(phy->mdio_handle.resource.base);
	phy->handle.mdioHandle = &phy->mdio_handle;
#else
	phy->resource.read = mdio_read(phy->mdio_id);
	phy->resource.write = mdio_write(phy->mdio_id);
	phy->config.resource = &phy->resource;
#endif

	if (PHY_Init(&phy->handle, &phy->config) != kStatus_Success) {
		os_log(LOG_ERR, "phy(%u) PHY_Init() failed\n", phy_id);
		goto err;
	}

	if (phy->fixup)
		phy->fixup(&phy->handle);

	phy->up = false;
	phy->speed = kPHY_Speed10M;
	phy->duplex = kPHY_HalfDuplex;

	phy->callback = callback;
	phy->data = data;

	phy->enabled = true;

	os_log(LOG_INIT, "phy(%u) connect\n", phy_id);

	return 0;

err:
	return -1;
}

__exit void phy_disconnect(unsigned int phy_id)
{
	struct net_phy *phy = __phy_get(phy_id);

	phy->enabled = false;

	phy->callback = NULL;
	phy->data = NULL;

	os_log(LOG_INIT, "phy(%u) disconnect\n", phy_id);
}

static void phy_status_update(struct net_phy *phy)
{
	bool old_up = phy->up;
	phy_speed_t old_speed = phy->speed;
	phy_duplex_t old_duplex = phy->duplex;

	if (phy_poll(phy, &phy->up, &phy->speed, &phy->duplex) < 0)
		goto out;

	if (phy->up != old_up) {
		if (phy->up == true)
			phy->callback(phy->data, EVENT_PHY_UP, phy->speed, phy->duplex);
		else
			phy->callback(phy->data, EVENT_PHY_DOWN, phy->speed, phy->duplex);

	} else if ((phy->up == true) && ((phy->speed != old_speed) || (phy->duplex != old_duplex))) {
		phy->callback(phy->data, EVENT_PHY_DOWN, phy->speed, phy->duplex);
		phy->callback(phy->data, EVENT_PHY_UP, phy->speed, phy->duplex);
	}

out:
	return;
}

static void phy_task(void *pvParameters)
{
	struct net_phy_ctx *ctx = (struct net_phy_ctx *)pvParameters;
	struct net_phy *phy;
	int i;

	os_log(LOG_INIT, "started\n");

	for (;;) {
		vTaskDelay(pdMS_TO_TICKS(PHY_TASK_PERIOD_MS));

		for (i = 0; i < CFG_NUM_PHY; i++) {
			phy = &ctx->devices[i];

			if (phy->enabled)
				phy_status_update(phy);
		}
	}

	os_log(LOG_INIT, "exited\n");

	vTaskDelete(NULL);
}

__init int phy_init(void)
{
	struct net_phy *phy;
	int i;

	for (i = 0; i < CFG_NUM_PHY; i++) {
		phy = __phy_get(i);

		phy->enabled = false;
	}

	if (xTaskCreate(phy_task, PHY_TASK_NAME, PHY_TASK_STACK_DEPTH, (void *)&phy_ctx, PHY_TASK_PRIORITY, &phy_ctx.task_handle) != pdPASS) {
		os_log(LOG_ERR, "xTaskCreate(%s) failed\n", PHY_TASK_NAME);
		goto err;
	}

	return 0;

err:
	return -1;
}

__exit void phy_exit(void)
{
	vTaskDelete(phy_ctx.task_handle);
	phy_ctx.task_handle = NULL;

	os_log(LOG_INIT, "done\n");
}
#else
void phy_port_status(unsigned int phy_id, struct net_port_status *status) { }
void phy_get_ts_latency(unsigned int phy_id, uint32_t *rx_latency, uint32_t *tx_latency) { }
__init int phy_connect(unsigned int phy_id, phy_callback_t callback, void *data) { return -1; }
__exit void phy_disconnect(unsigned int phy_id) { }
__init int phy_init(void) { return 0; }
__exit void phy_exit(void) { }
#endif /* CFG_NUM_PHY */
