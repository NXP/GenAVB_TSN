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

#include "common/types.h"
#include "net_port.h"
#include "net_port_enetc_ep.h"
#include "net_port_netc_sw.h"
#include "genavb/stats.h"

#if CFG_NUM_NETC_SW || CFG_NUM_ENETC_EP_MAC

#define NETC_PORT_RX_STATS_MAX	18
#define NETC_PORT_TX_STATS_MAX	18
#define NETC_PORT_ERR_STATS_MAX	10
#define NETC_PORT_STATS_MAX	(NETC_PORT_RX_STATS_MAX + NETC_PORT_TX_STATS_MAX + NETC_PORT_ERR_STATS_MAX)

static const char *netc_port_stats_str[NETC_PORT_STATS_MAX] = {
	"in-bytes",
	"in-valid-bytes",
	"in-pause-frames",
	"in-valid-frames",
	"in-vlan-frames",
	"in-uc-frames",
	"in-mc-frames",
	"in-bc-frames",
	"in-frames",
	"in-min-frames",
	"in-64-frames",
	"in-65-127-frames",
	"in-128-255-frames",
	"in-256-511-frames",
	"in-512-1023-frames",
	"in-1024-1522-frames",
	"in-1523-max-frames",
	"in-control-frames",

	"out-bytes",
	"out-valid-bytes",
	"out-pause-frames",
	"out-valid-frames",
	"out-vlan-frames",
	"out-uc-frames",
	"out-mc-frames",
	"out-bc-frames",
	"out-frames",
	"out-min-frames",
	"out-64-frames",
	"out-65-127-frames",
	"out-128-255-frames",
	"out-256-511-frames",
	"out-512-1023-frames",
	"out-1024-1522-frames",
	"out-1523-max-frames",
	"out-control-frames",

	"in-error",
	"in-error-undersize",
	"in-error-oversize",
	"in-error-fcs",
	"in-error-fragment",
	"in-error-jabber",
	"in-error-discard",
	"in-error-dicard-no-truncated",
	"out-error-fcs",
	"out-error-undersize"
};
#endif /* CFG_NUM_NETC_SW || CFG_NUM_ENETC_EP_MAC */

#if CFG_NUM_ENETC_EP_MAC

#include "netc_hw/fsl_netc_hw_port.h"

#define NETC_PSEUDO_PORT_RX_STATS_MAX	4
#define NETC_PSEUDO_PORT_TX_STATS_MAX	4
#define NETC_PSEUDO_PORT_STATS_MAX	(NETC_PSEUDO_PORT_RX_STATS_MAX + NETC_PSEUDO_PORT_TX_STATS_MAX)

static const char *netc_pseudo_port_stats_str[] = {
	"in-bytes",
	"in-uc-frames",
	"in-mc-frames",
	"in-bc-frames",
	"out-bytes",
	"out-uc-frames",
	"out-mc-frames",
	"out-bc-frames",
};

static int netc_ep_pseudo_stats_get_number(struct net_port *port)
{
	return NETC_PSEUDO_PORT_STATS_MAX;
}

static int netc_ep_pseudo_stats_get_strings(struct net_port *port, char *buf, unsigned int buf_len)
{
	int i;

	if (buf_len < (NETC_PSEUDO_PORT_STATS_MAX * GENAVB_PORT_STATS_STR_LEN))
		return -1;

	for (i = 0; i < NETC_PSEUDO_PORT_STATS_MAX; i++) {

		strncpy(buf, netc_pseudo_port_stats_str[i], GENAVB_PORT_STATS_STR_LEN);
		buf += GENAVB_PORT_STATS_STR_LEN;
	}

	return 0;
}

