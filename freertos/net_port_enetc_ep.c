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

#include "common/log.h"

#include "net_port_enetc_ep.h"

#include "config.h"

#if CFG_NUM_ENETC_EP_MAC

#include "fsl_netc.h"
#include "fsl_netc_endpoint.h"
#include "fsl_netc_timer.h"
#include "fsl_phy.h"

#include "net_port_netc_1588.h"

#include "net_tx.h"
#include "clock.h"
#include "hw_clock.h"
#include "hw_timer.h"

#include "ptp.h"

/*
BD rings mapping
---------------
ENETC0: 4 Rx/Tx BD rings (all of them can be assigned for EP driver)
ENETC1: 10 Rx/Tx BD rings which can be partitioned as follow:
	- 4 Rx/Tx BD rings for PSI/EP
	- 4 Rx/Tx BD rings for VSI/EP
	- 1 Rx/Tx BD rings for PSI/SWT => BD ring 0, no priority
*/

#define ENETC_EP_BD_ALIGN 128U

/* Tx settings */

#define ENETC_EP_TXBD_NUM 16
#define ENETC_EP0_TXRING_NUM 4U
#define ENETC_EP1_TXRING_NUM 4U

AT_NONCACHEABLE_SECTION_ALIGN(static netc_tx_bd_t ep0_tx_desc[ENETC_EP0_TXRING_NUM][ENETC_EP_TXBD_NUM], ENETC_EP_BD_ALIGN);
static netc_tx_frame_info_t ep0_tx_dirty[ENETC_EP0_TXRING_NUM][ENETC_EP_TXBD_NUM];

#if (CFG_NUM_ENETC_EP_MAC > 1)
AT_NONCACHEABLE_SECTION_ALIGN(static netc_tx_bd_t ep1_tx_desc[ENETC_EP1_TXRING_NUM][ENETC_EP_TXBD_NUM], ENETC_EP_BD_ALIGN);
static netc_tx_frame_info_t ep1_tx_dirty[ENETC_EP1_TXRING_NUM][ENETC_EP_TXBD_NUM];
#endif


/* Rx settings */

#define ENETC_EP_RXBD_NUM 16 /* ENETC rx bd queue size must be a power of 2 */
#define ENETC_EP_BUFF_SIZE_ALIGN 64U
#define ENETC_EP_RXBUFF_SIZE NET_DATA_SIZE
#define ENETC_EP_RXBUFF_SIZE_ALIGN SDK_SIZEALIGN(ENETC_EP_RXBUFF_SIZE, ENETC_EP_BUFF_SIZE_ALIGN)

#define ENETC_EP0_RXRING_NUM 4U
#define ENETC_EP1_RXRING_NUM 4U

typedef uint8_t rx_buffer_t[ENETC_EP_RXBUFF_SIZE_ALIGN];

AT_NONCACHEABLE_SECTION_ALIGN(static netc_rx_bd_t ep0_rx_desc[ENETC_EP0_RXRING_NUM][ENETC_EP_RXBD_NUM], ENETC_EP_BD_ALIGN);
AT_NONCACHEABLE_SECTION_ALIGN(static rx_buffer_t ep0_rx_buff[ENETC_EP0_RXRING_NUM][ENETC_EP_RXBD_NUM/2], ENETC_EP_BUFF_SIZE_ALIGN);
static uint64_t ep0_rx_buff_addr[ENETC_EP0_RXRING_NUM][ENETC_EP_RXBD_NUM/2];

#if (CFG_NUM_ENETC_EP_MAC > 1)
AT_NONCACHEABLE_SECTION_ALIGN(static netc_rx_bd_t ep1_rx_desc[ENETC_EP1_RXRING_NUM][ENETC_EP_RXBD_NUM], ENETC_EP_BD_ALIGN);
AT_NONCACHEABLE_SECTION_ALIGN(static rx_buffer_t ep1_rx_buff[ENETC_EP1_RXRING_NUM][ENETC_EP_RXBD_NUM/2], ENETC_EP_BUFF_SIZE_ALIGN);
static uint64_t ep1_rx_buff_addr[ENETC_EP1_RXRING_NUM][ENETC_EP_RXBD_NUM/2];
#endif

