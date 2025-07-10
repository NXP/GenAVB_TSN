/*
* Copyright 2018, 2020-2021, 2023-2025 NXP
*
* SPDX-License-Identifier: BSD-3-Clause
*/

/**
 \file socket.c
 \brief control public API
 \details
*/

#include "api/socket.h"

#if defined(CONFIG_SOCKET)
#include "genavb/error.h"
#include "os/stdlib.h"

int genavb_socket_rx_open(struct genavb_socket_rx **sock, genavb_sock_f_t flags,
			  struct genavb_socket_rx_params *params)
{
	int rc = 0;

	if (!sock) {
		rc = -GENAVB_ERR_INVALID;
		goto out;
	}


	if (!socket_rx_flags_ok(flags)) {
		rc = -GENAVB_ERR_SOCKET_PARAMS;
		goto out;
	}

	*sock = os_malloc(sizeof(struct genavb_socket_rx));
	if (!*sock) {
		rc = -GENAVB_ERR_NO_MEMORY;
		goto out_set_null;
	}

	os_memset(*sock, 0, sizeof(struct genavb_socket_rx));

	os_memcpy(&(*sock)->params, params, sizeof(struct genavb_socket_rx_params));
	(*sock)->flags = flags;

	if (params->addr.port >= CFG_MAX_LOGICAL_PORTS) {
		rc = -GENAVB_ERR_SOCKET_PARAMS;
		goto out_free_socket;
	}

	if (socket_rx_event_init(*sock) < 0) {
		rc = -GENAVB_ERR_SOCKET_INIT;
		goto out_free_socket;
	}

	if (net_rx_init(&(*sock)->net, &params->addr, NULL, (*sock)->priv) < 0) {
		rc = -GENAVB_ERR_SOCKET_INIT;
		goto out_event_exit;
	}

	if (MAC_IS_MCAST(params->addr.u.l2.dst_mac)) {
		if (net_add_multi(&(*sock)->net, params->addr.port,
				  params->addr.u.l2.dst_mac) < 0)
			goto out_rx_exit;
	}

	return GENAVB_SUCCESS;

out_rx_exit:
	net_rx_exit(&(*sock)->net);

out_event_exit:
	socket_rx_event_exit(*sock);

out_free_socket:
	os_free(*sock);

out_set_null:
	*sock = NULL;

out:
	return rc;
}

int genavb_socket_tx_open(struct genavb_socket_tx **sock, genavb_sock_f_t flags,
			  struct genavb_socket_tx_params *params)
{
	int rc;
	struct net_address *addr;

	if (!sock) {
		rc = -GENAVB_ERR_INVALID;
		goto out;
	}

	if (flags & GENAVB_SOCKF_NONBLOCK) {
		rc = -GENAVB_ERR_SOCKET_PARAMS;
		goto out;
	}

	if (!socket_tx_flags_ok(flags)) {
		rc = -GENAVB_ERR_SOCKET_PARAMS;
		goto out;
	}

	if (!(flags & GENAVB_SOCKF_ZEROCOPY) && (flags & GENAVB_SOCKF_TX_REUSE)) {
		rc = -GENAVB_ERR_SOCKET_PARAMS;
		goto out;
	}

	*sock = os_malloc(sizeof(struct genavb_socket_tx));
	if (!*sock) {
		rc = -GENAVB_ERR_NO_MEMORY;
		goto out;
	}
	os_memset(*sock, 0, sizeof(struct genavb_socket_tx));

	os_memcpy(&(*sock)->params, params, sizeof(struct genavb_socket_tx_params));
	(*sock)->flags = flags;

