/*
* Copyright 2022-2024 NXP
*
* SPDX-License-Identifier: BSD-3-Clause
*/

/**
 @file
 @brief RTOS specific Network service implementation
 @details
*/

#include "common/log.h"

#include "net_port_enetc_ep.h"
#include "net_port_netc_sw_drv.h"

#include "config.h"

#if CFG_NUM_ENETC_EP_MAC

#include "genavb/error.h"

#include "fsl_netc.h"
#include "fsl_netc_endpoint.h"
#include "fsl_netc_timer.h"
#include "fsl_phy.h"

#include "net_port_netc_1588.h"

#include "net_tx.h"
#if (defined(BOARD_ENETC0_RX_ZERO_COPY) || defined(BOARD_ENETC1_RX_ZERO_COPY))
#include "net_rx.h"
#endif
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

AT_NONCACHEABLE_SECTION_ALIGN(static netc_tx_bd_t ep0_tx_desc[ENETC_EP0_TXRING_NUM * ENETC_EP_TXBD_NUM], ENETC_EP_BD_ALIGN);
static netc_tx_frame_info_t ep0_tx_dirty[ENETC_EP0_TXRING_NUM * ENETC_EP_TXBD_NUM];

#if (CFG_NUM_ENETC_EP_MAC > 1)
AT_NONCACHEABLE_SECTION_ALIGN(static netc_tx_bd_t ep1_tx_desc[ENETC_EP1_TXRING_NUM * ENETC_EP_TXBD_NUM], ENETC_EP_BD_ALIGN);
static netc_tx_frame_info_t ep1_tx_dirty[ENETC_EP1_TXRING_NUM * ENETC_EP_TXBD_NUM];
#endif

#define ENETC_EP_TXRING_MAX ((ENETC_EP0_TXRING_NUM > ENETC_EP1_TXRING_NUM) ? ENETC_EP0_TXRING_NUM : ENETC_EP1_TXRING_NUM)

/* Rx settings */

#define ENETC_EP_RXBD0_NUM 32 /* ENETC rx bd queue size must be a power of 2 */
#define ENETC_EP_RXBDn_NUM 16
#define ENETC_EP_BUFF_SIZE_ALIGN 64U
#define ENETC_EP_RXBUFF_SIZE 256
#define ENETC_EP_RXBUFF_SIZE_ALIGN SDK_SIZEALIGN(ENETC_EP_RXBUFF_SIZE, ENETC_EP_BUFF_SIZE_ALIGN)

#define ENETC_EP0_RXRING_NUM 4U
#define ENETC_EP1_RXRING_NUM 4U

#define ENETC_EP0_RXBD_NUM	(ENETC_EP_RXBD0_NUM + (ENETC_EP0_RXRING_NUM - 1) * ENETC_EP_RXBDn_NUM)
#define ENETC_EP1_RXBD_NUM	(ENETC_EP_RXBD0_NUM + (ENETC_EP1_RXRING_NUM - 1) * ENETC_EP_RXBDn_NUM)

#define ENETC_EP_RXRING_MAX ((ENETC_EP0_RXRING_NUM > ENETC_EP1_RXRING_NUM) ? ENETC_EP0_RXRING_NUM : ENETC_EP1_RXRING_NUM)

typedef uint8_t rx_buffer_t[ENETC_EP_RXBUFF_SIZE_ALIGN];

AT_NONCACHEABLE_SECTION_ALIGN(static netc_rx_bd_t ep0_rx_desc[ENETC_EP0_RXBD_NUM], ENETC_EP_BD_ALIGN);
#if (!defined(BOARD_ENETC0_RX_ZERO_COPY))
__ep_rx_buffers SDK_ALIGN(static rx_buffer_t ep0_rx_buff[ENETC_EP0_RXBD_NUM/2], ENETC_EP_BUFF_SIZE_ALIGN);
#endif
static uint64_t ep0_rx_buff_addr[ENETC_EP0_RXBD_NUM/2];

#if (CFG_NUM_ENETC_EP_MAC > 1)
AT_NONCACHEABLE_SECTION_ALIGN(static netc_rx_bd_t ep1_rx_desc[ENETC_EP1_RXBD_NUM], ENETC_EP_BD_ALIGN);
#if (!defined(BOARD_ENETC1_RX_ZERO_COPY))
__ep_rx_buffers SDK_ALIGN(static rx_buffer_t ep1_rx_buff[ENETC_EP1_RXBD_NUM/2], ENETC_EP_BUFF_SIZE_ALIGN);
#endif
static uint64_t ep1_rx_buff_addr[ENETC_EP1_RXBD_NUM/2];
#endif /* #if (CFG_NUM_ENETC_EP_MAC > 1) */

#if (defined(BOARD_ENETC0_RX_ZERO_COPY) || defined(BOARD_ENETC1_RX_ZERO_COPY))
#define MAX_RX_BUFF 6
#endif

enum {
	ENETC_EP_FP_EVENT_LINK_FAIL,
	ENETC_EP_FP_EVENT_LINK_OK,
};

/* Command settings */

#define ENETC_EP_CMDBD_NUM 8

AT_NONCACHEABLE_SECTION_ALIGN(static netc_cmd_bd_t ep0_g_cmd_buff_desc[ENETC_EP_CMDBD_NUM], ENETC_EP_BD_ALIGN);

#if (CFG_NUM_ENETC_EP_MAC > 1)
AT_NONCACHEABLE_SECTION_ALIGN(static netc_cmd_bd_t ep1_g_cmd_buff_desc[ENETC_EP_CMDBD_NUM], ENETC_EP_BD_ALIGN);
#endif

