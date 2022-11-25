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

#include "net_port_netc_sw.h"

#include "config.h"

#if CFG_NUM_NETC_SW

#include "fsl_netc.h"
#include "fsl_netc_switch.h"
#include "fsl_netc_timer.h"
#include "fsl_phy.h"

#include "net_port_enetc_ep.h"
#include "net_port_netc_1588.h"

#include "clock.h"
#include "hw_clock.h"
#include "ether.h"
#include "ptp.h"


#define NETC_SW_BD_ALIGN 128U

/* Tx settings */

#define NETC_SW_TXBD_NUM 16

SDK_ALIGN(static netc_tx_bd_t sw_tx_desc[NETC_SW_TXBD_NUM], NETC_SW_BD_ALIGN);
static netc_tx_frame_info_t sw_tx_dirty[NETC_SW_TXBD_NUM];

/* Rx settings */

#define NETC_SW_RXBD_NUM 16 /* ENETC rx bd queue size must be a power of 2 */
#define NETC_SW_BUFF_SIZE_ALIGN 64U
#define NETC_SW_RXBUFF_SIZE NET_DATA_SIZE
#define NETC_SW_RXBUFF_SIZE_ALIGN SDK_SIZEALIGN(NETC_SW_RXBUFF_SIZE, NETC_SW_BUFF_SIZE_ALIGN)

typedef uint8_t rx_buffer_t[NETC_SW_RXBUFF_SIZE_ALIGN];

SDK_ALIGN(static netc_rx_bd_t sw_rx_desc[NETC_SW_RXBD_NUM], NETC_SW_BD_ALIGN);
SDK_ALIGN(static rx_buffer_t sw_rx_buff[NETC_SW_RXBD_NUM/2], NETC_SW_BUFF_SIZE_ALIGN);
static uint64_t sw_rx_buff_addr[NETC_SW_RXBD_NUM/2];

/* Command settings */

#define NETC_SW_CMDBD_NUM 8

SDK_ALIGN(static netc_cmd_bd_t g_cmd_buff_desc[NETC_SW_CMDBD_NUM], NETC_SW_BD_ALIGN);


#define NUM_TX_TS 32

#define TX_TS_ID_MAX_AGE 32
#define TX_TS_ID_FREE		(1 << 0)
#define TX_TS_ID_PENDING 	(1 << 1)
#define TX_TS_ID_AGED 		(1 << 2)

struct tx_ts_info_t {
	uint8_t state;
	uint16_t age;
	uint16_t tx_ts_id;
	uint32_t timestamp;
	struct net_tx_desc *desc;
	struct net_port *port;
};

#define NUM_IPF_ENTRIES 2
#define NUM_TC		8	/* Number of traffic classes, priorities and buffer pools */
#define HOST_PORT_ID 	(BOARD_NUM_NETC_PORTS - 1)

struct netc_sw_drv {
	NETC_ENETC_Type *base;
	u8 refcount;
	bool use_masquerade;
	u8 st_enabled;
	swt_handle_t handle;
	swt_config_t config;
	swt_transfer_config_t tx_rx_config;
	ep_handle_t *ep_handle;
	struct hw_clock clock;
	os_clock_id_t clock_local;
	void *timer_handle;
	struct tx_ts_info_t tx_ts[NUM_TX_TS];
	uint32_t ipf_entries_id[NUM_IPF_ENTRIES];
	uint16_t num_ipf_entries;
	uint16_t tc_words_log2[NUM_TC];
	uint8_t word_size;
	uint8_t ports;	/* total enabled ports */
};

void netc_sw_stats_init(struct net_port *);

static struct netc_sw_drv netc_sw_drivers[CFG_NUM_NETC_SW] = {
	[0] = {
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
};

#define get_drv(sw) ((struct netc_sw_drv *)sw->drv)

static inline void st_set(struct netc_sw_drv *drv, unsigned int port_id, bool enable)
{
	if (enable)
		drv->st_enabled |= (1U << port_id);
	else
		drv->st_enabled &= ~(1U << port_id);
}

static inline bool st_is_enabled(struct netc_sw_drv *drv, unsigned int port_id)
{
	return (drv->st_enabled & (1U << port_id));
}

static inline bool st_is_same(struct netc_sw_drv *drv, unsigned int port_id, bool enable)
{
	return (enable && st_is_enabled(drv, port_id)) || (!enable && !st_is_enabled(drv, port_id));
}

void *netc_sw_get_handle(struct net_port *port)
{
	struct netc_sw_drv *drv = get_drv(port);

	return &drv->handle;
}

void *netc_sw_get_port_handle(struct net_port *port)
{
	struct netc_sw_drv *drv = get_drv(port);

	return drv->handle.hw.ports[port->base].eth;
}

static int netc_sw_add_multi(struct net_port *port, uint8_t *addr)
{
	return 0;
}

static int netc_sw_del_multi(struct net_port *port, uint8_t *addr)
{
	return 0;
}

static void netc_sw_ts_free(struct tx_ts_info_t *ts_info)
{
	ts_info->state = TX_TS_ID_FREE;
	ts_info->age = 0;

	if (ts_info->desc) {
		net_tx_free(ts_info->desc);
		ts_info->desc = NULL;
	}
}

static void netc_sw_ts_flush(struct netc_sw_drv *drv)
{
	int i;

	for (i = 0; i < NUM_TX_TS; i++)
		netc_sw_ts_free(&drv->tx_ts[i]);
}

static struct tx_ts_info_t *netc_sw_ts_alloc(struct netc_sw_drv *drv, uint16_t tx_ts_id)
{
	struct tx_ts_info_t *ts_info, *oldest = NULL;
	struct tx_ts_info_t *found = NULL;
	int i, oldest_slot = 0;