	if (params->addr.ptype == PTYPE_L2) {
		if (!(flags & GENAVB_SOCKF_RAW)) {
			/*
			 * Prepare L2 header, src MAC address
			 * is inserted in transmit path.
			 */
			if (params->addr.vlan_id != VLAN_VID_NONE) {
				(*sock)->header_len = net_add_eth_header(
					(*sock)->header_template,
					params->addr.u.l2.dst_mac,
					ETHERTYPE_VLAN);

				(*sock)->header_len += net_add_vlan_header(
					(*sock)->header_template + (*sock)->header_len,
					ntohs(params->addr.u.l2.protocol),
					ntohs(params->addr.vlan_id), params->addr.priority, 0);
			} else {
				(*sock)->header_len = net_add_eth_header(
					(*sock)->header_template,
					params->addr.u.l2.dst_mac,
					ntohs(params->addr.u.l2.protocol));
			}
		}
		addr = &params->addr;
	} else if (params->addr.ptype == PTYPE_PTP) {
		if (!(flags & GENAVB_SOCKF_RAW)) {
			/* use hardcoded PTP settings */
			const u8 ptp_dst_mac[6] = MC_ADDR_PTP;

			(*sock)->header_len = net_add_eth_header(
					(*sock)->header_template,
					ptp_dst_mac,
					ETHERTYPE_PTP);
		}
		addr = &params->addr;
	} else {
		rc = -GENAVB_ERR_INVALID;
		goto out_free_socket;
	}

	if (net_tx_init(&(*sock)->net, addr) < 0) {
		rc = -GENAVB_ERR_SOCKET_INIT;
		goto out_free_socket;
	}

	if (flags & GENAVB_SOCKF_RAW)
		net_tx_enable_raw(&(*sock)->net);

	return GENAVB_SUCCESS;

out_free_socket:
	os_free(*sock);

out:
	if (sock)
		*sock = NULL;

	return rc;
}

static int __genavb_socket_rx(struct genavb_socket_rx *sock, struct genavb_iovec *buf, struct genavb_socket_rx_receive_params *params, unsigned int n)
{
	struct net_rx_desc *desc_array[n], *desc;
	unsigned int data_len, dst_size;
	uint8_t *dst_buf;
	int rc, i, n_now;

	data_len = 0;

	if (!sock || !n) {
		rc = -GENAVB_ERR_INVALID;
		goto out;
	}

	if (!buf) {
		rc = -GENAVB_ERR_SOCKET_FAULT;
		goto out;
	}

	if (socket_rx_event_check(sock) < 0) {
		rc = -GENAVB_ERR_SOCKET_INTR;
		goto out;
	}

	n_now = __net_rx_multi(&sock->net, desc_array, n);
	if (n_now <= 0) {
		rc = -GENAVB_ERR_SOCKET_AGAIN;
		goto out_rearm;
	}

	if (sock->flags & GENAVB_SOCKF_ZEROCOPY) {
		for (i = 0; i < n_now; i++) {
			desc = desc_array[i];

			buf[i].iov_base = net_rx_desc_to_buffer(desc);
			buf[i].iov_len = desc->len;

			if (params && (params[i].flags & GENAVB_SOCKET_RX_TS))
				params[i].ts = desc->ts64;

		}
	} else {
		for (i = 0; i < n_now; i++) {
			desc = desc_array[i];

			dst_buf = buf[i].iov_base;
			dst_size = buf[i].iov_len;

			if (sock->flags & GENAVB_SOCKF_RAW)
				data_len = desc->len;
			else
				data_len = desc->len - (desc->l3_offset - desc->l2_offset);

			if (dst_size < data_len) {
				rc = i;
				goto out_free_desc;
			}

			if (sock->flags & GENAVB_SOCKF_RAW) {
					os_memcpy(dst_buf, (uint8_t *)desc + desc->l2_offset, data_len);
			} else {
					os_memcpy(dst_buf, (uint8_t *)desc + desc->l3_offset, data_len);
			}

			buf[i].iov_len = data_len;

			if (params && (params[i].flags & GENAVB_SOCKET_RX_TS))
				params[i].ts = desc->ts64;

		}

		net_free_multi((void **)&desc_array[0], n_now);
	}

	socket_rx_event_rearm(sock);

	return n_now;

out_free_desc:
	net_free_multi((void **)&desc_array[i], n_now - i);

out_rearm:
	socket_rx_event_rearm(sock);

out:
	return rc;
}