struct enetc_ep_drv {
	ep_handle_t handle;
	ep_config_t config;
	netc_bdr_config_t buffer_config;
	netc_rx_bdr_config_t rxBdrConfig[ENETC_EP0_RXRING_NUM];
	netc_tx_bdr_config_t txBdrConfig[ENETC_EP0_TXRING_NUM];
	struct hw_clock clock;
	void *timer_handle;
};

void netc_ep_pseudo_stats_init(struct net_port *);
void netc_ep_stats_init(struct net_port *);

static struct enetc_ep_drv enetc_ep_drivers[CFG_NUM_ENETC_EP_MAC] = {
	[0] = {
		.rxBdrConfig[0].bdArray = &ep0_rx_desc[0][0],
		.rxBdrConfig[0].len = ENETC_EP_RXBD_NUM,
		.rxBdrConfig[0].extendDescEn = true,
		.rxBdrConfig[0].buffAddrArray =  (uint64_t *)&ep0_rx_buff_addr[0][0],
		.rxBdrConfig[0].buffSize = ENETC_EP_RXBUFF_SIZE_ALIGN,
		.txBdrConfig[0].bdArray = &ep0_tx_desc[0][0],
		.txBdrConfig[0].priority = 2,
		.txBdrConfig[0].len = ENETC_EP_TXBD_NUM,
		.txBdrConfig[0].dirtyArray = &ep0_tx_dirty[0][0],

		.rxBdrConfig[1].bdArray = &ep0_rx_desc[1][0],
		.rxBdrConfig[1].len = ENETC_EP_RXBD_NUM,
		.rxBdrConfig[1].extendDescEn = true,
		.rxBdrConfig[1].buffAddrArray =  (uint64_t *)&ep0_rx_buff_addr[1][0],
		.rxBdrConfig[1].buffSize = ENETC_EP_RXBUFF_SIZE_ALIGN,
		.txBdrConfig[1].bdArray  = &ep0_tx_desc[1][0],
		.txBdrConfig[1].priority = 2,
		.txBdrConfig[1].len = ENETC_EP_TXBD_NUM,
		.txBdrConfig[1].dirtyArray = &ep0_tx_dirty[1][0],

		.rxBdrConfig[2].bdArray = &ep0_rx_desc[2][0],
		.rxBdrConfig[2].len = ENETC_EP_RXBD_NUM,
		.rxBdrConfig[2].extendDescEn = true,
		.rxBdrConfig[2].buffAddrArray =  (uint64_t *)&ep0_rx_buff_addr[2][0],
		.rxBdrConfig[2].buffSize = ENETC_EP_RXBUFF_SIZE_ALIGN,
		.txBdrConfig[2].bdArray  = &ep0_tx_desc[2][0],
		.txBdrConfig[2].priority = 2,
		.txBdrConfig[2].len = ENETC_EP_TXBD_NUM,
		.txBdrConfig[2].dirtyArray = &ep0_tx_dirty[2][0],

		.rxBdrConfig[3].bdArray = &ep0_rx_desc[3][0],
		.rxBdrConfig[3].len = ENETC_EP_RXBD_NUM,
		.rxBdrConfig[3].extendDescEn = true,
		.rxBdrConfig[3].buffAddrArray =  (uint64_t *)&ep0_rx_buff_addr[3][0],
		.rxBdrConfig[3].buffSize = ENETC_EP_RXBUFF_SIZE_ALIGN,
		.txBdrConfig[3].bdArray  = &ep0_tx_desc[3][0],
		.txBdrConfig[3].priority = 2,
		.txBdrConfig[3].len = ENETC_EP_TXBD_NUM,
		.txBdrConfig[3].dirtyArray = &ep0_tx_dirty[3][0],

		.clock = {
			.rate = NSECS_PER_SEC,
			.period = UINT64_MAX,
			.to_ns = {
				.shift = 0,
			},
			.to_cyc = {
				.shift = 0,
			},
		},
	},

#if (CFG_NUM_ENETC_EP_MAC > 1)
	[1] = {
		.rxBdrConfig[0].bdArray = &ep1_rx_desc[0][0],
		.rxBdrConfig[0].len = ENETC_EP_RXBD_NUM,
		.rxBdrConfig[0].extendDescEn = true,
		.rxBdrConfig[0].buffAddrArray =  (uint64_t *)&ep1_rx_buff_addr[0][0],
		.rxBdrConfig[0].buffSize = ENETC_EP_RXBUFF_SIZE_ALIGN,
		.txBdrConfig[0].bdArray = &ep1_tx_desc[0][0],
		.txBdrConfig[0].priority = 2,
		.txBdrConfig[0].len = ENETC_EP_TXBD_NUM,
		.txBdrConfig[0].dirtyArray = &ep1_tx_dirty[0][0],

		.rxBdrConfig[1].bdArray = &ep1_rx_desc[1][0],
		.rxBdrConfig[1].len = ENETC_EP_RXBD_NUM,
		.rxBdrConfig[1].extendDescEn = true,
		.rxBdrConfig[1].buffAddrArray =  (uint64_t *)&ep1_rx_buff_addr[1][0],
		.rxBdrConfig[1].buffSize = ENETC_EP_RXBUFF_SIZE_ALIGN,
		.txBdrConfig[1].bdArray = &ep1_tx_desc[1][0],
		.txBdrConfig[1].priority = 2,
		.txBdrConfig[1].len = ENETC_EP_TXBD_NUM,
		.txBdrConfig[1].dirtyArray = &ep1_tx_dirty[1][0],

		.rxBdrConfig[2].bdArray = &ep1_rx_desc[2][0],
		.rxBdrConfig[2].len = ENETC_EP_RXBD_NUM,
		.rxBdrConfig[2].extendDescEn = true,
		.rxBdrConfig[2].buffAddrArray =  (uint64_t *)&ep1_rx_buff_addr[2][0],
		.rxBdrConfig[2].buffSize = ENETC_EP_RXBUFF_SIZE_ALIGN,
		.txBdrConfig[2].bdArray  = &ep1_tx_desc[2][0],
		.txBdrConfig[2].priority = 2,
		.txBdrConfig[2].len = ENETC_EP_TXBD_NUM,
		.txBdrConfig[2].dirtyArray = &ep1_tx_dirty[2][0],

		.rxBdrConfig[3].bdArray = &ep1_rx_desc[3][0],
		.rxBdrConfig[3].len = ENETC_EP_RXBD_NUM,
		.rxBdrConfig[3].extendDescEn = true,
		.rxBdrConfig[3].buffAddrArray =  (uint64_t *)&ep1_rx_buff_addr[3][0],
		.rxBdrConfig[3].buffSize = ENETC_EP_RXBUFF_SIZE_ALIGN,
		.txBdrConfig[3].bdArray  = &ep1_tx_desc[3][0],
		.txBdrConfig[3].priority = 2,
		.txBdrConfig[3].len = ENETC_EP_TXBD_NUM,
		.txBdrConfig[3].dirtyArray = &ep1_tx_dirty[3][0],

		.clock = {
			.rate = NSECS_PER_SEC,
			.period = UINT64_MAX,
			.to_ns = {
				.shift = 0,
			},
			.to_cyc = {
				.shift = 0,
			},
		},
	},
#endif
};