struct enetc_ep_drv {
	ep_handle_t handle;
	bool st_enabled;
	bool is_pseudo;
	bool fp_enabled;
};

void netc_ep_pseudo_stats_init(struct net_port *);
void netc_ep_stats_init(struct net_port *);

static struct enetc_ep_drv enetc_ep_drivers[CFG_NUM_ENETC_EP_MAC];

static struct tx_queue_properties tx_q_capabilites[CFG_NUM_ENETC_EP_MAC]= {
	[0] = {
		.num_queues = ENETC_EP0_TXRING_NUM,
		.queue_prop = {
			TX_QUEUE_FLAGS_STRICT_PRIORITY,
			TX_QUEUE_FLAGS_STRICT_PRIORITY,
			TX_QUEUE_FLAGS_STRICT_PRIORITY|TX_QUEUE_FLAGS_CREDIT_SHAPER,
			TX_QUEUE_FLAGS_STRICT_PRIORITY|TX_QUEUE_FLAGS_CREDIT_SHAPER,
		},
	},
#if CFG_NUM_ENETC_EP_MAC > 1
	[1] = {
		.num_queues = ENETC_EP1_TXRING_NUM,
		.queue_prop = {
			TX_QUEUE_FLAGS_STRICT_PRIORITY,
			TX_QUEUE_FLAGS_STRICT_PRIORITY,
			TX_QUEUE_FLAGS_STRICT_PRIORITY|TX_QUEUE_FLAGS_CREDIT_SHAPER,
			TX_QUEUE_FLAGS_STRICT_PRIORITY|TX_QUEUE_FLAGS_CREDIT_SHAPER,
		},
	},
#endif
};

static inline bool fp_is_enabled(struct enetc_ep_drv *drv)
{
	return drv->fp_enabled;
}

static inline void fp_set_enabled(struct enetc_ep_drv *drv, bool enable)
{
	drv->fp_enabled = enable;
}

static inline void st_set(struct enetc_ep_drv *drv, bool enable)
{
	drv->st_enabled = enable;
}

static inline bool st_is_enabled(struct enetc_ep_drv *drv)
{
	return drv->st_enabled;
}

static inline bool st_is_same(struct enetc_ep_drv *drv, bool enable)
{
	return (enable == st_is_enabled(drv));
}

void *enetc_ep_get_handle(struct net_port *port)
{
	struct enetc_ep_drv *drv = net_port_drv(port);

	return &drv->handle;
}

void *enetc_ep_get_link_handle(struct net_port *port)
{
	struct enetc_ep_drv *drv = net_port_drv(port);

	if (!drv->is_pseudo)
		return drv->handle.hw.portGroup.eth;
	else
		return drv->handle.hw.portGroup.pseudo;
}

void *enetc_ep_get_si_handle(struct net_port *port)
{
	struct enetc_ep_drv *drv = net_port_drv(port);

	return drv->handle.hw.si;
}

void *enetc_ep_get_port_handle(struct net_port *port)
{
	struct enetc_ep_drv *drv = net_port_drv(port);

	return drv->handle.hw.portGroup.port;
}

static int enetc_ep_add_multi(struct net_port *port, uint8_t *addr)
{
	struct enetc_ep_drv *drv = net_port_drv(port);

	/* Add entry into Rx L2 MAC Address hash filter*/
	if (EP_RxL2MFAddHashEntry(&drv->handle, kNETC_PacketMulticast, addr) == kStatus_Success) {
		return 0;
	} else {
		return -1;
	}
}

static int enetc_ep_del_multi(struct net_port *port, uint8_t *addr)
{
	struct enetc_ep_drv *drv = net_port_drv(port);

	/* Delete entry from Rx L2 MAC Address hash filter*/
	if (EP_RxL2MFDelHashEntry(&drv->handle, kNETC_PacketMulticast, addr) == kStatus_Success) {
		return 0;
	} else {
		return -1;
	}
}

static status_t enetc_ep_reclaim_cb(ep_handle_t *handle, uint8_t ring, netc_tx_frame_info_t *frameInfo, void *userData)
{
	struct net_port *port = (struct net_port *)userData;
	struct net_tx_desc *desc;

	if (frameInfo) {
		desc = (struct net_tx_desc *)frameInfo->context;
		if (desc) {
			if (frameInfo->isTsAvail) {
				uint64_t cycles, ts;

				cycles = netc_1588_hwts_to_u64(port->hw_clock, frameInfo->timestamp);

				ts = hw_clock_cycles_to_time(port->hw_clock, cycles) + port->tx_tstamp_latency;

				ptp_tx_ts(port, ts, desc->priv);
			}

			port_tx_clean_desc(port, (unsigned long)desc);
		}
	}

	return kStatus_Success;
}

#if (defined(BOARD_ENETC0_RX_ZERO_COPY) || defined(BOARD_ENETC1_RX_ZERO_COPY))
static void *enetc_ep_rxbuff_alloc(ep_handle_t *handle, uint8_t ring, uint32_t length, void *userData)
{
	struct net_port *port = (struct net_port *)userData;
	struct net_rx_desc *desc;
	uint8_t *buff, *buff_unaligned;
	uint32_t offset;

	desc = net_pool_rx_alloc(length + ENETC_EP_BUFF_SIZE_ALIGN);
	if (!desc) {
		port->stats[ring].rx_alloc_err++;
		buff = NULL;
		goto err;
	}

	buff_unaligned = (uint8_t *)desc + NET_DATA_OFFSET;

	/* Buffers need to be ENETC_EP_BUFF_SIZE_ALIGN Bytes aligned */
	/* We store the required alignment offset just before the buffer address, so we can retrieve it later (on alloc and free functions) */
	buff = (uint8_t *)(((uintptr_t)buff_unaligned + ENETC_EP_BUFF_SIZE_ALIGN - 1) & ~(ENETC_EP_BUFF_SIZE_ALIGN - 1));
	offset = (uint32_t)(buff - buff_unaligned);
	*((uint32_t *)buff - 1) = offset;

err:
	return buff;
}