int genavb_socket_rx(struct genavb_socket_rx *sock, void *buf, unsigned int len, uint64_t *ts)
{
	struct genavb_iovec data = { .iov_base = buf, .iov_len = len };
	int rc;

	if (ts) {
		struct genavb_socket_rx_receive_params params = { .flags = GENAVB_SOCKET_RX_TS };
		rc =  __genavb_socket_rx(sock, &data, &params, 1);
		if (rc < 1) {
			if (rc == 0)
				rc = -GENAVB_ERR_SOCKET_AGAIN;
		} else {
			rc = data.iov_len;
			*ts = params.ts;
		}
	} else {
		rc =  __genavb_socket_rx(sock, &data, NULL, 1);
		if (rc < 1) {
			if (rc == 0)
				rc = -GENAVB_ERR_SOCKET_AGAIN;
		} else {
			rc = data.iov_len;
		}
	}

	return rc;
}

int genavb_socket_rx_receive_iov(struct genavb_socket_rx *sock, struct genavb_iovec *iovec, struct genavb_socket_rx_receive_params *params, unsigned int n)
{
	return __genavb_socket_rx(sock, iovec, params, n);
}

static int __genavb_socket_tx(struct genavb_socket_tx *sock, struct genavb_iovec *buf, struct genavb_socket_tx_send_params *params, unsigned int n)
{
	struct net_tx_desc *desc_array[n], *desc;
	int i;
	int rc, n_now;

	if (!sock || !n) {
		rc = -GENAVB_ERR_INVALID;
		goto out;
	}

	if (!buf) {
		rc = -GENAVB_ERR_SOCKET_FAULT;
		goto out;
	}

	if (sock->flags & GENAVB_SOCKF_ZEROCOPY) {
		n_now = n;

		for (i = 0; i < n_now; i++) {
			desc = net_tx_desc_from_buffer(buf[i].iov_base);

			desc->flags = 0;
			desc->len = buf[i].iov_len;
			desc->l2_offset = NET_DATA_OFFSET;
			desc->port = sock->params.addr.port;

			if (params) {
				if (params[i].flags & GENAVB_SOCKET_TX_TS) {
					desc->priv = params[i].priv;
					desc->flags |= NET_TX_FLAGS_HW_TS;
				}

				if (params[i].flags & GENAVB_SOCKET_TX_TIME) {
					desc->ts64 = params[i].ts;
					desc->flags |= NET_TX_FLAGS_TS64;
				}
			}

			if (sock->flags & GENAVB_SOCKF_TX_REUSE)
				desc->flags |= NET_TX_BUSY | NET_TX_REUSE;

			desc_array[i] = desc;
		}
	} else {
		unsigned int data_len, src_len;
		uint8_t *src;

		data_len = 0;
		for (i = 0; i < n; i++) {
			if (buf[i].iov_len > data_len)
				data_len = buf[i].iov_len;
		}

		data_len += (sock->flags & GENAVB_SOCKF_RAW) ? 0 : sock->header_len;

		rc = net_tx_alloc_multi(&sock->net, desc_array, n, data_len);
		if (rc <= 0) {
			rc = -GENAVB_ERR_NO_MEMORY;
			goto out;
		}

		n_now = rc;

		for (i = 0; i < n_now; i++) {
			desc = desc_array[i];
			src = buf[i].iov_base;
			src_len = buf[i].iov_len;

			if (params) {
				if (params[i].flags & GENAVB_SOCKET_TX_TS) {
					desc->priv = params[i].priv;
					desc->flags |= NET_TX_FLAGS_HW_TS;
				}

				if (params[i].flags & GENAVB_SOCKET_TX_TIME) {
					desc->ts64 = params[i].ts;
					desc->flags |= NET_TX_FLAGS_TS64;
				}
			}

			if (sock->flags & GENAVB_SOCKF_RAW) {
				os_memcpy((uint8_t *)desc + desc->l2_offset, src, src_len);
				desc->len = src_len;
			} else {
				os_memcpy((uint8_t *)desc + desc->l2_offset,
					  sock->header_template, sock->header_len);
				os_memcpy((uint8_t *)desc + desc->l2_offset + sock->header_len,
						src, src_len);
				desc->len = src_len + sock->header_len;
			}

			desc->port = sock->params.addr.port;
		}
	}

	rc = net_tx_multi(&sock->net, &desc_array[0], n_now);
	if (rc < n_now)
		goto out_free_desc;

	return n_now;

out_free_desc:
	net_free_multi((void **)&desc_array[rc], n_now - rc);

out:
	return rc;
}

