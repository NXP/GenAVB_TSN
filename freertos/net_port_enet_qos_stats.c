/*
* Copyright 2020 NXP
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
#include "net_port_enet_qos.h"
#include "net_port_stats.h"
#include "genavb/stats.h"

#if NUM_ENET_QOS_MAC

#define REG_OFFSET(m) offset_of(ENET_QOS_Type, m)

typedef enum {
	EQOS_IN_IPV4_FRAMES = ETH_STD_STATS_MAX,
	EQOS_IN_IPV6_FRAMES,
	EQOS_OUT_FPE_FRAGS,
	EQOS_OUT_FPE_HOLD_REQUEST,
	EQOS_IN_FPE_ERROR_REASSEMBLY,
	EQOS_IN_FPE_ERROR_SMD,
	EQOS_IN_FPE_REASSEMBLY,
	EQOS_IN_FPE_FRAGS,
	EQOS_STATS_MAX
} eqos_stats_t;

static const char *eqos_stats_str[EQOS_STATS_MAX - ETH_STD_STATS_MAX] = {
	[EQOS_IN_IPV4_FRAMES - ETH_STD_STATS_MAX] = "eqos-in-ipv4-frames",
	[EQOS_IN_IPV6_FRAMES - ETH_STD_STATS_MAX] = "eqos-in-ipv6-frames",
	[EQOS_OUT_FPE_FRAGS - ETH_STD_STATS_MAX] = "eqos-out-fpe-frags",
	[EQOS_OUT_FPE_HOLD_REQUEST - ETH_STD_STATS_MAX] = "eqos-out-fpe-hold-request",
	[EQOS_IN_FPE_ERROR_REASSEMBLY - ETH_STD_STATS_MAX] = "eqos-in-fpe-error-reassembly",
	[EQOS_IN_FPE_ERROR_SMD - ETH_STD_STATS_MAX] = "eqos-in-fpe-error-smd",
	[EQOS_IN_FPE_REASSEMBLY - ETH_STD_STATS_MAX] = "eqos-in-fpe-reassembly",
	[EQOS_IN_FPE_FRAGS - ETH_STD_STATS_MAX] = "eqos-in-fpe-frags",
};

static const int eqos_stats[EQOS_STATS_MAX] = {
	[IEEE_OUT_FRAMES] = REG_OFFSET(MAC_TX_PACKET_COUNT_GOOD),
	[IEEE_IN_FRAMES] = -1,
	[IEEE_IN_ERROR_CRC_FRAMES] = REG_OFFSET(MAC_RX_CRC_ERROR_PACKETS),
	[IEEE_IN_ERROR_ALIGN_FRAMES] = REG_OFFSET(MAC_RX_ALIGNMENT_ERROR_PACKETS),
	[IEEE_OUT_ERROR_MAC_INTERNAL_FRAMES] = REG_OFFSET(MAC_RX_ALIGNMENT_ERROR_PACKETS),
	[IEEE_IN_ERROR_MAC_INTERNAL_FRAMES] = REG_OFFSET(MAC_RX_FIFO_OVERFLOW_PACKETS),
	[IEEE_OUT_MC_FRAMES] = REG_OFFSET(MAC_TX_MULTICAST_PACKETS_GOOD),
	[IEEE_OUT_BC_FRAMES] = REG_OFFSET(MAC_TX_BROADCAST_PACKETS_GOOD),
	[IEEE_IN_MC_FRAMES] = REG_OFFSET(MAC_RX_MULTICAST_PACKETS_GOOD),
	[IEEE_IN_BC_FRAMES] = REG_OFFSET(MAC_RX_BROADCAST_PACKETS_GOOD),
	[IEEE_OUT_PAUSE_FRAMES] = REG_OFFSET(MAC_TX_PAUSE_PACKETS),
	[IEEE_IN_PAUSE_FRAMES] = REG_OFFSET(MAC_RX_PAUSE_PACKETS),
	[IEEE_OUT_LPI_TIME] = REG_OFFSET(MAC_TX_LPI_USEC_CNTR),
	[IEEE_IN_LPI_TIME] = REG_OFFSET(MAC_RX_LPI_USEC_CNTR),
	[IEEE_OUT_LPI_TRANSITIONS] = REG_OFFSET(MAC_TX_LPI_TRAN_CNTR),
	[IEEE_IN_LPI_TRANSITIONS] = REG_OFFSET(MAC_RX_LPI_TRAN_CNTR),
	[RMON_IN_ERROR_UNDERSIZE_FRAMES] = REG_OFFSET(MAC_RX_UNDERSIZE_PACKETS_GOOD),
	[RMON_IN_ERROR_OVERSIZE_FRAMES] = REG_OFFSET(MAC_RX_OVERSIZE_PACKETS_GOOD),
	[RMON_IN_ERROR_FRAGS_FRAMES] = REG_OFFSET(MAC_RX_RUNT_ERROR_PACKETS),
	[RMON_IN_64_FRAMES] = REG_OFFSET(MAC_RX_64OCTETS_PACKETS_GOOD_BAD),
	[RMON_IN_65_127_FRAMES] = REG_OFFSET(MAC_RX_65TO127OCTETS_PACKETS_GOOD_BAD),
	[RMON_IN_128_255_FRAMES] = REG_OFFSET(MAC_RX_128TO255OCTETS_PACKETS_GOOD_BAD),
	[RMON_IN_256_511_FRAMES] = REG_OFFSET(MAC_RX_256TO511OCTETS_PACKETS_GOOD_BAD),
	[RMON_IN_512_1023_FRAMES] = REG_OFFSET(MAC_RX_512TO1023OCTETS_PACKETS_GOOD_BAD),
	[RMON_IN_1024_MAX_FRAMES] = REG_OFFSET(MAC_RX_1024TOMAXOCTETS_PACKETS_GOOD_BAD),
	[RMON_OUT_64_FRAMES] = REG_OFFSET(MAC_TX_64OCTETS_PACKETS_GOOD_BAD),
	[RMON_OUT_65_127_FRAMES] = REG_OFFSET(MAC_TX_65TO127OCTETS_PACKETS_GOOD_BAD),
	[RMON_OUT_128_255_FRAMES] = REG_OFFSET(MAC_TX_128TO255OCTETS_PACKETS_GOOD_BAD),
	[RMON_OUT_256_511_FRAMES] = REG_OFFSET(MAC_TX_256TO511OCTETS_PACKETS_GOOD_BAD),
	[RMON_OUT_512_1023_FRAMES] = REG_OFFSET(MAC_TX_512TO1023OCTETS_PACKETS_GOOD_BAD),
	[RMON_OUT_1024_MAX_FRAMES] = REG_OFFSET(MAC_TX_1024TOMAXOCTETS_PACKETS_GOOD_BAD),
	[EQOS_IN_IPV4_FRAMES] = REG_OFFSET(MAC_RXIPV4_GOOD_PACKETS),
	[EQOS_IN_IPV6_FRAMES] = REG_OFFSET(MAC_RXIPV6_GOOD_PACKETS),
	[EQOS_OUT_FPE_FRAGS] = REG_OFFSET(MAC_MMC_TX_FPE_FRAGMENT_CNTR),
	[EQOS_OUT_FPE_HOLD_REQUEST] = REG_OFFSET(MAC_MMC_TX_HOLD_REQ_CNTR),
	[EQOS_IN_FPE_ERROR_REASSEMBLY] = REG_OFFSET(MAC_MMC_RX_PACKET_ASSEMBLY_ERR_CNTR),
	[EQOS_IN_FPE_ERROR_SMD] = REG_OFFSET(MAC_MMC_RX_PACKET_SMD_ERR_CNTR),
	[EQOS_IN_FPE_REASSEMBLY] = REG_OFFSET(MAC_MMC_RX_PACKET_ASSEMBLY_OK_CNTR),
	[EQOS_IN_FPE_FRAGS] = REG_OFFSET(MAC_MMC_RX_FPE_FRAGMENT_CNTR),
};

static unsigned int num_stats;

static int enet_qos_stats_get_number(struct net_port *port)
{
	return num_stats;
}

static int enet_qos_stats_get_strings(struct net_port *port, char *buf, unsigned int buf_len)
{
	int i;
	const char *stat_str;

	if (buf_len < (num_stats * GENAVB_PORT_STATS_STR_LEN))
		return -1;

	for (i = 0; i < EQOS_STATS_MAX; i++) {
		if (eqos_stats[i] < 0)
			continue;

		if (i < ETH_STD_STATS_MAX)
			stat_str = net_port_stats_str(i);
		else
			stat_str = eqos_stats_str[i - ETH_STD_STATS_MAX];

		strncpy(buf, stat_str, GENAVB_PORT_STATS_STR_LEN);
		buf += GENAVB_PORT_STATS_STR_LEN;
	}

	return 0;
}

static int enet_qos_stats_get(struct net_port *port, uint64_t *buf, unsigned int buf_len)
{
	int i, j = 0;

	if (buf_len < (num_stats * sizeof(uint64_t)))
		return -1;

	for (i = 0; i < EQOS_STATS_MAX; i++) {
		if (eqos_stats[i] < 0)
			continue;

		buf[j++] = *(volatile uint32_t *)((uint8_t *)port->base + eqos_stats[i]);
	}

	return 0;
}

__init void enet_qos_stats_init(struct net_port *port)
{
	int i;

	port->drv_ops.stats_get_number = enet_qos_stats_get_number;
	port->drv_ops.stats_get_strings = enet_qos_stats_get_strings;
	port->drv_ops.stats_get = enet_qos_stats_get;

	num_stats = 0;

	for (i = 0; i < EQOS_STATS_MAX; i++)
		if (eqos_stats[i] >= 0)
			num_stats++;
}

#endif /* NUM_ENET_QOS_MAC */