#define get_drv(port) ((struct enetc_ep_drv *)port->drv)

#define ENETC_TXQ_MAX 4

static struct tx_queue_properties tx_q_capabilites[CFG_NUM_ENETC_EP_MAC]= {
	[0] = {
		.num_queues = ENETC_TXQ_MAX,
		.queue_prop = {
			TX_QUEUE_FLAGS_STRICT_PRIORITY,
			TX_QUEUE_FLAGS_STRICT_PRIORITY,
			TX_QUEUE_FLAGS_STRICT_PRIORITY|TX_QUEUE_FLAGS_CREDIT_SHAPER,
			TX_QUEUE_FLAGS_STRICT_PRIORITY|TX_QUEUE_FLAGS_CREDIT_SHAPER,
		},
	},
#if CFG_NUM_ENETC_EP_MAC > 1
	[1] = {
		.num_queues = ENETC_TXQ_MAX,
		.queue_prop = {
			TX_QUEUE_FLAGS_STRICT_PRIORITY,
			TX_QUEUE_FLAGS_STRICT_PRIORITY,
			TX_QUEUE_FLAGS_STRICT_PRIORITY|TX_QUEUE_FLAGS_CREDIT_SHAPER,
			TX_QUEUE_FLAGS_STRICT_PRIORITY|TX_QUEUE_FLAGS_CREDIT_SHAPER,
		},
	},

#endif
};