	for (i = 0; i < NUM_TX_TS; i++) {
		ts_info = &drv->tx_ts[i];

		if ((!found) && (ts_info->state == TX_TS_ID_FREE)) {
			found = ts_info;
		} else {
			if ((ts_info->state == TX_TS_ID_PENDING) && (++ts_info->age >= TX_TS_ID_MAX_AGE)) {
				os_log(LOG_ERR, "slot(%u) aged ts_id(%u)\n", i, ts_info->tx_ts_id);
				ts_info->state = TX_TS_ID_AGED;
			}

			if ((!oldest) || (oldest->age < ts_info->age)) {
				oldest = ts_info;
				oldest_slot = i;
			}
		}
	}

	if (!found) {
		os_log(LOG_ERR, "tx_ts array is full ts_id(%u), replacing oldest slot(%u) ts_id(%u)\n", tx_ts_id, oldest_slot, oldest->tx_ts_id);

		netc_sw_ts_free(oldest);

		found = oldest;
	}

	found->state = TX_TS_ID_PENDING;
	found->tx_ts_id = tx_ts_id;

	return found;
}

static struct tx_ts_info_t *netc_sw_ts_find(struct netc_sw_drv *drv, uint16_t tx_ts_id)
{
	struct tx_ts_info_t *ts_info;
	int i;

	for (i = 0; i < NUM_TX_TS; i++) {
		ts_info = &drv->tx_ts[i];
		if ((ts_info->state != TX_TS_ID_FREE) && (ts_info->tx_ts_id == tx_ts_id))  {
			return ts_info;
		}
	}

