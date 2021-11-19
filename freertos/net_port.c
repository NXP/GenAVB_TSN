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

#include "os/net.h"
#include "os/clock.h"

#include "clock.h"
#include "net_port.h"
#include "net_port_enet.h"
#include "net_port_enet_qos.h"
#include "net_phy.h"
#include "net_rx.h"
#include "ptp.h"

#include "system_config.h"

struct net_port ports[CFG_PORTS] = {
	[0] = {
		.index = 0,
		.drv_type = BOARD_ENET0_DRV_TYPE,
		.base = BOARD_ENET0_DRV_BASE,
		.phy_index = BOARD_ENET0_PHY_INDEX,
		.mii_mode = BOARD_ENET0_MII_MODE,
		.up = false,
		.clock_local = OS_CLOCK_LOCAL_EP_0,
		.clock_gptp = OS_CLOCK_GPTP_EP_0_0,
		.timers = {
			[0] = {
				.channel = BOARD_ENET0_1588_TIMER_CHANNEL_0,
			},
			[1] = {
				.channel = BOARD_ENET0_1588_TIMER_CHANNEL_1,
			},
			[2] = {
				.channel = BOARD_ENET0_1588_TIMER_CHANNEL_2,
			},
		},
		.timer_event = {
			.channel = BOARD_ENET0_1588_TIMER_EVENT_CHANNEL,
		},
#ifdef BOARD_ENET0_1588_TIMER_PPS
		.pps_timer_channel = BOARD_ENET0_1588_TIMER_PPS,
#else
		.pps_timer_channel = -1,
#endif
	},

#if CFG_PORTS > 1
	[1] = {
		.index = 1,
		.drv_type = BOARD_ENET1_DRV_TYPE,
		.base = BOARD_ENET1_DRV_BASE,
		.phy_index = BOARD_ENET1_PHY_INDEX,
		.mii_mode = BOARD_ENET1_MII_MODE,
		.up = false,
		.clock_local = OS_CLOCK_LOCAL_EP_1,
		.clock_gptp = OS_CLOCK_GPTP_EP_1_0,
		.timers = {
			[0] = {
				.channel = BOARD_ENET1_1588_TIMER_CHANNEL_0,
			},
			[1] = {
				.channel = BOARD_ENET1_1588_TIMER_CHANNEL_1,
			},
			[2] = {
				.channel = BOARD_ENET1_1588_TIMER_CHANNEL_2,
			},
		},
		.timer_event = {
			.channel = BOARD_ENET1_1588_TIMER_EVENT_CHANNEL,
		},
#ifdef BOARD_ENET1_1588_TIMER_PPS
		.pps_timer_channel = BOARD_ENET1_1588_TIMER_PPS,
#else
		.pps_timer_channel = -1,
#endif
	},
#endif
};

static unsigned int port_tx_queue_prop_num(struct tx_queue_properties *cfg, unsigned int flag)
{
	unsigned int i, num = 0;

	for (i = 0; i < cfg->num_queues; i++)
		if (cfg->queue_prop[i] & flag)
			num++;

	return num;
}

unsigned int port_tx_queue_prop_num_cbs(struct tx_queue_properties *cfg)
{
	return port_tx_queue_prop_num(cfg, TX_QUEUE_FLAGS_CREDIT_SHAPER);
}

unsigned int port_tx_queue_prop_num_sp(struct tx_queue_properties *cfg)
{
	return port_tx_queue_prop_num(cfg, TX_QUEUE_FLAGS_STRICT_PRIORITY);
}

static int port_tx_queue_config_check(struct tx_queue_properties *cap,
				      struct tx_queue_properties *cfg)
{
	int i;

	if (cfg->num_queues > cap->num_queues)
		return -1;

	for (i = 0; i < cfg->num_queues; i++) {
		if (cfg->queue_prop[i] && !(cfg->queue_prop[i] & cap->queue_prop[i]))
			return -1;
	}

	return 0;
}