static void enetc_ep_rxbuff_free(ep_handle_t *handle, uint8_t ring, void *buff, void *userData)
{
	struct net_rx_desc *desc;
	uint32_t offset;

	offset = *((uint32_t *)buff - 1);
	desc = (struct net_rx_desc *)((uint8_t *)buff - NET_DATA_OFFSET - offset);
	net_rx_free(desc);
}

static uint32_t enetc_ep_get_total_buffs_len(netc_frame_struct_t *rx_frame)
{
	uint32_t len;
	int i;

	len = 0;
	for (i = 0; i < rx_frame->length; i++)
		len += rx_frame->buffArray[i].length;

	return len;
}

static struct net_rx_desc *enetc_ep_rx_frames_to_rx_desc(struct net_port *port, netc_frame_struct_t *rx_frame, uint8_t ring)
{
	struct enetc_ep_drv *drv = net_port_drv(port);
	struct net_rx_desc *desc = NULL;
	uint32_t offset;

	if (rx_frame->length == 1) {
		offset = *((uint32_t *)rx_frame->buffArray[0].buffer - 1);
		desc = (struct net_rx_desc *)((uint8_t *)rx_frame->buffArray[0].buffer - NET_DATA_OFFSET - offset);
		desc->l2_offset = NET_DATA_OFFSET + offset;
		desc->len = rx_frame->buffArray[0].length;
	} else if (rx_frame->length > 1) {
		uint32_t total_len;
		netc_buffer_struct_t *bs;
		uint8_t *desc_data;
		int i;

		total_len = enetc_ep_get_total_buffs_len(rx_frame);

		desc = net_pool_rx_alloc(total_len);
		if (!desc) {
			port->stats[ring].rx_alloc_err++;
			net_rx_free(desc);
			goto err;
		}

		desc->len = total_len;
		desc_data = (uint8_t *)desc + desc->l2_offset;
		for (i = 0; i < rx_frame->length; i++) {
			bs = &rx_frame->buffArray[i];
			memcpy(desc_data, bs->buffer, bs->length);
			desc_data += bs->length;
			enetc_ep_rxbuff_free(&drv->handle, ring, bs->buffer, drv->handle.cfg.userData);
		}
	} else {
		goto err;
	}

err:
	return desc;
}

static int enetc_ep_read_frame_zero_copy(struct net_port *port, struct net_rx_desc **desc, unsigned int *port_index, uint64_t *ts, uint32_t queue)
{
	struct enetc_ep_drv *drv = net_port_drv(port);
	netc_buffer_struct_t rxBuff[MAX_RX_BUFF];
	netc_frame_struct_t rxFrame = {.buffArray = rxBuff, .length = MAX_RX_BUFF};
	netc_frame_attr_t attr;
	status_t result;
	int rc;

	result = EP_ReceiveFrame(&drv->handle, queue, &rxFrame, &attr);
	if (result == kStatus_Success) {
		*desc = enetc_ep_rx_frames_to_rx_desc(port, &rxFrame, queue);
		if (!*desc) {
			rc = -1;
			goto err;
		}

		if (ts && attr.isTsAvail)
			*ts = netc_1588_hwts_to_u64(port->hw_clock, attr.timestamp);

#if CFG_NUM_NETC_SW && defined(CFG_DSA_CPU_PORT_ENETC)
		if (drv->is_pseudo)
			*port_index = netc_sw_port_to_net_port[attr.srcPort];
		else
#endif
		*port_index = port->index;

		rc = 1;
	} else if (result == kStatus_NETC_RxFrameEmpty) {
		rc = 0;
	} else {
		rc = -1;
	}

err:
	return rc;
}
#else
static void *enetc_ep_rxbuff_alloc(ep_handle_t *handle, uint8_t ring, uint32_t length, void *userData) {return NULL;}
static int enetc_ep_read_frame_zero_copy(struct net_port *port, struct net_rx_desc **desc, unsigned int *port_index, uint64_t *ts, uint32_t queue) {return -1;}
static void enetc_ep_rxbuff_free(ep_handle_t *handle, uint8_t ring, void *buff, void *userData) {}
#endif /* #if (defined(BOARD_ENETC0_RX_ZERO_COPY) || defined(BOARD_ENETC1_RX_ZERO_COPY)) */

static int enetc_ep_get_rx_frame_size(struct net_port *port, uint32_t *length, uint32_t queue)
{
	struct enetc_ep_drv *drv = net_port_drv(port);
	status_t rc;

	rc = EP_GetRxFrameSize(&drv->handle, queue, length);
	if (rc == kStatus_Success)
		return 1;
	else if (rc == kStatus_NETC_RxFrameEmpty)
		return 0;
	else
		return -1;
}