static struct tx_queue_properties tx_q_default_config[CFG_NUM_ENETC_EP_MAC] = {
	[0] = {
		.num_queues = ENETC_TXQ_MAX,
		.queue_prop = {
			TX_QUEUE_FLAGS_STRICT_PRIORITY,
			TX_QUEUE_FLAGS_STRICT_PRIORITY,
			TX_QUEUE_FLAGS_STRICT_PRIORITY|TX_QUEUE_FLAGS_CREDIT_SHAPER,
			TX_QUEUE_FLAGS_STRICT_PRIORITY|TX_QUEUE_FLAGS_CREDIT_SHAPER,
		},
	},
#if CFG_NUM_ENETC_EP_MAC > 1
	[1] = {
		.num_queues = ENETC_TXQ_MAX,
		.queue_prop = {
			TX_QUEUE_FLAGS_STRICT_PRIORITY,
			TX_QUEUE_FLAGS_STRICT_PRIORITY,
			TX_QUEUE_FLAGS_STRICT_PRIORITY|TX_QUEUE_FLAGS_CREDIT_SHAPER,
			TX_QUEUE_FLAGS_STRICT_PRIORITY|TX_QUEUE_FLAGS_CREDIT_SHAPER,
		},
	},
#endif
};

void *enetc_ep_get_handle(struct net_port *port)
{
	struct enetc_ep_drv *drv = get_drv(port);

	return &drv->handle;
}

void *enetc_ep_get_port_handle(struct net_port *port)
{
	struct enetc_ep_drv *drv = get_drv(port);

	return drv->handle.hw.portGroup.eth;
}

void *enetc_ep_get_pseudo_port_handle(struct net_port *port)
{
	struct enetc_ep_drv *drv = get_drv(port);

	return drv->handle.hw.portGroup.pseudo;
}

static int enetc_ep_add_multi(struct net_port *port, uint8_t *addr)
{
	return 0;
}

static int enetc_ep_del_multi(struct net_port *port, uint8_t *addr)
{
	return 0;
}

static status_t enetc_ep_reclaim_cb(ep_handle_t *handle, uint8_t ring, netc_tx_frame_info_t *frameInfo, void *userData)
{
	struct net_port *port = (struct net_port *)userData;
	struct enetc_ep_drv *drv = get_drv(port);
	struct net_tx_desc *desc;

	if (frameInfo) {
		desc = (struct net_tx_desc *)frameInfo->context;
		if (desc) {
			if (frameInfo->isTsAvail) {
				uint64_t cycles, ts;

				cycles = netc_1588_hwts_to_u64(drv->timer_handle, frameInfo->timestamp);

				ts = hw_clock_cycles_to_time(port->hw_clock, cycles) + port->tx_tstamp_latency;

				ptp_tx_ts(port, ts, desc->priv);
			}

			port_tx_clean_desc(port, (unsigned long)desc);
		}
	}

	return kStatus_Success;
}

static int enetc_ep_get_rx_frame_size(struct net_port *port, uint32_t *length, uint32_t queue)
{
	struct enetc_ep_drv *drv = get_drv(port);
	status_t rc;

	rc = EP_GetRxFrameSize(&drv->handle, 0, length);
	if (rc == kStatus_Success)
		return 1;
	else if (rc == kStatus_NETC_RxFrameEmpty)
		return 0;
	else
		return -1;
}

static int enetc_ep_read_frame(struct net_port *port, uint8_t *data, uint32_t length, uint64_t *ts, uint32_t queue)
{
	struct enetc_ep_drv *drv = get_drv(port);
	netc_frame_attr_t attr;

	if (EP_ReceiveFrameCopy(&drv->handle, 0, data, length, &attr)== kStatus_Success) {
		if (ts && attr.isTsAvail)
			*ts = netc_1588_hwts_to_u64(drv->timer_handle, attr.timestamp);

		return 0;
	} else {
		return -1;
	}
}