int port_set_tx_queue_config(struct net_port *port, struct tx_queue_properties *cfg)
{
	if (port_tx_queue_config_check(port->tx_q_cap, cfg) < 0) {
		os_log(LOG_ERR, "port(%u) invalid config\n", port->index);
		goto err;
	}

	if (port->drv_ops.set_tx_queue_config(port, cfg) < 0) {
		os_log(LOG_ERR, "port(%u) port_set_tx_queue_config() error \n", port->index);
		goto err;
	}

	port->num_tx_q = cfg->num_queues;

	return 0;

err:
	return -1;
}

int port_set_tx_idle_slope(struct net_port *port, unsigned int idle_slope, unsigned int queue)
{
	if (port->drv_ops.set_tx_idle_slope(port, idle_slope, queue) < 0) {
		os_log(LOG_ERR, "port(%u) set_tx_idle_slope() error\n", port->index);
		goto err;
	}

	return 0;

err:
	return -1;
}

uint8_t *port_get_hwaddr(struct net_port *port)
{
	return port->mac_addr;
}

int port_status(struct net_port *port, struct net_port_status *status)
{
	if (port->up) {
		status->up = true;
		phy_port_status(port, status);
	} else {
		status->up = false;
		status->full_duplex = false;
		status->rate = 0;
	}

	return 0;
}

int port_add_multi(struct net_port *port, uint8_t *addr)
{
	return port->drv_ops.add_multi(port, addr);
}

int port_del_multi(struct net_port *port, uint8_t *addr)
{
	return port->drv_ops.del_multi(port, addr);
}

static inline void ptp_hdr_to_data(struct ptp_hdr *ptp, struct ptp_frame_data *ptp_data)
{
	if (ptp) {
		ptp_data->version = ptp->version_ptp;
		memcpy(&ptp_data->src_port_id, &ptp->source_port_id, sizeof(struct ptp_port_identity));
		ptp_data->sequence_id = ntohs(ptp->sequence_id);
		ptp_data->message_type = ptp->msg_type;
	} else {
		ptp_data->version = 0;
		memset(&ptp_data->src_port_id, 0, sizeof(struct ptp_port_identity));
		ptp_data->sequence_id = 0;
		ptp_data->message_type = 0;
	}
}

unsigned int port_rx(struct net_rx_ctx *net, struct net_port *port, unsigned int n, unsigned int queue)
{
	uint32_t length;
	uint64_t cycles;
	struct net_rx_desc *desc;
	unsigned int read = 0;
	int rc;

	if (!port->up)
		return 0;

	while (read < n) {
		rc = port->drv_ops.get_rx_frame_size(port, &length, queue);
		if (rc == 1) {

			desc = net_rx_alloc(length);
			if (!desc) {
				port->stats[queue].rx_alloc_err++;
				break;

			}
			desc->len = length;

			if (!port->drv_ops.read_frame(port, (uint8_t *)desc + desc->l2_offset,
						      length, &cycles, queue)) {
				desc->ts64 = hw_clock_cycles_to_time(port->hw_clock, cycles) -
					     port->rx_tstamp_latency;
				desc->ts = (uint32_t)desc->ts64;
				port->stats[queue].rx++;
				eth_rx(net, desc, port);
			} else {
				port->stats[queue].rx_err++;
				net_rx_free(desc);
			}

		} else if (rc < 0) {
			port->stats[queue].rx_err++;
			port->drv_ops.read_frame(port, NULL, 0, NULL, queue);
		} else {
			break;
		}

		read++;

		xSemaphoreGive(net->mutex);
		/* Explicit preemption point, to allow socket synchronous receive */
		xSemaphoreTake(net->mutex, portMAX_DELAY);
	}

	return read;
}

void port_tx_cleanup(struct net_port *port)
{
	port->drv_ops.tx_cleanup(port);
}

static inline void port_tx_lock(struct net_port *port, unsigned int queue)
{
	xSemaphoreTake(port->mutex[queue], portMAX_DELAY);
}

static inline void port_tx_unlock(struct net_port *port, unsigned int queue)
{
	xSemaphoreGive(port->mutex[queue]);
}

static void port_tx_lock_all(struct net_port *port)
{
	int i;

	for (i = 0; i < port->num_tx_q; i++)
		port_tx_lock(port, i);
}

