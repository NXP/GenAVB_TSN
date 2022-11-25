/*
* Copyright 2014 Freescale Semiconductor, Inc.
* Copyright 2020-2021 NXP
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
 @brief Linux specific Network service implementation
 @details
*/


#define _GNU_SOURCE

#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/ethtool.h>
#include <linux/if_ether.h>
#include <linux/net_tstamp.h>
#include <linux/sockios.h>

#include "genavb/qos.h"
#include "genavb/sr_class.h"
#include "genavb/helpers.h"
#include "os/string.h"
#include "os/assert.h"
#include "common/log.h"
#include "common/net.h"
#include "epoll.h"

#include "net.h"

#include "net_logical_port.h"

__attribute__((weak)) int net_avb_init(struct net_ops_cb *net_ops) { return -1; };
__attribute__((weak)) int net_std_init(struct net_ops_cb *net_ops) { return -1; };
__attribute__((weak)) int net_xdp_init(struct net_ops_cb *net_ops, struct os_xdp_config *xdp_config) { return -1; };
static struct net_ops_cb net_ops;

static int socket_fd = -1;

struct net_rx_desc *__net_rx(struct net_rx *rx)
{
	return net_ops.__net_rx(rx);
}

void net_rx(struct net_rx *rx)
{
	return net_ops.net_rx(rx);
}

void net_rx_multi(struct net_rx *rx)
{
	return net_ops.net_rx_multi(rx);
}

int net_rx_init(struct net_rx *rx, struct net_address *addr, void (*func)(struct net_rx *, struct net_rx_desc *), unsigned long epoll_fd)
{
	return net_ops.net_rx_init(rx, addr, func, epoll_fd);
}

int net_rx_init_multi(struct net_rx *rx, struct net_address *addr, void (*func)(struct net_rx *, struct net_rx_desc **, unsigned int), unsigned int packets, unsigned int time, unsigned long epoll_fd)
{
	return net_ops.net_rx_init_multi(rx, addr, func, packets, time, epoll_fd);
}

void net_rx_exit(struct net_rx *rx)
{
	return net_ops.net_rx_exit(rx);
}

int net_tx_init(struct net_tx *tx, struct net_address *addr)
{
	return net_ops.net_tx_init(tx, addr);
}

void net_tx_exit(struct net_tx *tx)
{
	net_ops.net_tx_exit(tx);
}

int net_tx(struct net_tx *tx, struct net_tx_desc *desc)
{
	return net_ops.net_tx(tx, desc);
}

int net_tx_multi(struct net_tx *tx, struct net_tx_desc **desc, unsigned int n)
{
	return net_ops.net_tx_multi(tx, desc, n);
}

void net_tx_ts_process(struct net_tx *tx)
{
	uint64_t ts;
	unsigned int private;
	int rc;

	while ((rc = net_ops.net_tx_ts_get(tx, &ts, &private)) > 0)
		tx->func_tx_ts(tx, ts, private);
}

struct net_tx_desc *net_tx_alloc(unsigned int size)
{
	return net_ops.net_tx_alloc(size);
}

int net_tx_alloc_multi(struct net_tx_desc **desc, unsigned int n, unsigned int size)
{
	return net_ops.net_tx_alloc_multi(desc, n, size);
}

struct net_tx_desc *net_tx_clone(struct net_tx_desc *src)
{
	return net_ops.net_tx_clone(src);
}

void net_tx_free(struct net_tx_desc *buf)
{
	return net_ops.net_tx_free(buf);
}

void net_rx_free(struct net_rx_desc *buf)
{
	return net_ops.net_rx_free(buf);
}

void net_free_multi(void **buf, unsigned int n){
	return net_ops.net_free_multi(buf, n);
}

int net_set_hw_ts(unsigned int port_id, bool enable)
{
	struct ifreq hwtstamp;
	struct hwtstamp_config config;

	if (!logical_port_valid(port_id))
		return -1;

	/* set hw timestamping on the interface */
	memset(&hwtstamp, 0, sizeof(hwtstamp));
	strncpy(hwtstamp.ifr_name, logical_port_name(port_id), sizeof(hwtstamp.ifr_name) - 1);
	memset(&config, 0, sizeof(config));
	hwtstamp.ifr_data = (void *)&config;


	config.tx_type = (enable) ? HWTSTAMP_TX_ON : HWTSTAMP_TX_OFF;

	if (enable) {
		config.rx_filter = logical_port_is_bridge(port_id) ? HWTSTAMP_FILTER_PTP_V2_EVENT: HWTSTAMP_FILTER_ALL;
	} else {
		config.rx_filter = HWTSTAMP_FILTER_NONE;
	}

	if (ioctl(socket_fd, SIOCSHWTSTAMP, &hwtstamp) < 0) {
		os_log(LOG_ERR, "ioctl(SIOCSHWTSTAMP) %s, failed %s timestamping on %s\n", strerror(errno), (enable)?"enabling":"disabling", logical_port_name(port_id));
		goto err_ioctl;
	}

	os_log(LOG_INFO, "Configured HW timestamping to tx_type(%d) rx_filter(%d) on %s\n", config.tx_type, config.rx_filter, logical_port_name(port_id));

	return 0;

err_ioctl:
	return -1;
}

