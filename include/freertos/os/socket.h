/*
 * Copyright 2018-2020, 2023 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 \file
 \brief OS specific GenAVB public control API
 \details OS specific packet API definition for the GenAVB library

 \copyright Copyright 2018-2020, 2023 NXP
*/

#ifndef _OS_GENAVB_PUBLIC_SOCKET_API_H_
#define _OS_GENAVB_PUBLIC_SOCKET_API_H_


/** Set socket rx callback
 * \ingroup socket
 * \return		GENAVB_SUCCESS or negative error code.
 * \param sock		Socket handler
 * \param callback	Socket rx callback
 * \param data		Socket callback data
 */
int genavb_socket_rx_set_callback(struct genavb_socket_rx *sock, void (*callback)(void *), void *data);

/** Enable socket rx callback
 * \ingroup socket
 * \return		GENAVB_SUCCESS or negative error code.
 * \param sock		Socket handler
 */
int genavb_socket_rx_enable_callback(struct genavb_socket_rx *sock);

#endif /* _OS_GENAVB_PUBLIC_SOCKET_API_H_ */