static void port_tx_unlock_all(struct net_port *port)
{
	int i;

	for (i = port->num_tx_q - 1; i >= 0; i--)
		port_tx_unlock(port, i);
}


int port_tx(struct net_port *port, struct net_tx_desc *desc, unsigned int queue)
{
	int rc;

	port_tx_lock(port, queue);

	if (port->up) {

		port->stats[queue].tx++;
		rc = port->drv_ops.send_frame(port, (uint8_t *)desc + desc->l2_offset,
					      desc->len, desc, queue, desc->flags & NET_TX_FLAGS_HW_TS);
		if (rc < 0) {
			port->stats[queue].tx_err++;
			rc = -1;
		}
	} else {
		rc = -1;
	}

	port_tx_unlock(port, queue);

	/* Free descriptor now only if buffer is copied and successfully transmitted */
	if (!rc)
		net_tx_free(desc);

	return rc;
}

void port_tx_up(struct net_port *port)
{
	port->tx_up = true;

	port_qos_up(port);

	xEventGroupSetBits(port->event_group_handle, PORT_TX_SUCCESS);
}

void port_tx_down(struct net_port *port)
{
	port->tx_up = false;

	port_qos_down(port);

	xEventGroupSetBits(port->event_group_handle, PORT_TX_SUCCESS);
}

void port_up(struct net_port *port)
{
	port_tx_lock_all(port);

	if (port->up)
		goto out;

	port->drv_ops.link_up(port);

	port->up = true;

	phy_set_ts_latency(port);

out:
	port_tx_unlock_all(port);

	os_log(LOG_INFO, "port(%u) up, speed:%x, duplex:%x\n", port->index, port->phy_speed, port->phy_duplex);

	xEventGroupSetBits(port->event_group_handle, PORT_SUCCESS);
}

int port_set_st_config(struct net_port *port, struct genavb_st_config *config)
{
	int rc;

	xSemaphoreTake(port->config_mutex, portMAX_DELAY);

	if (port->drv_ops.set_st_config)
		rc = port->drv_ops.set_st_config(port, config);
	else
		rc = -1;

	xSemaphoreGive(port->config_mutex);

	return rc;
}

int port_get_st_config(struct net_port *port, genavb_st_config_type_t type,
		       struct genavb_st_config *config, unsigned int list_length)
{
	int rc;

	xSemaphoreTake(port->config_mutex, portMAX_DELAY);

	if (port->drv_ops.get_st_config)
		rc = port->drv_ops.get_st_config(port, type, config, list_length);
	else
		rc = -1;

	xSemaphoreGive(port->config_mutex);

	return rc;
}

int port_set_fp(struct net_port *port, unsigned int type, struct genavb_fp_config *config)
{
	int rc;

	xSemaphoreTake(port->config_mutex, portMAX_DELAY);

	if (port->drv_ops.set_fp)
		rc = port->drv_ops.set_fp(port, type, config);
	else
		rc = -1;

	xSemaphoreGive(port->config_mutex);

	return rc;
}

int port_get_fp(struct net_port *port, unsigned int type, struct genavb_fp_config *config)
{
	int rc;

	xSemaphoreTake(port->config_mutex, portMAX_DELAY);

	if (port->drv_ops.get_fp)
		rc = port->drv_ops.get_fp(port, type, config);
	else
		rc = -1;

	xSemaphoreGive(port->config_mutex);

	return rc;
}

int port_stats_get_number(struct net_port *port)
{
	return port->drv_ops.stats_get_number(port);
}

int port_stats_get_strings(struct net_port *port, char *buf, unsigned int buf_len)
{
	return port->drv_ops.stats_get_strings(port, buf, buf_len);
}

int port_stats_get(struct net_port *port, uint64_t *buf, unsigned int buf_len)
{
	return port->drv_ops.stats_get(port, buf, buf_len);
}

void port_down(struct net_port *port)
{
	port_tx_lock_all(port);

	if (!port->up)
		goto out;

	port->drv_ops.link_down(port);

	port->up = false;

out:
	port_tx_unlock_all(port);

	os_log(LOG_INFO, "port(%u) down\n", port->index);

	xEventGroupSetBits(port->event_group_handle, PORT_SUCCESS);
}