int net_tx_ts_init(struct net_tx *tx, struct net_address *addr, void (*func)(struct net_tx *, uint64_t, unsigned int), unsigned long priv)
{
	return net_ops.net_tx_ts_init(tx, addr, func, priv);
}

int net_tx_ts_exit(struct net_tx *tx)
{
	return net_ops.net_tx_ts_exit(tx);
}

int net_get_local_addr(unsigned int port_id, unsigned char *addr)
{
	struct ifreq buf;

	if (!logical_port_valid(port_id)) {
		os_log(LOG_ERR, "logical_port(%u) invalid\n", port_id);
		goto err_portid;
	}

	memset(&buf, 0, sizeof(buf));

	h_strncpy(buf.ifr_name, logical_port_name(port_id), sizeof(buf.ifr_name));

	if (ioctl(socket_fd, SIOCGIFHWADDR, &buf) < 0) {
		os_log(LOG_ERR, "ioctl(SIOCGIFHWADDR) failed: %s\n", strerror(errno));
		goto err_ioctl;
	}

	os_memcpy(addr, buf.ifr_hwaddr.sa_data, ETH_ALEN);

	os_log(LOG_DEBUG,"%s %02x:%02x:%02x:%02x:%02x:%02x\n", logical_port_name(port_id),
		addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);

	return 0;

err_ioctl:
err_portid:
	return -1;
}

int net_std_port_status(unsigned int port_id, bool *up, bool *point_to_point, unsigned int *rate)
{
	int rc = -1;
	struct ifreq if_req;
	struct ethtool_cmd eth_cmd;

	os_assert((up != NULL) && (point_to_point != NULL) && (rate != NULL));

	if_req.ifr_ifindex = if_nametoindex(logical_port_name(port_id));
	if (!if_req.ifr_ifindex) {
		os_log(LOG_ERR, "if_nametoindex error failed: %s %s\n", strerror(errno), logical_port_name(port_id));
		goto err_itf;
	}

	h_strncpy(if_req.ifr_name, logical_port_name(port_id), IF_NAMESIZE);

	if (ioctl(socket_fd, SIOCGIFFLAGS, &if_req) < 0) {
		os_log(LOG_ERR, "ioctl(SIOCGIFFLAGS) failed: %s %s\n", strerror(errno), logical_port_name(port_id));
		goto err_ioctl;
	}

	/* up: true if port is up, false otherwise */
	*up = (if_req.ifr_flags & IFF_RUNNING) ? 1 : 0;

	eth_cmd.cmd = ETHTOOL_GSET;
	if_req.ifr_data = (void *)&eth_cmd;

	if (ioctl(socket_fd, SIOCETHTOOL, &if_req) < 0) {
		os_log(LOG_ERR, "ioctl(SIOCETHTOOL) failed: %s\n", strerror(errno));
		goto err_ioctl;
	}

	/* rate:  port rate in bits per second (valid only if up is true) */
	*rate = ethtool_cmd_speed(&eth_cmd);

	/* point_to_point: true if port is full duplex (valid only if up is true) */
	*point_to_point = eth_cmd.duplex;

	rc = 0;

err_ioctl:
err_itf:
	return rc;
}

int net_dflt_port_status(struct net_tx *tx, unsigned int port_id, bool *up, bool *point_to_point, unsigned int *rate)
{
	return net_std_port_status(port_id, up, point_to_point, rate);
}

int net_port_status(struct net_tx *tx, unsigned int port_id, bool *up, bool *point_to_point, unsigned int *rate)
{
	if (!logical_port_valid(port_id))
		return -1;

	return net_ops.net_port_status(tx, port_id, up, point_to_point, rate);
}

static int __net_std_multi(unsigned int port, unsigned long request, const unsigned char *hw_addr)
{
	struct ifreq ifr = {0};

	memcpy(ifr.ifr_name, logical_port_name(port), IFNAMSIZ);
	memcpy(ifr.ifr_hwaddr.sa_data, hw_addr, 6);

	if (ioctl(socket_fd, request, (char *)&ifr) < 0) {
		os_log(LOG_ERR, "ioctl failed: %s\n", strerror(errno));
		return -1;
	}

	return 0;
}

int net_std_add_multi(struct net_rx *rx, unsigned int port_id, const unsigned char *hw_addr)
{
	if (!logical_port_valid(port_id)) {
		os_log(LOG_ERR, "logical_port(%u) invalid\n", port_id);
		return -1;
	}

	os_log(LOG_INFO, "%s %02x:%02x:%02x:%02x:%02x:%02x\n", logical_port_name(port_id),
		hw_addr[0], hw_addr[1], hw_addr[2], hw_addr[3], hw_addr[4], hw_addr[5]);

	return __net_std_multi(port_id, SIOCADDMULTI, hw_addr);
}

