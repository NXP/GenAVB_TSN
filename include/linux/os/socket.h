/*
 * Copyright 2020, 2023 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 \file
 \brief OS specific GenAVB public control API
 \details OS specific qos API definition for the GenAVB library

 \copyright Copyright 2020, 2023 NXP
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