static int enetc_ep_read_frame(struct net_port *port, uint8_t *data, uint32_t length, unsigned int *port_index, uint64_t *ts, uint32_t queue)
{
	struct enetc_ep_drv *drv = net_port_drv(port);
	netc_frame_attr_t attr;

	if (EP_ReceiveFrameCopy(&drv->handle, queue, data, length, &attr)== kStatus_Success) {
		if (ts && attr.isTsAvail)
			*ts = netc_1588_hwts_to_u64(port->hw_clock, attr.timestamp);

#if CFG_NUM_NETC_SW && defined(CFG_DSA_CPU_PORT_ENETC)
		if (drv->is_pseudo)
			*port_index = netc_sw_port_to_net_port[attr.srcPort];
		else
#endif
		*port_index = port->index;

		return 0;
	} else {
		return -1;
	}
}

#define MIN_PACKET_SIZE 64
static int enetc_ep_send_frame(struct net_port *port, struct net_tx_desc *desc, uint32_t queue)
{
	netc_buffer_struct_t txBuff = {.buffer = (uint8_t *)desc + desc->l2_offset};
	netc_frame_struct_t txFrame = {.buffArray = &txBuff, .length = 1};
	struct enetc_ep_drv *drv = net_port_drv(port);
	ep_tx_opt opt;

	//FIXME required only for NETC standalone EP
	//if (port->base == kNETC_ENETC0PSI0)
	if (desc->len < MIN_PACKET_SIZE)
		desc->len = MIN_PACKET_SIZE;

	txBuff.length = desc->len;

	opt.flags = (desc->flags & NET_TX_FLAGS_HW_TS) ? kEP_TX_OPT_REQ_TS: 0;

	if (desc->flags & NET_TX_FLAGS_TS64) {
		opt.flags |= kEP_TX_OPT_START_TIME;

		/* In 32ns units */
		opt.timestamp = ((uint32_t)desc->ts64) >> 5U;
	}

	if (EP_SendFrame(&drv->handle, queue, &txFrame, (void *)desc, &opt) == kStatus_Success) {
		return 0;
	} else {
		return -1;
	}
}

static void enetc_ep_tx_cleanup(struct net_port *port)
{
	struct enetc_ep_drv *drv = net_port_drv(port);
	unsigned int ring;

	for (ring = 0; ring < port->num_tx_q; ring++)
		EP_ReclaimTxDescriptor(&drv->handle, ring);
}

static void enetc_ep_port_fp_event(struct net_port *port, unsigned int event)
{
	struct enetc_ep_drv *drv = net_port_drv(port);

	rtos_mutex_lock(&port->config_mutex, RTOS_WAIT_FOREVER);

	if (fp_is_enabled(drv)) {
		NETC_ETH_LINK_Type *base_eth = enetc_ep_get_link_handle(port);

		switch (event) {
		case ENETC_EP_FP_EVENT_LINK_FAIL:
			base_eth->MAC_MERGE_MMCSR |= NETC_ETH_LINK_MAC_MERGE_MMCSR_LINK_FAIL(1U);
			break;
		case ENETC_EP_FP_EVENT_LINK_OK:
			base_eth->MAC_MERGE_MMCSR &= ~(NETC_ETH_LINK_MAC_MERGE_MMCSR_LINK_FAIL(1U));
			break;
		default:
			break;
		}

	}

	rtos_mutex_unlock(&port->config_mutex);
}

static void enetc_ep_link_up(struct net_port *port)
{
	struct enetc_ep_drv *drv = net_port_drv(port);
	netc_hw_mii_speed_t miiSpeed;
	netc_hw_mii_duplex_t miiDuplex;
	uint16_t speed;

	/* Use the actual speed and duplex when phy success
	 * to finish the autonegotiation.
	 */
	if (port->phy_index == -1) {
		port->phy_speed = kPHY_Speed1000M;
		port->phy_duplex = kPHY_FullDuplex;
	}

	switch (port->phy_speed) {
	case kPHY_Speed10M:
	default:
		miiSpeed = kNETC_MiiSpeed10M;
		speed = 0;
		break;

	case kPHY_Speed100M:
		miiSpeed = kNETC_MiiSpeed100M;
		speed = 9;
		break;

	case kPHY_Speed1000M:
		miiSpeed = kNETC_MiiSpeed1000M;
		speed = 99;
		break;
	}

	switch (port->phy_duplex) {
	case kPHY_HalfDuplex:
	default:
		miiDuplex = kNETC_MiiHalfDuplex;
		break;

	case kPHY_FullDuplex:
		miiDuplex = kNETC_MiiFullDuplex;
		break;
	}

	EP_Up(&drv->handle, miiSpeed, miiDuplex);
	EP_SetPortSpeed(&drv->handle, speed);

	enetc_ep_port_fp_event(port, ENETC_EP_FP_EVENT_LINK_OK);
}

static void enetc_ep_link_down(struct net_port *port)
{
	//struct enetc_ep_drv *drv = net_port_drv(port);

	//EP_Down(&drv->handle);

	enetc_ep_port_fp_event(port, ENETC_EP_FP_EVENT_LINK_FAIL);
}

static int enetc_ep_set_tx_queue_config(struct net_port *port, struct tx_queue_properties *tx_q_cfg)
{
	return 0;
}

static int enetc_ep_set_tx_idle_slope(struct net_port *port, uint64_t idle_slope, uint32_t queue)
{
	return -1;
}