static int netc_ep_pseudo_stats_get(struct net_port *port, uint64_t *buf, unsigned int buf_len)
{
	void *handle = enetc_ep_get_pseudo_port_handle(port);

	if (buf_len < (NETC_PSEUDO_PORT_STATS_MAX * sizeof(uint64_t)))
		return -1;

	NETC_PortGetPseudoMacTrafficStatistic(handle, false, (netc_port_pseudo_mac_traffic_statistic_t *)buf);
	buf += NETC_PSEUDO_PORT_RX_STATS_MAX;

	NETC_PortGetPseudoMacTrafficStatistic(handle, true, (netc_port_pseudo_mac_traffic_statistic_t *)buf);

	return 0;
}

static int netc_ep_stats_get_number(struct net_port *port)
{
	return NETC_PORT_STATS_MAX;
}

static int netc_ep_stats_get_strings(struct net_port *port, char *buf, unsigned int buf_len)
{
	int i;

	if (buf_len < (NETC_PORT_STATS_MAX * GENAVB_PORT_STATS_STR_LEN))
		return -1;

	for (i = 0; i < NETC_PORT_STATS_MAX; i++) {

		strncpy(buf, netc_port_stats_str[i], GENAVB_PORT_STATS_STR_LEN);
		buf += GENAVB_PORT_STATS_STR_LEN;
	}

	return 0;
}

static int netc_ep_stats_get(struct net_port *port, uint64_t *buf, unsigned int buf_len)
{
	void *handle = enetc_ep_get_port_handle(port);

	if (buf_len < (NETC_PORT_STATS_MAX * sizeof(uint64_t)))
		return -1;

	NETC_PortGetPhyMacRxStatistic(handle, kNETC_ExpressMAC, (netc_port_phy_mac_traffic_statistic_t *)buf);
	buf += NETC_PORT_RX_STATS_MAX;

	NETC_PortGetPhyMacTxStatistic(handle, kNETC_ExpressMAC, (netc_port_phy_mac_traffic_statistic_t *)buf);
	buf += NETC_PORT_TX_STATS_MAX;

	NETC_PortGetPhyMacDiscardStatistic(handle, kNETC_ExpressMAC, (netc_port_phy_mac_discard_statistic_t *)buf);
	buf += NETC_PORT_ERR_STATS_MAX;

	return 0;
}

__init void netc_ep_pseudo_stats_init(struct net_port *port)
{
	port->drv_ops.stats_get_number = netc_ep_pseudo_stats_get_number;
	port->drv_ops.stats_get_strings = netc_ep_pseudo_stats_get_strings;
	port->drv_ops.stats_get = netc_ep_pseudo_stats_get;
}

__init void netc_ep_stats_init(struct net_port *port)
{
	port->drv_ops.stats_get_number = netc_ep_stats_get_number;
	port->drv_ops.stats_get_strings = netc_ep_stats_get_strings;
	port->drv_ops.stats_get = netc_ep_stats_get;
}

#endif /* CFG_NUM_ENETC_EP_MAC */

#if CFG_NUM_NETC_SW

#include "fsl_netc_switch.h"

#define NETC_SW_STATS_PER_QUEUE	7
#define NETC_SW_QUEUE_PER_PORT	8
#define NETC_QUEUE_STATS_MAX	(NETC_SW_QUEUE_PER_PORT * NETC_SW_STATS_PER_QUEUE)
#define NETC_SW_STATS_MAX	(NETC_PORT_STATS_MAX + NETC_QUEUE_STATS_MAX)