	return NULL;
}

static uint8_t netc_sw_port_to_net_port[CFG_BR_DEFAULT_NUM_PORTS] = CFG_BR_LOGICAL_PORT_LIST;

static void netc_sw_ts_done(struct netc_sw_drv *drv, struct tx_ts_info_t *ts_info)
{
	struct net_port *port = ts_info->port;
	uint64_t cycles = 0, ts;

	cycles = netc_1588_hwts_to_u64(drv->timer_handle, ts_info->timestamp);

	ts = hw_clock_cycles_to_time(port->hw_clock, cycles) + port->tx_tstamp_latency;

	ptp_tx_ts(port, ts, ts_info->desc->priv);

	netc_sw_ts_free(ts_info);
}

static status_t netc_sw_reclaim_cb(swt_handle_t *handle, netc_tx_frame_info_t *frameInfo, void *userData)
{
	struct net_port *ports_array = (struct net_port *)userData;
	struct net_tx_desc *desc = (struct net_tx_desc *)frameInfo->context;
	struct net_port *port = &ports_array[netc_sw_port_to_net_port[desc->port]];
	struct netc_sw_drv *drv = get_drv(port);
	struct tx_ts_info_t *tx_ts_info;

	if (frameInfo->isTxTsIdAvail) {
		tx_ts_info = netc_sw_ts_find(drv, frameInfo->txtsid);
		if (tx_ts_info) {
			tx_ts_info->port = port;
			tx_ts_info->desc = desc;

			netc_sw_ts_done(drv, tx_ts_info);
		} else {
			tx_ts_info = netc_sw_ts_alloc(drv, frameInfo->txtsid);

			tx_ts_info->port = port;
			tx_ts_info->desc = desc;
		}
	} else {
		net_tx_free(desc);
	}

	return kStatus_Success;
}

static int netc_sw_get_rx_frame_size(struct net_bridge *sw, uint32_t *length)
{
	struct netc_sw_drv *drv = get_drv(sw);
	int rc;

	switch (SWT_GetRxFrameSize(&drv->handle, length)) {
	case kStatus_Success:
	case kStatus_NETC_RxHRZeroFrame:
		rc = BR_RX_FRAME_SUCCESS;
		break;

	case kStatus_NETC_RxTsrResp:
		rc = BR_RX_FRAME_EGRESS_TS;
		break;

	case kStatus_NETC_RxFrameEmpty:
		rc = BR_RX_FRAME_EMPTY;
		break;

	default:
		rc = BR_RX_FRAME_ERROR;
		break;
	}

	return rc;
}

static int netc_sw_read_frame(struct net_bridge *sw, uint8_t *data, uint32_t length, uint8_t *port_index, uint64_t *ts)
{
	struct netc_sw_drv *drv = get_drv(sw);
	netc_frame_attr_t attr;

	if (SWT_ReceiveFrameCopy(&drv->handle, data, length, &attr) ==  kStatus_Success) {
		if (port_index)
			*port_index = netc_sw_port_to_net_port[attr.srcPort];

		if (ts && attr.isTsAvail) {
			*ts = netc_1588_hwts_to_u64(drv->timer_handle, attr.timestamp);
			os_log(LOG_DEBUG, "port(%u) attr.timestamp(%u) ts(%lu)\n", netc_sw_port_to_net_port[attr.srcPort], attr.timestamp, *ts);
		}
	} else {
		goto err;
	}

	return 0;

err:
	return -1;
}

static int netc_sw_read_egress_ts_frame(struct net_bridge *sw)
{
	struct netc_sw_drv *drv = get_drv(sw);
	struct tx_ts_info_t *tx_ts_info;
	swt_tsr_resp_t tsr;

	if (SWT_GetTimestampRefResp(&drv->handle, &tsr) == kStatus_Success) {
		tx_ts_info = netc_sw_ts_find(drv, tsr.txtsid);
		if (tx_ts_info) {
			tx_ts_info->timestamp = tsr.timestamp;

			netc_sw_ts_done(drv, tx_ts_info);
		} else {
			tx_ts_info = netc_sw_ts_alloc(drv, tsr.txtsid);

			tx_ts_info->timestamp = tsr.timestamp;
		}
	} else {
		goto err;
	}

	return 0;

err:
	return -1;
}

static const uint8_t port_to_netc_sw_port[CFG_BR_DEFAULT_NUM_PORTS] = {kNETC_SWITCH0Port0, kNETC_SWITCH0Port1, kNETC_SWITCH0Port2, kNETC_SWITCH0Port3, kNETC_SWITCH0Port4};

static int netc_sw_send_frame(struct net_bridge *sw, unsigned int port, uint8_t *data, uint32_t length, struct net_tx_desc *desc, uint8_t priority, bool need_ts)
{
	netc_buffer_struct_t tx_buff = {.buffer = data, .length = desc->len};
	netc_frame_struct_t tx_frame = {.buffArray = &tx_buff, .length = 1};
	swt_mgmt_tx_arg_t mgmt_tx = {.ipv = priority, .dr = 0};
	struct netc_sw_drv *drv = get_drv(sw);
	swt_tx_opt opt;

	opt.flags = (need_ts)? kSWT_TX_OPT_DIRECT_ENQUEUE_REQ_TSR: 0;

	desc->port = port;

	if (SWT_SendFrame(&drv->handle, mgmt_tx, port_to_netc_sw_port[port], drv->use_masquerade, &tx_frame, (void *)desc, &opt) == kStatus_Success) {
		return 1;
	} else {
		return -1;
	}
}

static void netc_sw_tx_cleanup(struct net_bridge *sw)
{
	struct netc_sw_drv *drv = get_drv(sw);

	SWT_ReclaimTxDescriptor(&drv->handle, false, 0);
}

static void netc_sw_link_up(struct net_port *port)
{
	struct netc_sw_drv *drv = get_drv(port);
	uint16_t speed;

	/* Use the actual speed and duplex when phy success
	 * to finish the autonegotiation.
	 */
	switch (port->phy_speed) {
	case kPHY_Speed10M:
	default:
		drv->config.ports[port->base].ethMac.miiSpeed = kNETC_MiiSpeed10M;
		speed = 0;
		break;

	case kPHY_Speed100M:
		drv->config.ports[port->base].ethMac.miiSpeed = kNETC_MiiSpeed100M;
		speed = 9;
		break;

	case kPHY_Speed1000M:
		drv->config.ports[port->base].ethMac.miiSpeed = kNETC_MiiSpeed1000M;
		speed = 99;
		break;
	}

	switch (port->phy_duplex) {
	case kPHY_HalfDuplex:
	default:
		drv->config.ports[port->base].ethMac.miiDuplex = kNETC_MiiHalfDuplex;
		break;

	case kPHY_FullDuplex:
		drv->config.ports[port->base].ethMac.miiDuplex = kNETC_MiiFullDuplex;
		break;
	}

	SWT_SetEthPortMII(&drv->handle, port->base, drv->config.ports[port->base].ethMac.miiSpeed, drv->config.ports[port->base].ethMac.miiDuplex);
	SWT_SetPortSpeed(&drv->handle, port->base, speed);
}

static void netc_sw_link_down(struct net_port *port)
{
	struct netc_sw_drv *drv = get_drv(port);

	netc_sw_ts_flush(drv);
}

static int netc_sw_set_tx_queue_config(struct net_port *port, struct tx_queue_properties *tx_q_cfg)
{
	return 0;
}

static int netc_sw_set_tx_idle_slope(struct net_port *port, unsigned int idle_slope, uint32_t queue)
{
	return 0;
}

static int netc_sw_enable_st_config(struct net_port *port, bool enable)
{
	struct netc_sw_drv *drv = get_drv(port);
	netc_hw_port_idx_t port_idx = port->base;
	status_t status;
	int rc = 0;

	if (st_is_same(drv, port_idx, enable))
		goto out;

#if (defined(FSL_FEATURE_NETC_HAS_ERRATA_051130) && FSL_FEATURE_NETC_HAS_ERRATA_051130)
	status = SWT_TxPortTGSEnable(&drv->handle, port_idx, enable, 0xff);
#else
	status = SWT_TxPortTGSEnable(&drv->handle, port_idx, enable);
#endif
	if (status != kStatus_Success) {
		os_log(LOG_ERR, "SWT_TxPortTGSEnable failed\n");
		rc = -1;
		goto err;
	}

	st_set(drv, port_idx, enable);

err:
out:
	return rc;
}

static int netc_sw_set_admin_config(struct net_port *port, struct genavb_st_config *config)
{
	struct netc_sw_drv *drv = get_drv(port);
	netc_tb_tgs_gcl_t time_gate_control_list;
	netc_tgs_gate_entry_t *gate_control_entries = NULL;
	netc_tb_tgs_entry_id_t entry_id = port->base;
	int rc = 0;
	int i;

	if (config->list_length > NETC_TB_TGS_MAX_ENTRY) {
		os_log(LOG_ERR, "list_length: %u invalid (max: %u)\n", config->list_length, NETC_TB_TGS_MAX_ENTRY);
		rc = -1;
		goto err;
	}

	time_gate_control_list.entryID = entry_id;
	time_gate_control_list.baseTime = config->base_time;
	time_gate_control_list.cycleTime = ((uint64_t)NSECS_PER_SEC * config->cycle_time_p) / config->cycle_time_q;
	time_gate_control_list.extTime = config->cycle_time_ext;
	time_gate_control_list.numEntries = config->list_length;

	if (config->list_length) {
		gate_control_entries = pvPortMalloc(config->list_length * sizeof(netc_tgs_gate_entry_t));
		if (!gate_control_entries) {
			os_log(LOG_ERR, "pvPortMalloc failed\n");
			rc = -1;
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

	if (SWT_TxTGSConfigAdminGcl(&drv->handle, &time_gate_control_list) != kStatus_Success) {
		os_log(LOG_ERR, "SWT_TxTGSConfigAdminGcl failed\n");
		rc = -1;
	}

	if (gate_control_entries)
		vPortFree(gate_control_entries);

err_alloc:
err:
	return rc;
}

static int netc_sw_set_st_config(struct net_port *port, struct genavb_st_config *config)
{
	int rc = 0;

	if (netc_sw_enable_st_config(port, config->enable) < 0) {
		os_log(LOG_ERR, "netc_sw_enable_st_config failed\n");
		rc = -1;
		goto err;
	}

	if (config->enable) {
		if (netc_sw_set_admin_config(port, config) < 0) {
			os_log(LOG_ERR, "netc_sw_set_admin_config failed\n");
			rc = -1;
			goto err;
		}
	}

err:
	return rc;
}

static int netc_sw_get_st_config(struct net_port *port, genavb_st_config_type_t type, struct genavb_st_config *config, unsigned int list_length)
{
	struct netc_sw_drv *drv = get_drv(port);
	netc_tb_tgs_gcl_t time_gate_control_list = {0};
	netc_tgs_gate_entry_t *gate_control_entries = NULL;
	status_t status;
	int rc = 0;
	int i;

	if (!config) {
		rc = -1;
		goto err;
	}

	if (type == GENAVB_ST_ADMIN) {
		os_log(LOG_ERR, "GENAVB_ST_ADMIN not supported\n");
		rc = -1;
		goto err;
	}

	if (list_length > NETC_TB_TGS_MAX_ENTRY) {
		os_log(LOG_ERR, "list_length: %u invalid (max: %u)\n", list_length, NETC_TB_TGS_MAX_ENTRY);
		rc = -1;
		goto err;
	}

	if (list_length) {
		gate_control_entries = pvPortMalloc(list_length * sizeof(netc_tgs_gate_entry_t));
		if (!gate_control_entries) {
			os_log(LOG_ERR, "pvPortMalloc failed\n");
			rc = -1;
			goto err_alloc;
		}
		memset(gate_control_entries, 0, list_length * sizeof(netc_tgs_gate_entry_t));
	}

	time_gate_control_list.entryID = port->base;
	time_gate_control_list.gcList = gate_control_entries;


	if (st_is_enabled(drv, port->base)) {
		status = SWT_TxtTGSGetOperGcl(&drv->handle, &time_gate_control_list, list_length);
		if (status != kStatus_Success) {
			os_log(LOG_ERR, "SWT_TxtTGSGetOperGcl failed with status %d\n", status);
			rc = -1;
			goto exit_free;
		}

		config->enable = 1;
		config->base_time = time_gate_control_list.baseTime;
		config->cycle_time_p = time_gate_control_list.cycleTime;
		config->cycle_time_q = NSECS_PER_SEC;
		config->cycle_time_ext = time_gate_control_list.extTime;
		config->list_length = time_gate_control_list.numEntries;

		if (config->list_length > list_length) {
			rc = -1;
			goto exit_free;
		}

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
		vPortFree(gate_control_entries);

err_alloc:
err:
	return rc;
}

static int netc_sw_set_fp(struct net_port *port, unsigned int type, struct genavb_fp_config *config)
{
	return 0;
}

static int netc_sw_get_fp(struct net_port *port, unsigned int type, struct genavb_fp_config *config)
{
	return 0;
}

static int netc_sw_stats_get_number(struct net_port *port)
{
	return 0;
}

static int netc_sw_stats_get_strings(struct net_port *port, char *buf, unsigned int buf_len)
{
	return 0;
}

static int netc_sw_stats_get(struct net_port *port, uint64_t *buf, unsigned int buf_len)
{
	return 0;
}

static void netc_sw_del_ingress_filtering(struct netc_sw_drv *drv)
{
	while (drv->num_ipf_entries) {
		SWT_RxIPFDelTableEntry(&drv->handle, drv->ipf_entries_id[drv->num_ipf_entries]);
		drv->num_ipf_entries--;
	}
}

static int netc_sw_set_ingress_filtering(struct netc_sw_drv *drv)
{
	static netc_tb_ipf_config_t ipfEntryCfg;
	uint8_t vlan_reserved_mac[6] = C_VLAN_RESERVED_BASE;
	uint8_t vlan_reserved_mask[6] = C_VLAN_RESERVED_MASK;
	uint8_t mmrp_mvrp_mac[6] = MMRP_MVRP_BASE;
	uint8_t mmrp_mvrp_mask[6] = MMRP_MVRP_MASK;

	memset(&ipfEntryCfg, 0U, sizeof(netc_tb_ipf_config_t));
	ipfEntryCfg.cfge.fltfa = kNETC_IPFRedirectToMgmtPort;
	ipfEntryCfg.cfge.hr = kNETC_SoftwareDefHR0;

	drv->num_ipf_entries = 0;

	/* IEEE 802.1Q-2018 - Table 10.1 - MRP application addresses */
	memcpy(ipfEntryCfg.keye.dmac, mmrp_mvrp_mac, 6);
	memcpy(ipfEntryCfg.keye.dmacMask, mmrp_mvrp_mask, 6);

	if (SWT_RxIPFAddTableEntry(&drv->handle, &ipfEntryCfg, &drv->ipf_entries_id[drv->num_ipf_entries]) != kStatus_Success)
		goto err_ipf;

	drv->num_ipf_entries++;

	/*  IEEE 802.1Q-2018 - Table 8.1 - C-VLAN and MAC Bridge addresses */
	memcpy(ipfEntryCfg.keye.dmac, vlan_reserved_mac, 6);
	memcpy(ipfEntryCfg.keye.dmacMask, vlan_reserved_mask, 6);

	if (SWT_RxIPFAddTableEntry(&drv->handle, &ipfEntryCfg, &drv->ipf_entries_id[drv->num_ipf_entries]) != kStatus_Success)
		goto err_ipf;

	drv->num_ipf_entries++;

	return 0;

err_ipf:
	return -1;
}

static int netc_sw_post_init(struct net_port *port)
{
	return 0;
}

static void netc_sw_pre_exit(struct net_port *port)
{

}

__exit static void netc_sw_port_exit(struct net_port *port)
{

}

__init int netc_sw_port_init(struct net_port *port)
{
	if (port->drv_index >= CFG_NUM_NETC_SW)
		return -1;

	if (port->base >= CFG_NUM_NETC_SW_PORTS)
		return -1;

	port->drv_ops.add_multi = netc_sw_add_multi;
	port->drv_ops.del_multi = netc_sw_del_multi;
	port->drv_ops.link_up = netc_sw_link_up;
	port->drv_ops.link_down = netc_sw_link_down;
	port->drv_ops.set_tx_queue_config = netc_sw_set_tx_queue_config;
	port->drv_ops.set_tx_idle_slope = netc_sw_set_tx_idle_slope;
	port->drv_ops.set_st_config = netc_sw_set_st_config;
	port->drv_ops.get_st_config = netc_sw_get_st_config;
	port->drv_ops.set_fp = netc_sw_set_fp;
	port->drv_ops.get_fp = netc_sw_get_fp;
	port->drv_ops.stats_get_number = netc_sw_stats_get_number;
	port->drv_ops.stats_get_strings = netc_sw_stats_get_strings;
	port->drv_ops.stats_get = netc_sw_stats_get;
	port->drv_ops.exit = netc_sw_port_exit;
	port->drv_ops.post_init = netc_sw_post_init;
	port->drv_ops.pre_exit = netc_sw_pre_exit;

	port->drv = &netc_sw_drivers[port->drv_index];
	port->hw_clock = &netc_sw_drivers[port->drv_index].clock;

	port->num_tx_q = 1;
	port->num_rx_q = 1;

	netc_sw_stats_init(port);

	os_log(LOG_INFO, "port(%u) driver handle(%p)\n", port->index, port->drv);

	return 0;
}

__exit static void netc_sw_exit(struct net_bridge *sw)
{
	struct netc_sw_drv *drv = get_drv(sw);

	netc_sw_ts_flush(drv);

	hw_clock_unregister(clock_to_hw_clock(drv->clock_local));

	netc_sw_del_ingress_filtering(drv);

	SWT_Deinit(&drv->handle);

	sw->drv = NULL;
}

__init static void host_port_init(swt_config_t *config, unsigned int bridge_port_id)
{
	config->ports[bridge_port_id].enTxRx = true;

	config->ports[bridge_port_id].commonCfg.qosMode.vlanQosMap = 0;
	config->ports[bridge_port_id].commonCfg.qosMode.defaultIpv = 0;
	config->ports[bridge_port_id].commonCfg.qosMode.defaultDr = 2;
	config->ports[bridge_port_id].commonCfg.qosMode.enVlanInfo = true;
	config->ports[bridge_port_id].commonCfg.qosMode.vlanTagSelect = true;

	/* Map each priority to a dedicated buffer pool */
	config->ports[bridge_port_id].ipvToBP[0] = 0;
	config->ports[bridge_port_id].ipvToBP[1] = 1;
	config->ports[bridge_port_id].ipvToBP[2] = 2;
	config->ports[bridge_port_id].ipvToBP[3] = 3;
	config->ports[bridge_port_id].ipvToBP[4] = 4;
	config->ports[bridge_port_id].ipvToBP[5] = 5;
	config->ports[bridge_port_id].ipvToBP[6] = 6;
	config->ports[bridge_port_id].ipvToBP[7] = 7;

	config->ports[bridge_port_id].bridgeCfg.isRxVlanAware = false;
}

__init static void external_port_init(struct net_port *port, swt_config_t *config, unsigned int bridge_port_id)
{
	config->ports[bridge_port_id].enTxRx = true;

	config->ports[bridge_port_id].ethMac.miiMode = port->mii_mode;
	config->ports[bridge_port_id].ethMac.miiSpeed = kNETC_MiiSpeed100M;
	config->ports[bridge_port_id].ethMac.miiDuplex = kNETC_MiiFullDuplex;

	config->ports[bridge_port_id].commonCfg.qosMode.vlanQosMap = 0;
	config->ports[bridge_port_id].commonCfg.qosMode.defaultIpv = 0;
	config->ports[bridge_port_id].commonCfg.qosMode.defaultDr = 2;
	config->ports[bridge_port_id].commonCfg.qosMode.enVlanInfo = true;
	config->ports[bridge_port_id].commonCfg.qosMode.vlanTagSelect = true;

	/* Map each priority to a dedicated buffer pool */
	config->ports[bridge_port_id].ipvToBP[0] = 0;
	config->ports[bridge_port_id].ipvToBP[1] = 1;
	config->ports[bridge_port_id].ipvToBP[2] = 2;
	config->ports[bridge_port_id].ipvToBP[3] = 3;
	config->ports[bridge_port_id].ipvToBP[4] = 4;
	config->ports[bridge_port_id].ipvToBP[5] = 5;
	config->ports[bridge_port_id].ipvToBP[6] = 6;
	config->ports[bridge_port_id].ipvToBP[7] = 7;

	config->ports[bridge_port_id].commonCfg.ipfCfg.enIPFTable = true;

	config->ports[bridge_port_id].bridgeCfg.isRxVlanAware = false;
}

__init static void etm_congestion_group_init(struct netc_sw_drv *drv, unsigned int port_id)
{
	netc_tb_etmcg_config_t config;
	int i;

	/* ETM Congestion group table */
	/* One entry per port and group */
	/* Same threshold for all DR */
	/* Never drop frames for DR=0 */
	config.cfge.tdDr0En = 0;
	config.cfge.tdDr1En = 1;
	config.cfge.tdDr2En = 1;
	config.cfge.tdDr3En = 1;
	config.cfge.oal = 0;
	config.cfge.tdDRThresh[0].ta = drv->word_size;
	config.cfge.tdDRThresh[1].ta = drv->word_size;
	config.cfge.tdDRThresh[2].ta = drv->word_size;
	config.cfge.tdDRThresh[3].ta = drv->word_size;

	for (i = 0; i < NUM_TC; i++) {
		config.entryID = (port_id << 4) | i;

		config.cfge.tdDRThresh[0].tn = drv->tc_words_log2[i];
		config.cfge.tdDRThresh[1].tn = drv->tc_words_log2[i];
		config.cfge.tdDRThresh[2].tn = drv->tc_words_log2[i];
		config.cfge.tdDRThresh[3].tn = drv->tc_words_log2[i];

		if (SWT_TxETMConfigCongestionGroup(&drv->handle, &config) != kStatus_Success)
			os_log(LOG_ERR, "SWT_TxETMConfigCongestionGroup(%u, %u) failed\n",
			       port_id, i);
		else
			os_log(LOG_INIT, "port(%u), queue(%u), words: %u\n",
			port_id, i, 1 << drv->tc_words_log2[i]);
	}
}

__init static void etm_class_init(struct netc_sw_drv *drv, unsigned int port_id)
{
	/* ETM Class queue table */
	/* One entry per port and queue */
	/* By default each queue is associated to a separate congestion group */
	/* All ports are configured the same */
}

__init static void buffer_pool_init(struct netc_sw_drv *drv, unsigned int pool_id)
{
	netc_tb_bp_config_t config;

	config.entryID = pool_id;
	config.cfge.sbpEn = false;
	config.cfge.gcCfg = kNETC_FlowCtrlDisable;

	config.cfge.maxThresh = NETC_TB_BP_THRESH(drv->ports, drv->tc_words_log2[pool_id]);

	config.cfge.fcOnThresh = NETC_TB_BP_THRESH(0, 0);
	config.cfge.fcOffThresh = NETC_TB_BP_THRESH(0, 0);
	config.cfge.sbpThresh = NETC_TB_BP_THRESH(0, 0);
	config.cfge.sbpEid = 0;
	config.cfge.fcPorts = 0;

	if (SWT_UpdateBPTableEntry(&drv->handle, &config) != kStatus_Success)
		os_log(LOG_ERR, "SWT_UpdateBPTableEntry(%u) failed\n", pool_id);
	else
		os_log(LOG_INIT, "buffer_pool(%u), words: %u\n",
			pool_id, drv->ports * (1 << drv->tc_words_log2[pool_id]));
}

__init static unsigned int log_2(unsigned int val)
{
	unsigned int i = 0;

	while ((1 << i) <= val) i++;

	return i - 1;
}

__init static void buffering_init(struct netc_sw_drv *drv)
{
	unsigned int total_bytes;
	unsigned int words;
	int i;

	words = (drv->handle.hw.base->SMBCAPR & NETC_SW_SMBCAPR_NUM_WORDS_MASK) >> NETC_SW_SMBCAPR_NUM_WORDS_SHIFT;

	switch ((drv->handle.hw.base->SMBCAPR & NETC_SW_SMBCAPR_WORD_SIZE_MASK) >> NETC_SW_SMBCAPR_WORD_SIZE_SHIFT) {
	case 0:
	default:
		drv->word_size = 24;
		break;
	}

	total_bytes = words * drv->word_size;

	os_log(LOG_INIT, "words: %u, size: %u, total: %u KiB\n",
	       words, drv->word_size, total_bytes / 1024);

	/*
	 * Words per traffic class
	 * 50% for tc 0 and 1
	 * 50% for tc 2, 3, 4, 5, 6 and 7
	 */
	drv->tc_words_log2[0] = log_2(words / 4 / drv->ports);
	drv->tc_words_log2[1] = log_2(words / 4 / drv->ports);
	drv->tc_words_log2[2] = log_2(words / 12 / drv->ports);
	drv->tc_words_log2[3] = log_2(words / 12 / drv->ports);
	drv->tc_words_log2[4] = log_2(words / 12 / drv->ports);
	drv->tc_words_log2[5] = log_2(words / 12 / drv->ports);
	drv->tc_words_log2[6] = log_2(words / 12 / drv->ports);
	drv->tc_words_log2[7] = log_2(words / 12 / drv->ports);

	/* Initialize buffer pools */
	for (i = 0; i < NUM_TC; i++)
		buffer_pool_init(drv, i);

	for (i = 0; i < BOARD_NUM_NETC_PORTS; i++) {
		if (!drv->config.ports[i].enTxRx)
			continue;

		/* Initialize traffic classes */
		etm_class_init(drv, i);

		/* Initialize queue sizes */
		etm_congestion_group_init(drv, i);
	}
}

__init int netc_sw_init(struct net_bridge *sw)
{
	struct netc_sw_drv *drv;
	struct net_port *ep_port = NULL;
	struct net_port *sw_port = NULL;
	unsigned int net_port_idx, bridge_port_id;

	if (sw->drv_index >= CFG_NUM_NETC_SW)
		goto err_drv_index;

	sw->drv_ops.get_rx_frame_size = netc_sw_get_rx_frame_size;
	sw->drv_ops.read_frame = netc_sw_read_frame;
	sw->drv_ops.read_egress_ts_frame = netc_sw_read_egress_ts_frame;
	sw->drv_ops.send_frame = netc_sw_send_frame;
	sw->drv_ops.tx_cleanup = netc_sw_tx_cleanup;
	sw->drv_ops.exit = netc_sw_exit;

	sw->drv = &netc_sw_drivers[sw->drv_index];

	drv = get_drv(sw);

	drv->ports = 0;
	drv->use_masquerade = false;
	drv->st_enabled = 0;

	netc_sw_ts_flush(drv);

	/* Endpoint PSI may already be initialized through the EP driver.
	Parse the whole ports table and find the EP pseudo entry and at
	least on switch port */
	for (net_port_idx = 0; net_port_idx < CFG_PORTS; net_port_idx ++) {
		if ((!ep_port) && (ports[net_port_idx].drv_type == ENETC_PSEUDO_1G_t)) {
			ep_port = &ports[net_port_idx];
			continue;
		}

		if ((!sw_port) && (ports[net_port_idx].drv_type == NETC_SW_t)) {
			sw_port = &ports[net_port_idx];
			continue;
		}
	}

	/* No speudo MAC endpoint or switch port found in the configuration, the switch cannot be initialized */
	if ((ep_port == NULL) || (sw_port == NULL))
		goto err_port_config;

	drv->ep_handle = (ep_handle_t *)enetc_ep_get_handle(ep_port);

	/* Switch configuration. */
	SWT_GetDefaultConfig(&drv->config);

	/* Disable all ports by default */
	for (bridge_port_id = 0; bridge_port_id < BOARD_NUM_NETC_PORTS; bridge_port_id++)
		drv->config.ports[bridge_port_id].enTxRx = false;

	/* Enable ports found in the configuration */
	for (net_port_idx = 0; net_port_idx < CFG_PORTS; net_port_idx++) {
		struct net_port *port = &ports[net_port_idx];

		if (port->drv_type == NETC_SW_t) {
			external_port_init(port, &drv->config, port->base);
			drv->ports++;
		}
	}

	host_port_init(&drv->config, HOST_PORT_ID);
	drv->ports++;

	drv->timer_handle = netc_1588_init();
	if (!drv->timer_handle)
		goto err_1588_init;

	drv->clock.priv = drv->timer_handle;
	drv->clock.read_counter = netc_1588_read_counter;
	drv->clock.adj_freq = netc_1588_clock_adj_freq;

	/* perform hw clock registration only once for the whole bridge */
	drv->clock_local = sw_port->clock_local;
	if (hw_clock_register(clock_to_hw_clock(drv->clock_local), &drv->clock) < 0) {
		os_log(LOG_ERR, "failed to register hw_clock(%d), bridge(%p)\n", clock_to_hw_clock(sw_port->clock_local), sw->index);
		goto err_clock_register;
	}

	drv->config.cmdRingUse = 1U;
	drv->config.cmdBdrCfg[0].bdBase = &g_cmd_buff_desc[0];
	drv->config.cmdBdrCfg[0].bdLength = NETC_SW_CMDBD_NUM;

	if (SWT_Init(&drv->handle, &drv->config) != kStatus_Success)
		goto err_swt_init;

	buffering_init(drv);

	for (uint8_t index = 0U; index < NETC_SW_RXBD_NUM/2; index++)
		sw_rx_buff_addr[index] = (uintptr_t)&sw_rx_buff[index];

	drv->tx_rx_config.rxZeroCopy = false;
	drv->tx_rx_config.reclaimCallback = netc_sw_reclaim_cb;
	drv->tx_rx_config.userData = (void *)ports;
	drv->tx_rx_config.enUseMgmtRxBdRing = true;
	drv->tx_rx_config.mgmtRxBdrConfig.bdArray = &sw_rx_desc[0];
	drv->tx_rx_config.mgmtRxBdrConfig.len = NETC_SW_RXBD_NUM;
	drv->tx_rx_config.mgmtRxBdrConfig.extendDescEn = true;
	drv->tx_rx_config.mgmtRxBdrConfig.buffAddrArray = &sw_rx_buff_addr[0];
	drv->tx_rx_config.mgmtRxBdrConfig.buffSize = NETC_SW_RXBUFF_SIZE_ALIGN;

	drv->tx_rx_config.enUseMgmtTxBdRing = true;
	drv->tx_rx_config.mgmtTxBdrConfig.bdArray = &sw_tx_desc[0],
	drv->tx_rx_config.mgmtTxBdrConfig.priority = 2;
	drv->tx_rx_config.mgmtTxBdrConfig.len = NETC_SW_TXBD_NUM;
	drv->tx_rx_config.mgmtTxBdrConfig.dirtyArray = &sw_tx_dirty[0];

	/* Config switch transfer resource */
	if (SWT_ManagementTxRxConfig(&drv->handle, drv->ep_handle, &drv->tx_rx_config) != kStatus_Success)
		goto err_mgmt_config;

	if (netc_sw_set_ingress_filtering(drv) < 0) {
		os_log(LOG_ERR, "failed to set ingress filtering\n");
		goto err_ipf;
	}

	os_log(LOG_INFO, "driver handle(%p)\n", drv);

	return 0;

err_ipf:
err_mgmt_config:
	SWT_Deinit(&drv->handle);

err_swt_init:
	hw_clock_unregister(clock_to_hw_clock(drv->clock_local));

err_clock_register:
	netc_1588_exit();

err_1588_init:
err_port_config:
err_drv_index:
	return -1;
}
#else
__init int netc_sw_init(struct net_bridge *sw) { return -1; }
__init int netc_sw_port_init(struct net_port *port) { return -1; }
#endif /* CFG_NUM_NETC_SW */