static int enetc_ep_enable_st_config(struct net_port *port, bool enable)
{
	struct enetc_ep_drv *drv = net_port_drv(port);
	status_t status;
	int rc = GENAVB_SUCCESS;

	if (st_is_same(drv, enable))
		goto out;

	/* Port TGS affects several independent features and can "never" be disabled.
	   Just togle it, to go back to an empty gate control list */
	if (!enable) {
#if (defined(FSL_FEATURE_NETC_HAS_ERRATA_051130) && FSL_FEATURE_NETC_HAS_ERRATA_051130)
		status = EP_TxPortTGSEnable(&drv->handle, false, 0xff);
#else
		status = EP_TxPortTGSEnable(&drv->handle, false);
#endif
		if (status != kStatus_Success) {
			os_log(LOG_ERR, "EP_TxPortTGSEnable(false) failed\n");
			rc = -GENAVB_ERR_ST_HW_CONFIG;
			goto err;
		}

#if (defined(FSL_FEATURE_NETC_HAS_ERRATA_051130) && FSL_FEATURE_NETC_HAS_ERRATA_051130)
		status = EP_TxPortTGSEnable(&drv->handle, true, 0xff);
#else
		status = EP_TxPortTGSEnable(&drv->handle, true);
#endif

		if (status != kStatus_Success) {
			os_log(LOG_ERR, "EP_TxPortTGSEnable(true) failed\n");
			rc = -GENAVB_ERR_ST_HW_CONFIG;
			goto err;
		}
	}

	st_set(drv, enable);

err:
out:
	return rc;
}

static int enetc_ep_set_admin_config(struct net_port *port, struct genavb_st_config *config)
{
	struct enetc_ep_drv *drv = net_port_drv(port);
	netc_tb_tgs_gcl_t time_gate_control_list;
	netc_tgs_gate_entry_t *gate_control_entries = NULL;
	netc_tb_tgs_entry_id_t entry_id = port->base == kNETC_ENETC0PSI0 ? kNETC_TGSEnetc0Port : kNETC_TGSEnetc1Port;
	int rc = GENAVB_SUCCESS;
	int i;

	if (config->list_length > NETC_TB_TGS_MAX_ENTRY) {
		os_log(LOG_ERR, "list_length: %u invalid (max: %u)\n", config->list_length, NETC_TB_TGS_MAX_ENTRY);
		rc = -GENAVB_ERR_ST_INVALID_GCL_SIZE;
		goto err;
	}

	time_gate_control_list.entryID = entry_id;
	time_gate_control_list.baseTime = config->base_time;
	time_gate_control_list.cycleTime = ((uint64_t)NSECS_PER_SEC * config->cycle_time_p) / config->cycle_time_q;
	time_gate_control_list.extTime = config->cycle_time_ext;
	time_gate_control_list.numEntries = config->list_length;

	if (config->list_length) {
		gate_control_entries = rtos_malloc(config->list_length * sizeof(netc_tgs_gate_entry_t));
		if (!gate_control_entries) {
			os_log(LOG_ERR, "rtos_malloc failed\n");
			rc = -GENAVB_ERR_ST_MALLOC;
			goto err_alloc;
		}
		memset(gate_control_entries, 0, config->list_length * sizeof(netc_tgs_gate_entry_t));

		for (i = 0; i < config->list_length; i++) {
			gate_control_entries[i].interval = config->control_list[i].time_interval;
			gate_control_entries[i].tcGateState = config->control_list[i].gate_states;
			gate_control_entries[i].operType = config->control_list[i].operation;
		}
	}

	time_gate_control_list.gcList = gate_control_entries;

	if (EP_TxTGSConfigAdminGcl(&drv->handle, &time_gate_control_list) != kStatus_Success) {
		os_log(LOG_ERR, "EP_TxTGSConfigAdminGcl failed\n");
		rc = -GENAVB_ERR_ST_HW_CONFIG;
	}

	if (gate_control_entries)
		rtos_free(gate_control_entries);

err_alloc:
err:
	return rc;
}

static int enetc_ep_psi_set_st_config(struct net_port *port, struct genavb_st_config *config)
{
	int rc = GENAVB_SUCCESS;

	rc = enetc_ep_enable_st_config(port, config->enable);
	if (rc < 0) {
		os_log(LOG_ERR, "enetc_ep_enable_st_config failed\n");
		goto err;
	}

	if (config->enable) {
		rc = enetc_ep_set_admin_config(port, config);
		if (rc < 0) {
			os_log(LOG_ERR, "enetc_ep_set_admin_config failed\n");
		}
	}

err:
	return rc;
}

static int enetc_ep_vsi_set_st_config(struct net_port *port, struct genavb_st_config *config)
{
	return -GENAVB_ERR_ST_NOT_SUPPORTED;
}