static const char *netc_queue_stats_str[] = {
	"q0-rejected-bytes",
	"q0-rejected-frames",
	"q0-dequeue-bytes",
	"q0-dequeue-frames",
	"q0-dropped-bytes",
	"q0-dropped-frames",
	"q0-frames",
	"q1-rejected-bytes",
	"q1-rejected-frames",
	"q1-dequeue-bytes",
	"q1-dequeue-frames",
	"q1-dropped-bytes",
	"q1-dropped-frames",
	"q1-frames",
	"q2-rejected-bytes",
	"q2-rejected-frames",
	"q2-dequeue-bytes",
	"q2-dequeue-frames",
	"q2-dropped-bytes",
	"q2-dropped-frames",
	"q2-frames",
	"q3-rejected-bytes",
	"q3-rejected-frames",
	"q3-dequeue-bytes",
	"q3-dequeue-frames",
	"q3-dropped-bytes",
	"q3-dropped-frames",
	"q3-frames",
	"q4-rejected-bytes",
	"q4-rejected-frames",
	"q4-dequeue-bytes",
	"q4-dequeue-frames",
	"q4-dropped-bytes",
	"q4-dropped-frames",
	"q4-frames",
	"q4-rejected-bytes",
	"q5-rejected-frames",
	"q5-dequeue-bytes",
	"q5-dequeue-frames",
	"q5-dropped-bytes",
	"q5-dropped-frames",
	"q5-frames",
	"q6-rejected-bytes",
	"q6-rejected-frames",
	"q6-dequeue-bytes",
	"q6-dequeue-frames",
	"q6-dropped-bytes",
	"q6-dropped-frames",
	"q6-frames",
	"q7-rejected-bytes",
	"q7-rejected-frames",
	"q7-dequeue-bytes",
	"q7-dequeue-frames",
	"q7-dropped-bytes",
	"q7-dropped-frames",
	"q7-frames",
};

static int netc_sw_stats_get_number(struct net_port *port)
{
	return NETC_SW_STATS_MAX;
}

static int netc_sw_stats_get_strings(struct net_port *port, char *buf, unsigned int buf_len)
{
	int i;

	if (buf_len < (NETC_SW_STATS_MAX * GENAVB_PORT_STATS_STR_LEN))
		return -1;

	for (i = 0; i < NETC_PORT_STATS_MAX; i++) {

		strncpy(buf, netc_port_stats_str[i], GENAVB_PORT_STATS_STR_LEN);
		buf += GENAVB_PORT_STATS_STR_LEN;
	}

	for (i = 0; i < NETC_QUEUE_STATS_MAX; i++) {

		strncpy(buf, netc_queue_stats_str[i], GENAVB_PORT_STATS_STR_LEN);
		buf += GENAVB_PORT_STATS_STR_LEN;
	}

	return 0;
}

static int netc_sw_stats_get(struct net_port *port, uint64_t *buf, unsigned int buf_len)
{
	void *handle = netc_sw_get_port_handle(port);
	uint32_t entry_id;
	int i;

	if (buf_len < (NETC_SW_STATS_MAX * sizeof(uint64_t)))
		return -1;

	NETC_PortGetPhyMacRxStatistic(handle, kNETC_ExpressMAC, (netc_port_phy_mac_traffic_statistic_t *)buf);
	buf += NETC_PORT_RX_STATS_MAX;

	NETC_PortGetPhyMacTxStatistic(handle, kNETC_ExpressMAC, (netc_port_phy_mac_traffic_statistic_t *)buf);
	buf += NETC_PORT_TX_STATS_MAX;

	NETC_PortGetPhyMacDiscardStatistic(handle, kNETC_ExpressMAC, (netc_port_phy_mac_discard_statistic_t *)buf);
	buf += NETC_PORT_ERR_STATS_MAX;

	for (i = 0; i < NETC_SW_QUEUE_PER_PORT; i++) {
		entry_id = (port->base << 4) | i;

		buf[NETC_SW_STATS_PER_QUEUE - 1] = 0;

		SWT_GetETMClassQueueStatistic(netc_sw_get_handle(port), entry_id, (netc_tb_etmcq_stse_t *)buf);

		buf += NETC_SW_STATS_PER_QUEUE;
	}

	return 0;
}

__init void netc_sw_stats_init(struct net_port *port)
{
	port->drv_ops.stats_get_number = netc_sw_stats_get_number;
	port->drv_ops.stats_get_strings = netc_sw_stats_get_strings;
	port->drv_ops.stats_get = netc_sw_stats_get;
}

#endif /* CFG_NUM_NETC_SW */
