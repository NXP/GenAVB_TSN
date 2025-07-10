/*
 * Copyright 2016 Freescale Semiconductor, Inc.
 * Copyright 2016, 2018-2020, 2023-2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 \file
 \brief OS specific GenAVB/TSN public control API
 \details OS specific packet API definition for the GenAVB/TSN library
*/

#ifndef _OS_GENAVB_PUBLIC_SOCKET_API_H_
#define _OS_GENAVB_PUBLIC_SOCKET_API_H_


/** Frees an array of receive buffers.
 * \ingroup socket_rx
 * \param buf		array of receive buffers, on entry per packet.
 * \param n			number of array entries.
 */
void genavb_socket_rx_free(void **buf, unsigned int n);

/** Set socket rx callback
 * \ingroup socket_rx
 * \return		GENAVB_SUCCESS or negative error code.
 * \param sock		Socket handle
 * \param callback	Socket rx callback
 * \param data		Socket callback data
 */
int genavb_socket_rx_set_callback(struct genavb_socket_rx *sock, void (*callback)(void *), void *data);

/** Enable socket rx callback
 * \ingroup socket_rx
 * \return		GENAVB_SUCCESS or negative error code.
 * \param sock		Socket handle
 */
int genavb_socket_rx_enable_callback(struct genavb_socket_rx *sock);

/** Set socket rx options.
 *
 * Currently the only supported options is ::GENAVB_SOCKET_RX_OPTION_TC_MASK.
 * \ingroup socket_rx
 * \return		GENAVB_SUCCESS or negative error code.
 * \param sock		Socket handle
 * \param option	socket option to set
 * \param value		option value to set
 */
int genavb_socket_rx_set_option(struct genavb_socket_rx *sock, genavb_socket_rx_option_t option, unsigned long value);

/** Allocates an array  of transmit buffers, all of size "size".
 * \ingroup socket_tx
 * \return			number of allocated buffers.
 * \param sock		Socket handle
 * \param buf		array of buffers.
 * \param n			number of buffers to allocate.
 * \param size		size of the allocated buffers.
 */
int genavb_socket_tx_alloc(struct genavb_socket_tx *sock, void **buf, unsigned int n, unsigned int size);

/** Frees an array of transmit buffers, previously allocated using genavb_socket_tx_alloc().
 * \ingroup socket_tx
 * \param buf		array of buffers to free.
 * \param n			number of buffers to free.
 */
void genavb_socket_tx_free(void **buf, unsigned int n);

/** Checks the transmit done status of an array of transmit buffers.
 * If a transmit socket is open with the ::GENAVB_SOCKF_TX_REUSE, this
 * function must be called for buffers previously passed to the socket transmit fuction.
 * Only when the buffers are done, can they be used again.
 *
 * \ingroup socket_tx
 * \return			number of buffers that have completed transmit.
 * \param sock		Socket handle
 * \param buf		array of buffers.
 * \param n			number of buffers to check.
 */
int genavb_socket_tx_done(struct genavb_socket_tx *sock, void **buf, unsigned int n);

/** Set socket tx callback
 * \ingroup socket_tx
 * \return		GENAVB_SUCCESS or negative error code.
 * \param sock		Socket handle
 * \param callback	Socket tx callback
 * \param data		Socket callback data
 */
int genavb_socket_tx_set_callback(struct genavb_socket_tx *sock, void (*callback)(void *), void *data);

/** Enable socket tx callback
 * \ingroup socket_tx
 * \return		GENAVB_SUCCESS or negative error code.
 * \param sock		Socket handle
 */
int genavb_socket_tx_enable_callback(struct genavb_socket_tx *sock);

/** Get logical port's hardware MAC address
 * \ingroup socket
 * \return		GENAVB_SUCCESS or negative error code.
 * \param port_id	logical port ID
 * \param addr		address pointer to store MAC address
 */
int genavb_socket_get_hwaddr(unsigned int port_id, unsigned char *addr);

#endif /* _OS_GENAVB_PUBLIC_SOCKET_API_H_ */