static int enetc_ep_psi_get_st_config(struct net_port *port, genavb_st_config_type_t type, struct genavb_st_config *config, unsigned int list_length)
{
	struct enetc_ep_drv *drv = net_port_drv(port);
	netc_tb_tgs_gcl_t time_gate_control_list = {0};
	netc_tgs_gate_entry_t *gate_control_entries = NULL;
	status_t status;
	int rc = GENAVB_SUCCESS;
	int i;

	if (type == GENAVB_ST_ADMIN) {
		os_log(LOG_ERR, "GENAVB_ST_ADMIN not supported\n");
		rc = -GENAVB_ERR_ST_CONFIG_TYPE_NOT_SUPPORTED;
		goto err;
	}

	if (list_length > NETC_TB_TGS_MAX_ENTRY) {
		os_log(LOG_ERR, "list_length: %u invalid (max: %u)\n", list_length, NETC_TB_TGS_MAX_ENTRY);
		rc = -GENAVB_ERR_ST_INVALID_GCL_SIZE;
		goto err;
	}

	if (list_length) {
		gate_control_entries = rtos_malloc(list_length * sizeof(netc_tgs_gate_entry_t));
		if (!gate_control_entries) {
			os_log(LOG_ERR, "rtos_malloc failed\n");
			rc = -GENAVB_ERR_ST_MALLOC;
			goto err_alloc;
		}
		memset(gate_control_entries, 0, list_length * sizeof(netc_tgs_gate_entry_t));
	}

	time_gate_control_list.entryID = port->base == kNETC_ENETC0PSI0 ? kNETC_TGSEnetc0Port : kNETC_TGSEnetc1Port;
	time_gate_control_list.gcList = gate_control_entries;

	if (st_is_enabled(drv)) {
		status = EP_TxtTGSGetOperGcl(&drv->handle, &time_gate_control_list, list_length);
		if (status != kStatus_Success) {
			os_log(LOG_ERR, "EP_TxtTGSGetOperGcl failed with status %d\n", status);
			rc = -GENAVB_ERR_ST_HW_READ;
			goto exit_free;
		}

		config->enable = 1;
		config->base_time = time_gate_control_list.baseTime;
		config->cycle_time_p = time_gate_control_list.cycleTime;
		config->cycle_time_q = NSECS_PER_SEC;
		config->cycle_time_ext = time_gate_control_list.extTime;
		config->list_length = time_gate_control_list.numEntries;

		for (i = 0; i < config->list_length; i++) {
			config->control_list[i].operation = time_gate_control_list.gcList[i].operType;
			config->control_list[i].gate_states = time_gate_control_list.gcList[i].tcGateState;
			config->control_list[i].time_interval = time_gate_control_list.gcList[i].interval;
		}
	} else {
		memset(config, 0, sizeof(struct genavb_st_config));
	}

exit_free:
	if (gate_control_entries)
		rtos_free(gate_control_entries);

err_alloc:
err:
	return rc;
}

static int enetc_ep_vsi_get_st_config(struct net_port *port, genavb_st_config_type_t type, struct genavb_st_config *config, unsigned int list_length)
{
	return -GENAVB_ERR_ST_NOT_SUPPORTED;
}

static int enetc_ep_st_max_entries(struct net_port *port)
{
	if (port->base == kNETC_ENETC0PSI0 || port->base == kNETC_ENETC1PSI0)
		return NETC_TB_TGS_MAX_ENTRY;
	else
		return 0;
}

static int enetc_ep_set_fp(struct net_port *port, unsigned int type, struct genavb_fp_config *config)
{
	ep_handle_t *handle = enetc_ep_get_handle(port);
	struct enetc_ep_drv *drv = net_port_drv(port);
	int rc = 0, i;

	if (NETC_PortIsPseudo(enetc_ep_get_port_handle(port))) {
		rc = -GENAVB_ERR_FP_NOT_SUPPORTED;
		goto err;
	}

	switch (type) {
	case GENAVB_FP_CONFIG_802_1Q:
		bool enPreemption;

		for (i = 0; i < QOS_PRIORITY_MAX; i++) {
			enPreemption = config->u.cfg_802_1Q.admin_status[i] == GENAVB_FP_ADMIN_STATUS_PREEMPTABLE ? true : false;
			EP_TxTcConfigPreemption(handle, port_priority_to_traffic_class(port, i), enPreemption);
		}

		break;

	case GENAVB_FP_CONFIG_802_3:
	default:
		netc_port_preemption_config mac_config = {
			.enMergeVerify = config->u.cfg_802_3.verify_disable_tx ? false : true,
			.mergeVerifyTime = (uint8_t)config->u.cfg_802_3.verify_time,
			.preemptMode = (netc_hw_preemption_mode_t)config->u.cfg_802_3.enable_tx,
			.raf_size = (netc_hw_raf_size_t)config->u.cfg_802_3.add_frag_size,
		};

		EP_TxPortEthMacConfigPreemption(handle, &mac_config);

		fp_set_enabled(drv, (bool)mac_config.preemptMode);

		break;
	}

err:
	return rc;
}

static int enetc_ep_get_fp(struct net_port *port, unsigned int type, struct genavb_fp_config *config)
{
	ep_handle_t *handle = enetc_ep_get_handle(port);
	netc_port_preemption_config mac_config;
	int rc = 0, i;

	if (NETC_PortIsPseudo(enetc_ep_get_port_handle(port))) {
		rc = -GENAVB_ERR_FP_NOT_SUPPORTED;
		goto err;
	}

	switch (type) {
	case GENAVB_FP_CONFIG_802_1Q:
		netc_port_tg_preemption_config tg_config;
		bool enabled = false;

		for (i = 0; i < QOS_PRIORITY_MAX; i++) {
			EP_TxGetTcPreemption(handle, port_priority_to_traffic_class(port, i), &enabled);
			config->u.cfg_802_1Q.admin_status[i] = enabled ? GENAVB_FP_ADMIN_STATUS_PREEMPTABLE : GENAVB_FP_ADMIN_STATUS_EXPRESS;
		}

		NETC_PortGetTGSFPConfig(enetc_ep_get_port_handle(port), &tg_config);
		NETC_PortGetPhyMacPreemptionControl(enetc_ep_get_link_handle(port), &mac_config);

		config->u.cfg_802_1Q.preemption_active = mac_config.PreemptionActive ? true : false;
		config->u.cfg_802_1Q.hold_advance = (uint32_t)tg_config.holdAdvance;
		config->u.cfg_802_1Q.release_advance = (uint32_t)tg_config.releaseAdvance;
		config->u.cfg_802_1Q.hold_request = 0; /* There is no specific information on the hardware to fill this parameter */
		break;

	case GENAVB_FP_CONFIG_802_3:
	default:
		netc_port_phy_mac_preemption_status_t preempt_status;

		EP_TxPortGetEthMacPreemption(handle, &mac_config, &preempt_status);

		config->u.cfg_802_3.support = 1;
		config->u.cfg_802_3.status_verify = (uint32_t)preempt_status.verifyStatus;
		config->u.cfg_802_3.enable_tx = (uint32_t)mac_config.preemptMode;
		config->u.cfg_802_3.verify_disable_tx = (uint32_t)(mac_config.enMergeVerify ? false : true);
		config->u.cfg_802_3.status_tx = (uint32_t)preempt_status.mergeActive;
		config->u.cfg_802_3.verify_time = (uint32_t)mac_config.mergeVerifyTime;
		config->u.cfg_802_3.add_frag_size = (uint32_t)mac_config.raf_size;
		break;
	}

err:
	return rc;
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
	struct enetc_ep_drv *drv = net_port_drv(port);

	EP_Deinit(&drv->handle);

	port->drv = NULL;
}

