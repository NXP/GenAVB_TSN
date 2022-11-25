/*
 * Copyright 2018 NXP
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *    Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *    Neither the name of NXP Semiconductors nor the
 *    names of its contributors may be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 \file socket.h
 \brief GenAVB public API
 \details Socket API definition for the GenAVB library

 \copyright Copyright 2018
*/

#ifndef _GENAVB_PUBLIC_SOCKET_API_H_
#define _GENAVB_PUBLIC_SOCKET_API_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "config.h"
#include "types.h"
#include "net_types.h"

struct genavb_socket_rx;
struct genavb_socket_tx;

/**
 * \ingroup socket
 * Socket rx parameters
 */
struct genavb_socket_rx_params {
	struct net_address addr; /**< Socket address */
};

/**
 * \ingroup socket
 * Socket rx parameters
 */
struct genavb_socket_tx_params {
	struct net_address addr; /**< Socket address */
};

/** Open rx socket
 * \ingroup socket
 * \return		::GENAVB_SUCCESS or negative error code.
 * \param sock		Socket handle
 */
int genavb_socket_rx_open(struct genavb_socket_rx **sock, genavb_sock_f_t flags,
			  struct genavb_socket_rx_params *params);

/** Open tx socket
 * \ingroup socket
 * \return		::GENAVB_SUCCESS or negative error code.
 * \param sock		Socket handle
 */
int genavb_socket_tx_open(struct genavb_socket_tx **sock, genavb_sock_f_t flags,
			  struct genavb_socket_tx_params *params);

/** Socket transmit
 * \ingroup socket
 * \return		::GENAVB_SUCCESS or negative error code.
 * \param sock		Socket handle
 */
int genavb_socket_tx(struct genavb_socket_tx *sock, void *buf, unsigned int len);

/** Socket receive
 * \ingroup socket
 * \return		::GENAVB_SUCCESS or negative error code.
 * \param sock		Socket handle
 */
int genavb_socket_rx(struct genavb_socket_rx *sock, void *buf, unsigned int len, uint64_t *ts);

/** Close rx socket
 * \ingroup socket
 * \param sock		Socket handle
 */
void genavb_socket_rx_close(struct genavb_socket_rx *sock);

/** Close tx socket
 * \ingroup socket
 * \param sock		Socket handle
 */
void genavb_socket_tx_close(struct genavb_socket_tx *sock);

/* OS specific headers */
#include "os/socket.h"

#endif /* _GENAVB_PUBLIC_SOCKET_API_H_ */

