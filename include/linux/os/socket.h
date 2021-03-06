/*
 * Copyright 2020 NXP
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
 \file qos.h
 \brief OS specific GenAVB public control API
 \details OS specific qos API definition for the GenAVB library

 \copyright Copyright 2020 NXP
*/

#ifndef _OS_GENAVB_PUBLIC_SOCKET_API_H_
#define _OS_GENAVB_PUBLIC_SOCKET_API_H_

/** Retrieve the file descriptor associated with a given socket.
 * \ingroup socket
 * \return		GENAVB_SUCCESS or negative error code.
 * \param sock		Socket handler
 */
int genavb_socket_rx_fd(struct genavb_socket_rx *sock);

/** Retrieve the file descriptor associated with a given socket.
 * \ingroup socket
 * \return		GENAVB_SUCCESS or negative error code.
 * \param sock		Socket handler
 */
int genavb_socket_tx_fd(struct genavb_socket_tx *sock);

#endif /* _OS_GENAVB_PUBLIC_SOCKET_API_H_ */