int genavb_socket_tx(struct genavb_socket_tx *sock, void *buf, unsigned int len)
{
	struct genavb_iovec buf_iov = { .iov_base = buf, .iov_len = len };
	int rc;

	rc = __genavb_socket_tx(sock, &buf_iov, NULL, 1);
	if (rc < 1) {
		if (rc == 0)
			rc = -GENAVB_ERR_SOCKET_TX;
	} else {
		rc = GENAVB_SUCCESS;
	}

	return rc;
}

int genavb_socket_tx_send(struct genavb_socket_tx *sock, void *buf, unsigned int len, struct genavb_socket_tx_send_params *params)
{
	struct genavb_iovec buf_iov = { .iov_base = buf, .iov_len = len };
	int rc;

	rc = __genavb_socket_tx(sock, &buf_iov, params, 1);
	if (rc < 1) {
		if (rc == 0)
			rc = -GENAVB_ERR_SOCKET_TX;
	} else {
		rc = GENAVB_SUCCESS;
	}

	return rc;
}

int genavb_socket_tx_send_iov(struct genavb_socket_tx *sock, struct genavb_iovec *iovec, struct genavb_socket_tx_send_params *params, unsigned int n)
{
	return __genavb_socket_tx(sock, iovec, params, n);
}

int genavb_socket_tx_get_ts(struct genavb_socket_tx *sock, uint64_t *ts, unsigned int *ts_priv)
{
	int ret;

	ret = net_tx_ts_get(&sock->net, ts, ts_priv);
	if (ret <= 0)
		return -1;

	return GENAVB_SUCCESS;
}

void genavb_socket_rx_close(struct genavb_socket_rx *sock)
{
	struct net_address *addr;

	if (!sock)
		return;

	addr = &sock->params.addr;

	if (MAC_IS_MCAST(addr->u.l2.dst_mac)) {
		net_del_multi(&sock->net, addr->port,
			      addr->u.l2.dst_mac);
	}

	socket_rx_event_exit(sock);

	net_rx_exit(&sock->net);

	os_free(sock);

	return;
}

void genavb_socket_tx_close(struct genavb_socket_tx *sock)
{
	if (!sock)
		return;

	net_tx_exit(&sock->net);

	os_free(sock);

	return;
}

#else /* CONFIG_SOCKET */

int genavb_socket_rx_open(struct genavb_socket_rx **sock, genavb_sock_f_t flags, struct genavb_socket_rx_params *params)
{
	return -1;
}

int genavb_socket_tx_open(struct genavb_socket_tx **sock, genavb_sock_f_t flags, struct genavb_socket_tx_params *params)
{
	return -1;
}

int genavb_socket_rx(struct genavb_socket_rx *sock, void *buf, unsigned int len, uint64_t *ts)
{
	return -1;
}

int genavb_socket_rx_receive_iov(struct genavb_socket_rx *sock, struct genavb_iovec *iovec, struct genavb_socket_rx_receive_params *params, unsigned int n)
{
	return -1;
}

int genavb_socket_tx(struct genavb_socket_tx *sock, void *buf, unsigned int len)
{
	return -1;
}

int genavb_socket_tx_send(struct genavb_socket_tx *sock, void *buf, unsigned int len, struct genavb_socket_tx_send_params *params)
{
	return -1;
}

int genavb_socket_tx_send_iov(struct genavb_socket_tx *sock, struct genavb_iovec *iovec, struct genavb_socket_tx_send_params *params, unsigned int n)
{
	return -1;
}

int genavb_socket_tx_get_ts(struct genavb_socket_tx *sock, uint64_t *ts, unsigned int *ts_priv)
{
	return -1;
}

void genavb_socket_rx_close(struct genavb_socket_rx *sock)
{
	return;
}

void genavb_socket_tx_close(struct genavb_socket_tx *sock)
{
	return;
}

#endif /* CONFIG_SOCKET */