int net_std_del_multi(struct net_rx *rx, unsigned int port_id, const unsigned char *hw_addr)
{
	if (!logical_port_valid(port_id)) {
		os_log(LOG_ERR, "logical_port(%u) invalid\n", port_id);
		return -1;
	}

	os_log(LOG_INFO,"%s %02x:%02x:%02x:%02x:%02x:%02x\n", logical_port_name(port_id),
		hw_addr[0], hw_addr[1], hw_addr[2], hw_addr[3], hw_addr[4], hw_addr[5]);

	return __net_std_multi(port_id, SIOCDELMULTI, hw_addr);
}


int net_add_multi(struct net_rx *rx, unsigned int port_id, const unsigned char *hw_addr)
{
	return net_ops.net_add_multi(rx, port_id, hw_addr);
}

int net_del_multi(struct net_rx *rx, unsigned int port_id, const unsigned char *hw_addr)
{
	return net_ops.net_del_multi(rx, port_id, hw_addr);
}

void net_std_rx_parser(struct net_rx *rx, struct net_rx_desc *desc)
{
	struct eth_hdr *ethhdr = (struct eth_hdr *)NET_DATA_START(desc);
	uint16_t ether_type = ethhdr->type;

	if (ether_type == htons(ETHERTYPE_VLAN)) {
		struct vlanhdr *vlan = (void *)(ethhdr + 1);
		desc->ethertype = ntohs(vlan->type);
		desc->l3_offset = desc->l2_offset + sizeof(struct eth_hdr) + sizeof(struct vlanhdr);
		desc->vid = VLAN_VID(vlan);
	} else {
		desc->ethertype = ntohs(ether_type);
		desc->l3_offset = desc->l2_offset + sizeof(struct eth_hdr);
		desc->vid = 0;
	}

	desc->l4_offset = 0;
	desc->l5_offset = 0;
	desc->flags = 0;
	desc->priv = 0;
}

int net_tx_event_enable(struct net_tx *tx, unsigned long priv)
{
	os_log(LOG_DEBUG, "tx(%p) priv %lu\n", tx, priv);

	tx->epoll_fd = priv;

	if (epoll_ctl_add(tx->epoll_fd, tx->fd, EPOLL_TYPE_NET_TX_EVENT, tx, &tx->epoll_data, EPOLLOUT) < 0) {
		os_log(LOG_ERR, "tx(%p) epoll_ctl_add() failed\n", tx);
		goto err_epoll_ctl;
	}

	return 0;

err_epoll_ctl:
	return -1;
}

int net_tx_event_disable(struct net_tx *tx)
{
	os_log(LOG_DEBUG, "tx(%p) epoll_fd %d\n", tx, tx->epoll_fd);

	if (epoll_ctl_del(tx->epoll_fd, tx->fd) < 0) {
		os_log(LOG_ERR, "tx(%p) epoll_ctl_del() failed\n", tx);
		goto err_epoll_ctl;
	}

	return 0;

err_epoll_ctl:
	return -1;
}

unsigned int net_tx_available(struct net_tx *tx)
{
	return net_ops.net_tx_available(tx);
}

int net_port_sr_config(unsigned int port_id, uint8_t *sr_class)
{
	return net_ops.net_port_sr_config(port_id, sr_class);
}

uint8_t net_port_priority_to_traffic_class(unsigned int port_id, uint8_t priority)
{
	const u8 *map;

	if (!logical_port_valid(port_id))
		return 0;

	if (priority > QOS_PRIORITY_MAX)
		return 0;

	/* FIXME retrieve more detailed information from the port */
	if (logical_port_is_bridge(port_id))
		map = priority_to_traffic_class_map(QOS_TRAFFIC_CLASS_MAX, QOS_SR_CLASS_MAX);
	else
		map = priority_to_traffic_class_map(CFG_TRAFFIC_CLASS_MAX, CFG_SR_CLASS_MAX);

	if (!map)
		return 0;

	return map[priority];
}

int net_init(struct os_net_config *config, struct os_xdp_config *xdp_config)
{
	socket_fd = socket(PF_INET, SOCK_DGRAM, 0);
	if (socket_fd < 0) {
		os_log(LOG_ERR, "socket() failed: %s\n", strerror(errno));
		goto err_sock;
	}

	switch (config->net_mode) {
	case NET_AVB:
		if (net_avb_init(&net_ops) < 0) {
			os_log(LOG_ERR, "Could not initialize AVB network service implementation\n");
			goto err;
		}
		break;
	case NET_STD:
		if (net_std_init(&net_ops) < 0) {
			os_log(LOG_ERR, "Could not initialize STD network service implementation\n");
			goto err;
		}
		break;
	case NET_XDP:
		if (net_xdp_init(&net_ops, xdp_config) < 0) {
			os_log(LOG_ERR, "Could not initialize XDP network service implementation\n");
			goto err;
		}
		break;
	default:
		goto err;
	}

	os_log(LOG_INIT, "done\n");

	return 0;

err:
	close(socket_fd);

	socket_fd = -1;
err_sock:
	return -1;
}

void net_exit(void)
{
	net_ops.net_exit();

	close(socket_fd);

	socket_fd = -1;
}