#define MIN_PACKET_SIZE 64
static int enetc_ep_send_frame(struct net_port *port, uint8_t *data, uint32_t length, struct net_tx_desc *desc, uint32_t queue, bool need_ts)
{
	netc_buffer_struct_t txBuff = {.buffer = data};
	netc_frame_struct_t txFrame = {.buffArray = &txBuff, .length = 1};
	struct enetc_ep_drv *drv = get_drv(port);
	ep_tx_opt opt;

	//FIXME required only for NETC standalone EP
	//if (port->base == kNETC_ENETC0PSI0)
	if (desc->len < MIN_PACKET_SIZE)
		desc->len = MIN_PACKET_SIZE;

	txBuff.length = desc->len;

	opt.flags = (need_ts)? kEP_TX_OPT_REQ_TS: 0;

	if (EP_SendFrame(&drv->handle, 1, &txFrame, (void *)desc, &opt) ==  kStatus_Success) {
		return 1;
	} else {
		return -1;
	}
}

static void enetc_ep_tx_cleanup(struct net_port *port)
{
	struct enetc_ep_drv *drv = get_drv(port);

	EP_ReclaimTxDescriptor(&drv->handle, 1);
}

static void enetc_ep_link_up(struct net_port *port)
{
	struct enetc_ep_drv *drv = get_drv(port);
	uint16_t speed;

	/* Use the actual speed and duplex when phy success
	 * to finish the autonegotiation.
	 */
	switch (port->phy_speed) {
	case kPHY_Speed10M:
	default:
		drv->config.port.ethMac.miiSpeed = kNETC_MiiSpeed10M;
		speed = 0;
		break;

	case kPHY_Speed100M:
		drv->config.port.ethMac.miiSpeed = kNETC_MiiSpeed100M;
		speed = 9;
		break;

	case kPHY_Speed1000M:
		drv->config.port.ethMac.miiSpeed = kNETC_MiiSpeed1000M;
		speed = 99;
		break;
	}

	switch (port->phy_duplex) {
	case kPHY_HalfDuplex:
	default:
		drv->config.port.ethMac.miiDuplex = kNETC_MiiHalfDuplex;
		break;

	case kPHY_FullDuplex:
		drv->config.port.ethMac.miiDuplex = kNETC_MiiFullDuplex;
		break;
	}

	EP_Up(&drv->handle, drv->config.port.ethMac.miiSpeed, drv->config.port.ethMac.miiDuplex);
	EP_SetPortSpeed(&drv->handle, speed);
}

static void enetc_ep_link_down(struct net_port *port)
{
	//struct enetc_ep_drv *drv = get_drv(port);

	//EP_Down(&drv->handle);
}

static int enetc_ep_set_tx_queue_config(struct net_port *port, struct tx_queue_properties *tx_q_cfg)
{
	return 0;
}

static int enetc_ep_set_tx_idle_slope(struct net_port *port, unsigned int idle_slope, uint32_t queue)
{
	return -1;
}

static int enetc_ep_set_st_config(struct net_port *port, struct genavb_st_config *config)
{
	return 0;
}

static int enetc_ep_get_st_config(struct net_port *port, genavb_st_config_type_t type, struct genavb_st_config *config, unsigned int list_length)
{
	return 0;
}

static int enetc_ep_set_fp(struct net_port *port, unsigned int type, struct genavb_fp_config *config)
{
	return 0;
}

static int enetc_ep_get_fp(struct net_port *port, unsigned int type, struct genavb_fp_config *config)
{
	return 0;
}

static int enetc_ep_post_init(struct net_port *port)
{
	return 0;
}

static void enetc_ep_pre_exit(struct net_port *port)
{

}

__exit static void enetc_ep_exit(struct net_port *port)
{
	struct enetc_ep_drv *drv = get_drv(port);

	hw_clock_unregister(clock_to_hw_clock(port->clock_local));

	netc_1588_exit();

	EP_Deinit(&drv->handle);

	port->drv = NULL;
}