__init static int __port_post_init(struct net_port *port)
{
	if (port->drv_ops.post_init)
		port->drv_ops.post_init(port);

	phy_init(port);

	/* Actual MAC setup is done on phy up (polled from net rx task) */

	return 0;
}
__exit static void __port_pre_exit(struct net_port *port)
{
	phy_exit(port);

	if (port->drv_ops.pre_exit)
		port->drv_ops.pre_exit(port);
}

__init int port_post_init(void)
{
	int i, j;

	for (i = 0; i < CFG_PORTS; i++)
		if (__port_post_init(&ports[i]) < 0) {
			os_log(LOG_ERR, "__port_post_init() port(%u) failed\n",
			       ports[i].index);
			goto err;
		}

	return 0;

err:
	for (j = 0; j < i; j++)
		__port_pre_exit(&ports[j]);

	return -1;
}

__exit void port_pre_exit(void)
{
	int i;

	for (i = 0; i < CFG_PORTS; i++)
		__port_pre_exit(&ports[i]);
}

__init static int __port_init(struct net_port *port)
{
	const struct net_config *net_cfg;
	int i;

	os_log(LOG_INIT, "port(%u): %p\n", port->index, port);

	switch (port->drv_type) {
	case ENET_t:
	case ENET_1G_t:
		port->drv_ops.init = enet_init;
		break;

	case ENET_QOS_t:
		port->drv_ops.init = enet_qos_init;
		break;

	default:
		os_log(LOG_ERR, "port(%u) invalid port type: %u\n", port->index, port->drv_type);
		goto err_type;
		break;
	}

	net_cfg = system_config_get_net(port->index);
	if (!net_cfg) {
		os_log(LOG_ERR, "system_config_get_net() port(%u) failed\n", port->index);
		goto err_config;
	}

	memset(&port->stats, 0, sizeof(port->stats));

	port->config_mutex = xSemaphoreCreateMutexStatic(&port->config_mutex_buffer);
	if (!port->config_mutex) {
		os_log(LOG_ERR, "xSemaphoreCreateMutexStatic() port(%u) failed\n", port->index);
		goto err_semaphore_create;
	}

	for (i = 0; i < MAX_QUEUES; i++) {
		port->mutex[i] = xSemaphoreCreateMutexStatic(&port->mutex_buffer[i]);
		if (!port->mutex[i]) {
			os_log(LOG_ERR, "xSemaphoreCreateMutexStatic() port(%u) failed\n", port->index);
			goto err_semaphore_create;
		}
	}

	port->event_group_handle = xEventGroupCreateStatic(&port->event_group);
	if (!port->event_group_handle) {
		os_log(LOG_ERR, "xEventGroupCreateStatic() port(%u) failed\n", port->index);
		goto err_group_create;
	}

	memcpy(port->mac_addr, net_cfg->hw_addr, 6);

	if (port->drv_ops.init(port) < 0) {
		os_log(LOG_ERR, "port drv_ops->init() port(%u) failed\n", port->index);
		goto err_drv_init;
	}

	port->hw_clock = hw_clock_get(clock_to_hw_clock(port->clock_local));
	if (!port->hw_clock) {
		os_log(LOG_ERR, "hw_clock_get() port(%u) failed\n", port->index);
		goto err_clock;
	}

	return 0;

err_clock:
	port->drv_ops.exit(port);

err_drv_init:
err_group_create:
err_semaphore_create:
err_config:
err_type:
	return -1;
}

__exit static void __port_exit(struct net_port *port)
{
	port->drv_ops.exit(port);
}

__init int port_init(void)
{
	int i, j;

	for (i = 0; i < CFG_PORTS; i++)
		if (__port_init(&ports[i]) < 0) {
			os_log(LOG_ERR, "__port_init() port(%u) failed\n",
			       ports[i].index);
			goto err;
		}

	return 0;

err:
	for (j = 0; j < i; j++)
		__port_exit(&ports[j]);

	return -1;
}

__exit void port_exit(void)
{
	int i;

	for (i = 0; i < CFG_PORTS; i++)
		__port_exit(&ports[i]);
}