__init int enetc_ep_init(struct net_port *port)
{
	struct enetc_ep_drv *drv;
	ep_config_t ep_config;
	netc_bdr_config_t buffer_config;
	netc_rx_bdr_config_t rxBdrConfig[ENETC_EP_RXRING_MAX];
	netc_tx_bdr_config_t txBdrConfig[ENETC_EP_TXRING_MAX];
	status_t result;
	uint8_t ring, index;
	uint64_t *ep_rx_buff_addr;
	rx_buffer_t *ep_rx_buff;
	netc_rx_bd_t *ep_rx_desc;
	netc_tx_bd_t *ep_tx_desc;
	netc_tx_frame_info_t *ep_tx_dirty;
	netc_si_l2mf_config_t siL2mfConfig;

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
	port->drv_ops.st_max_entries = enetc_ep_st_max_entries;
	port->drv_ops.set_fp = enetc_ep_set_fp;
	port->drv_ops.get_fp = enetc_ep_get_fp;
	port->drv_ops.exit = enetc_ep_exit;
	port->drv_ops.post_init = enetc_ep_post_init;
	port->drv_ops.pre_exit = enetc_ep_pre_exit;

	if (port->base == kNETC_ENETC0PSI0 || port->base == kNETC_ENETC1PSI0) {
		port->drv_ops.set_st_config = enetc_ep_psi_set_st_config;
		port->drv_ops.get_st_config = enetc_ep_psi_get_st_config;
	} else {
		port->drv_ops.set_st_config = enetc_ep_vsi_set_st_config;
		port->drv_ops.get_st_config = enetc_ep_vsi_get_st_config;
	}

	port->drv = &enetc_ep_drivers[port->drv_index];

	port->tx_q_cap = &tx_q_capabilites[port->drv_index];

	ep_rx_buff = NULL;

	if (port->drv_index == 0) {
		port->num_rx_q = ENETC_EP0_RXRING_NUM;
		port->num_tx_q = ENETC_EP0_TXRING_NUM;
		ep_rx_buff_addr = ep0_rx_buff_addr;
#if (!defined(BOARD_ENETC0_RX_ZERO_COPY))
		ep_rx_buff = ep0_rx_buff;
#endif
		ep_rx_desc = ep0_rx_desc;
		ep_tx_desc = ep0_tx_desc;
		ep_tx_dirty = ep0_tx_dirty;
	} else {
#if (CFG_NUM_ENETC_EP_MAC > 1)
		port->num_rx_q = ENETC_EP1_RXRING_NUM;
		port->num_tx_q = ENETC_EP1_TXRING_NUM;
		ep_rx_buff_addr = ep1_rx_buff_addr;
#if (!defined(BOARD_ENETC1_RX_ZERO_COPY))
		ep_rx_buff = ep1_rx_buff;
#endif
		ep_rx_desc = ep1_rx_desc;
		ep_tx_desc = ep1_tx_desc;
		ep_tx_dirty = ep1_tx_dirty;
#endif /* CFG_NUM_ENETC_EP_MAC > 1 */
	}

	drv = net_port_drv(port);

	memset(&rxBdrConfig, 0, sizeof(rxBdrConfig));

	for (ring = 0U; ring < port->num_rx_q; ring++) {
		rxBdrConfig[ring].bdArray = ep_rx_desc;

		if (ring == 0)
			rxBdrConfig[ring].len = ENETC_EP_RXBD0_NUM;
		else
			rxBdrConfig[ring].len = ENETC_EP_RXBDn_NUM;

		rxBdrConfig[ring].extendDescEn = true;
		rxBdrConfig[ring].buffAddrArray = ep_rx_buff_addr;
		rxBdrConfig[ring].buffSize = ENETC_EP_RXBUFF_SIZE_ALIGN;

		if (ep_rx_buff != NULL) {
			for (index = 0U; index < rxBdrConfig[ring].len / 2; index++) {
				*ep_rx_buff_addr = (uintptr_t)ep_rx_buff;
				ep_rx_buff_addr++;
				ep_rx_buff++;
			}
		} else {
			ep_rx_buff_addr += rxBdrConfig[ring].len / 2;
		}

		ep_rx_desc += rxBdrConfig[ring].len;
	}

	memset(&txBdrConfig, 0, sizeof(txBdrConfig));

	for (ring = 0U; ring < port->num_tx_q; ring++) {
		txBdrConfig[ring].bdArray  = ep_tx_desc;
		txBdrConfig[ring].priority = ring;
		txBdrConfig[ring].len = ENETC_EP_TXBD_NUM;
		txBdrConfig[ring].dirtyArray = ep_tx_dirty;

		ep_tx_desc += txBdrConfig[ring].len;
		ep_tx_dirty += txBdrConfig[ring].len;
	}

	/* Endpoint configuration. */
	EP_GetDefaultConfig(&ep_config);

#ifdef BOARD_NET_RX_CACHEABLE
	ep_config.rxCacheMaintain = true;
#endif
#ifdef BOARD_NET_TX_CACHEABLE
	ep_config.txCacheMaintain = true;
#endif

	if (!ep_rx_buff) {
		ep_config.rxZeroCopy = true;
		ep_config.rxBuffAlloc = &enetc_ep_rxbuff_alloc;
		ep_config.rxBuffFree = &enetc_ep_rxbuff_free;
		port->drv_ops.read_frame_zero_copy = &enetc_ep_read_frame_zero_copy;
	}

	ep_config.si = port->base;
	ep_config.siConfig.txRingUse = port->num_tx_q;
	ep_config.siConfig.rxRingUse = port->num_rx_q;
	ep_config.siConfig.vlanCtrl = kNETC_ENETC_StanCVlan;
	ep_config.siConfig.valnToIpvEnable = 1;
	ep_config.siConfig.rxBdrGroupNum = 1;
	ep_config.siConfig.defaultRxBdrGroup = kNETC_SiBDRGroupOne;
	ep_config.siConfig.ringPerBdrGroup = port->num_rx_q;

	for (index = 0U; index < 8U; index++)
		ep_config.siConfig.ipvToRingMap[index] = (index * ep_config.siConfig.ringPerBdrGroup) / 8;

	for (index = 0U; index < 16U; index++)
		ep_config.siConfig.vlanToIpvMap[index] = index / 2;

	ep_config.reclaimCallback = enetc_ep_reclaim_cb;
	ep_config.userData = (void *)port;

	/* Required for CBS, ST and TSD */
	ep_config.port.enableTg = true;

	for (index = 0U; index < 8U; index++)
		ep_config.txTcCfg[index].enableTsd = true;

	ep_config.port.ethMac.miiMode = port->mii_mode;
	ep_config.port.ethMac.miiSpeed  = port->phy_speed;
	ep_config.port.ethMac.miiDuplex = port->phy_duplex;

	ep_config.port.common.qosMode.vlanQosMap = 0;
	ep_config.port.common.qosMode.defaultIpv = 0;
	ep_config.port.common.qosMode.defaultDr = 2;
	ep_config.port.common.qosMode.enVlanInfo = true;
	ep_config.port.common.qosMode.vlanTagSelect = true;

#if CFG_NUM_NETC_SW
	/* Map all endpoints clocks to the bridge clock */
	clock_id_remap(port->clock[PORT_CLOCK_LOCAL], OS_CLOCK_LOCAL_BR_0);
	clock_id_remap(port->clock[PORT_CLOCK_GPTP_0], OS_CLOCK_GPTP_BR_0_0);
	clock_id_remap(port->clock[PORT_CLOCK_GPTP_1], OS_CLOCK_GPTP_BR_0_1);
#endif

	buffer_config.rxBdrConfig = rxBdrConfig;
	buffer_config.txBdrConfig = txBdrConfig;

	if (port->drv_index == 0)
		ep_config.cmdBdrConfig.bdBase = &ep0_g_cmd_buff_desc[0];
#if (CFG_NUM_ENETC_EP_MAC > 1)
	else
		ep_config.cmdBdrConfig.bdBase = &ep1_g_cmd_buff_desc[0];
#endif

	ep_config.cmdBdrConfig.bdLength = ENETC_EP_CMDBD_NUM;

	result = EP_Init(&drv->handle, port->mac_addr, &ep_config, &buffer_config);
	if (result != kStatus_Success) {
		os_log(LOG_ERR, "EP_Init returned error %d\n", result);
		goto err_ep_init;
	}

	/* Initialise L2 MAC filter for a specific SI */

#ifdef CFG_DSA_CPU_PORT_ENETC
	siL2mfConfig.macUCPromis = true;
	siL2mfConfig.macMCPromis = true;
#else
	siL2mfConfig.macUCPromis = false;
	siL2mfConfig.macMCPromis = false;
#endif
	siL2mfConfig.rejectUC = false;
	siL2mfConfig.rejectMC = false;
	siL2mfConfig.rejectBC = false;

	result = EP_RxL2MFInit(&drv->handle, &siL2mfConfig);
	if (result != kStatus_Success) {
		os_log(LOG_ERR, "EP_RxL2MFInit returned error %d\n", result);
		goto err_l2mf_init;
	}

	drv->is_pseudo = NETC_PortIsPseudo(enetc_ep_get_port_handle(port));

	if (!drv->is_pseudo)
		netc_ep_stats_init(port);
	else
		netc_ep_pseudo_stats_init(port);

	os_log(LOG_INFO, "port(%u) driver handle (%p)\n", port->index, drv);

	return 0;

err_l2mf_init:
	EP_Deinit(&drv->handle);

err_ep_init:
err_drv_index:
	return -1;
}
#else
int enetc_ep_init(struct net_port *port) { return -1; }
void *enetc_ep_get_handle(struct net_port *port) { return NULL; }
#endif /* CFG_NUM_ENETC_EP_MAC */