__init int enetc_ep_init(struct net_port *port)
{
	struct enetc_ep_drv *drv;
	status_t result;
	uint8_t ring, index;

	if (port->drv_index >= CFG_NUM_ENETC_EP_MAC)
		goto err_drv_index;

	port->drv_ops.add_multi = enetc_ep_add_multi;
	port->drv_ops.del_multi = enetc_ep_del_multi;
	port->drv_ops.get_rx_frame_size = enetc_ep_get_rx_frame_size;
	port->drv_ops.read_frame = enetc_ep_read_frame;
	port->drv_ops.send_frame = enetc_ep_send_frame;
	port->drv_ops.tx_cleanup = enetc_ep_tx_cleanup;
	port->drv_ops.link_up = enetc_ep_link_up;
	port->drv_ops.link_down = enetc_ep_link_down;
	port->drv_ops.set_tx_queue_config = enetc_ep_set_tx_queue_config;
	port->drv_ops.set_tx_idle_slope = enetc_ep_set_tx_idle_slope;
	port->drv_ops.set_st_config = enetc_ep_set_st_config;
	port->drv_ops.get_st_config = enetc_ep_get_st_config;
	port->drv_ops.set_fp = enetc_ep_set_fp;
	port->drv_ops.get_fp = enetc_ep_get_fp;
	port->drv_ops.exit = enetc_ep_exit;
	port->drv_ops.post_init = enetc_ep_post_init;
	port->drv_ops.pre_exit = enetc_ep_pre_exit;

	port->drv = &enetc_ep_drivers[port->drv_index];

	port->tx_q_cap = &tx_q_capabilites[port->drv_index];
	port->num_tx_q = tx_q_default_config[port->drv_index].num_queues;
	port->num_rx_q = 1;

	drv = get_drv(port);

	for (index = 0U; index < ENETC_EP_RXBD_NUM/2; index++) {
		if (port->drv_index == 0) {
			for (ring = 0U; ring < ENETC_EP0_RXRING_NUM; ring++)
				ep0_rx_buff_addr[ring][index] = (uintptr_t)&ep0_rx_buff[ring][index];
		} else {
		#if (CFG_NUM_ENETC_EP_MAC > 1)
			for (ring = 0U; ring < ENETC_EP1_RXRING_NUM; ring++)
				ep1_rx_buff_addr[ring][index] = (uintptr_t)&ep1_rx_buff[ring][index];
		#endif
		}
	}

	/* Endpoint configuration. */
	EP_GetDefaultConfig(&drv->config);

	drv->config.si = port->base;
	drv->config.siConfig.txRingUse = (port->drv_index == 0)? ENETC_EP0_TXRING_NUM : ENETC_EP1_TXRING_NUM;
	drv->config.siConfig.rxRingUse = (port->drv_index == 0)? ENETC_EP0_RXRING_NUM : ENETC_EP1_RXRING_NUM;
	drv->config.reclaimCallback = enetc_ep_reclaim_cb;
	drv->config.userData = (void *)port;
	drv->config.port.ethMac.miiMode = port->mii_mode;
	drv->config.port.ethMac.miiSpeed  = port->phy_speed;
	drv->config.port.ethMac.miiDuplex = port->phy_duplex;

	drv->buffer_config.rxBdrConfig = drv->rxBdrConfig;
	drv->buffer_config.txBdrConfig = drv->txBdrConfig;

	result = EP_Init(&drv->handle, port->mac_addr, &drv->config, &drv->buffer_config);
	if (result != kStatus_Success) {
		os_log(LOG_ERR, "EP_Init returned error %d\n", result);
		goto err_ep_init;
	}

	drv->timer_handle = netc_1588_init();
	if (!drv->timer_handle) {
		os_log(LOG_ERR, "netc_1588_init() failed\n");
		goto err_1588_init;
	}

	drv->clock.priv = drv->timer_handle;
	drv->clock.read_counter = netc_1588_read_counter;
	drv->clock.adj_freq = netc_1588_clock_adj_freq;

	if (hw_clock_register(clock_to_hw_clock(port->clock_local), &drv->clock) < 0) {
		os_log(LOG_ERR, "failed to register hw_clock(%d), port(%p)\n", clock_to_hw_clock(port->clock_local), port);
		goto err_clock_register;
	}

	port->hw_clock = &drv->clock;

	if (port->drv_type == ENETC_1G_t)
		netc_ep_stats_init(port);
	else
		netc_ep_pseudo_stats_init(port);

	os_log(LOG_INFO, "port(%u) driver handle (%p)\n", port->index, drv);

	return 0;

err_clock_register:
	netc_1588_exit();

err_1588_init:
	EP_Deinit(&drv->handle);

err_ep_init:
err_drv_index:
	return -1;
}
#else
int enetc_ep_init(struct net_port *port) { return -1; }
void *enetc_ep_get_handle(struct net_port *port) { return NULL; }
#endif /* CFG_NUM_ENETC_EP_MAC */

