/*
 * Copyright 2018, 2020, 2023-2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 \file
 \brief GenAVB/TSN public API
 \details Socket API definition for the GenAVB/TSN library
*/

#ifndef _GENAVB_PUBLIC_SOCKET_API_H_
#define _GENAVB_PUBLIC_SOCKET_API_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "config.h"
#include "clock.h"
#include "types.h"
#include "net_types.h"

struct genavb_socket_rx;
struct genavb_socket_tx;

/**
 * \defgroup socket		Socket
 * \ingroup library
 *
 * The socket API allows the creation of a network endpoint and exchanging layer 2 network frames.
 *
 * \ref socket_tx "Transmit" and \ref socket_rx "receive" sockets are handled separately and each of them requires its own (unidirectional) socket.
 * Parameters are set when the socket is opened and cannot be changed afterward.
 *
 * In receive, two approaches are possible:
 * * the stream is periodic like isochronous time-sensitive traffic. The application is aware of the frames timing and therefore can just "poll" the socket at the expected time.
 *
 * * the stream has no known timing and the application needs to be notified when frames are available for read.
 *
 * In transmit, a time sensitive application expects being able to send its frames at a defined timing. If not it's generally a critical error and the send will  be retried. Nonetheless, for some OS, some APIs may be available to be notified when buffer space is available for transmit.
 *
 * \if LINUX
 *
 * Using the file descriptor returned by the ::genavb_socket_rx_fd and ::genavb_socket_rx_fd  functions, an application may call poll/epoll/select system calls to sleep and be woken up only when received data is available or when buffer space is available for data to transmit.
 *
 * \else
 *
 * A callback can be registered using ::genavb_socket_rx_set_callback which is called when received data is available.
 * The callback must not block and should notify another task which performs the actual data reception.
 * After the callback has been called it must be re-armed by calling ::genavb_socket_rx_enable_callback. This should be done after the application has read all data available/written all data available (and never from the callback itself).
 *
 * \endif
 *
 * \defgroup socket_tx	Socket Transmit
 * \ingroup socket
 *
 * Socket is created using ::genavb_socket_tx_open and closed using ::genavb_socket_tx_close. The ::PTYPE_L2 and the ::PTYPE_PTP protocol types are currently supported.
 * The socket networking parameters are set in ::genavb_socket_tx_params addr field. See @ref net_addr for a complete description.
 *
 * The following transmit flags are available, set using ::genavb_sock_f_t flag:
 * * If ::GENAVB_SOCKF_RAW is set, it's the caller responsibility to set the layer 2 header in transmitted packets. The l2/ptp member of ::net_address is not used.
 * * If ::GENAVB_SOCKF_RAW isn't set, the layer 2 header is added internally to transmit packets, based on the l2/ptp members of ::net_address.
 * \if RTOS
 * * If ::GENAVB_SOCKF_ZEROCOPY is set, the socket is configured in zero-copy mode and it's the caller responsibility to allocate transmit buffers using ::genavb_socket_tx_alloc. For this mode to work, it is mandatory that the mode ::GENAVB_SOCKF_RAW is also set.
 *  + Moreover, it's possible to combine ::GENAVB_SOCKF_TX_REUSE with ::GENAVB_SOCKF_ZEROCOPY and enable transmit buffer recycling. If ::GENAVB_SOCKF_TX_REUSE is not set, the stack will free transmit buffers internally after transmission completes. The application needs to allocate new transmit buffers at each transmission.
 *  + If ::GENAVB_SOCKF_TX_REUSE is set, the application can re-use transmit buffers in subsequent transmissions. To do so safely, it needs to check if the transmission is complete by calling ::genavb_socket_tx_done before re-using the buffers. When the application no longer requires the buffers it needs to explicitly free them by calling ::genavb_socket_tx_free.
 * * If ::GENAVB_SOCKF_ZEROCOPY is not set, the frame is copied before the function returns so that the provided buffer can be freed.
 * \endif
 *
 * Frames are transmitted using one of the following functions :
 * * ::genavb_socket_tx
 * * ::genavb_socket_tx_send
 * * ::genavb_socket_tx_send_iov
 *
 * The per packet transmit parameters are set in ::genavb_socket_tx_send_params. See ::genavb_socket_tx_send_flags_t for a complete description.
 *
 * \defgroup socket_rx	Socket Receive
 * \ingroup socket
 *
 * Socket is created using ::genavb_socket_rx_open and closed using ::genavb_socket_rx_close.
 * The socket networking parameters are set in ::genavb_socket_rx_params addr field. See @ref net_addr for a complete description.
 * The following receive protocol types are available:
 * * ::PTYPE_L2: the frame matching is performed using the VLAN ID and the destination MAC address. The other fields of ::net_address are not used.
 * * ::PTYPE_OTHER: the socket will receive all the frames which have not been received by another GenAVB/TSN socket. This protocol type is generally used to connect a generic TCP/IP protocol stack.
 *
 * The following receive flags are available, set using ::genavb_sock_f_t flag:
 * * If ::GENAVB_SOCKF_NONBLOCK is set, the socket is configured in non-blocking mode and the call to ::genavb_socket_rx never blocks. If no frame is available for reading, the API returns ::GENAVB_ERR_SOCKET_AGAIN.
 * \if RTOS
 * * If ::GENAVB_SOCKF_NONBLOCK is not set, the socket is configured in blocking mode and the call to ::genavb_socket_rx only returns if a frame has been received or because an error occured. To use the blocking mode a task needs to be dedicated to receive handling.
 * * If ::GENAVB_SOCKF_ZEROCOPY is set,  the socket is configured in zero-copy mode and it's the caller responsibility to free the received buffers, using ::genavb_socket_rx_free free function. For this mode to work, it is mandatory that the mode ::GENAVB_SOCKF_RAW is also set.
 * * If ::GENAVB_SOCKF_ZEROCOPY is not set, the frame is copied to the application buffer(s) address(es) so that the received frame(s) can be freed.
 *
 * Frames are received by calling one of the following functions:
 * * ::genavb_socket_rx for receiving a single packet
 * * ::genavb_socket_rx_receive_iov for receiving multiple packets
 *
 * The receive socket also can provide specific customization by setting options. For more details, see ::genavb_socket_rx_set_option.
 * \endif
 *
 * \defgroup net_addr	Network Address
 * \ingroup socket
 *
 * The ::net_address addr field is used to configure the socket networking parameters:
 * * ptype: the protocol type. Needs to be set to ::PTYPE_L2 or ::PTYPE_OTHER
 * * port: the logical port number
 * * vlan_id: the VLAN ID in network order (can be set to ::VLAN_VID_NONE if no vlan is required)
 * * priority: traffic priority (range 0 to 7) (only used in transmit)
 * * u.l2: the l2 address
 *   + dst_mac: destination MAC address
 *   + protocol: the ether type (only used in transmit)
 *
 */

/**
 * \ingroup socket
 * Socket rx/tx parameters
 */
typedef enum {
	GENAVB_SOCKF_NONBLOCK	= 0x01,	/**< Non-blocking mode (only applies to receive socket) */

/** \cond RTOS */
	GENAVB_SOCKF_ZEROCOPY	= 0x02,	/**< Zero-copy mode (only implemented for RTOS, requires ::GENAVB_SOCKF_RAW too) */
/** \endcond */

	GENAVB_SOCKF_RAW		= 0x04,	/**< Raw socket */

/** \cond RTOS */
	GENAVB_SOCKF_TX_REUSE	= 0x08	/**< Re-use tx buffers */
/** \endcond */
} genavb_sock_f_t;

/** \cond RTOS */

/**
 * \ingroup socket_rx
 * Socket rx options
 */
typedef enum {
	GENAVB_SOCKET_RX_OPTION_TC_MASK = (1 << 0)		/**< Skip traffic class(es) for rx processing. Bitmask value with one bit per traffic class, e.g, bit0 = traffic class 0, bit1 = traffic class 1, ... */
} genavb_socket_rx_option_t;

/** \endcond */

/**
 * \ingroup socket_rx
 * Socket rx receive flags
 */
typedef enum {
	GENAVB_SOCKET_RX_TS = (1 << 0)		/**< Request receive timestamp */
} genavb_socket_rx_receive_flags_t;

/**
 * \ingroup socket_tx
 * Socket tx send flags
 */
typedef enum {
	GENAVB_SOCKET_TX_TS = (1 << 0),		/**< Request transmit timestamp */
	GENAVB_SOCKET_TX_TIME = (1 << 1)	/**< Specify transmit time */
} genavb_socket_tx_send_flags_t;


/**
 * \ingroup socket_rx
 * Socket rx parameters
 */
struct genavb_socket_rx_params {
	struct net_address addr; /**< Socket address */
};

/**
 * \ingroup socket_rx
 * Socket rx receive parametes
 */
struct genavb_socket_rx_receive_params {
	genavb_socket_rx_receive_flags_t flags;		/**< Socket rx receive flags */
	uint64_t ts;				/**< Timestamp, in nanoseconds */
};

/**
 * \ingroup socket_tx
 * Socket tx parameters
 */
struct genavb_socket_tx_params {
	struct net_address addr; /**< Socket address */
};

/**
 * \ingroup socket_tx
 * Socket tx send parameters
 */
struct genavb_socket_tx_send_params {
	genavb_socket_tx_send_flags_t flags;	/**< Socket tx send flags */
	unsigned long priv;			/**< Private data for socket tx PTP timestamps. This value is returned when calling ::genavb_socket_tx_get_ts. */
	uint64_t ts;				/**< Transmit time, in nanoseconds */
};

/** Open rx socket
 * \ingroup socket_rx
 * \return		::GENAVB_SUCCESS or negative error code.
 * \param sock		Socket handle
 * \param flags		Socket receive flags
 * \param params	Socket receive parameters
 */
int genavb_socket_rx_open(struct genavb_socket_rx **sock, genavb_sock_f_t flags,
			  struct genavb_socket_rx_params *params);

/** Open tx socket
 * \ingroup socket_tx
 * \return		::GENAVB_SUCCESS or negative error code.
 * \param sock		Socket handle
 * \param flags		Socket transmit flags
 * \param params	Socket transmit parameters
 */
int genavb_socket_tx_open(struct genavb_socket_tx **sock, genavb_sock_f_t flags,
			  struct genavb_socket_tx_params *params);

/** Socket transmit single packet.
 *
 * A success return code means that the frame has been correctly queued.
 * \ingroup socket_tx
 * \return			::GENAVB_SUCCESS or negative error code.
 * \param sock		Socket handle
 * \param buf		buffer containing the data to send. The buffer must be contiguous, including layer 2 headers if ::GENAVB_SOCKF_RAW is set and excluding them if not.
 * \param len		length of the data in bytes.
 */
int genavb_socket_tx(struct genavb_socket_tx *sock, void *buf, unsigned int len);

/** Socket transmit single packet with generic parameters.
 *
 * The latter is specified by using ::genavb_socket_tx_send_flags_t flag (e.g ::GENAVB_SOCKET_TX_TIME for packet Time Specific Departure).
 * A success return code means that the frame has been correctly queued.
 * \ingroup socket_tx
 * \return			::GENAVB_SUCCESS or negative error code.
 * \param sock		Socket handle
 * \param buf		buffer containing the data to send. The buffer must be contiguous, including layer 2 headers if ::GENAVB_SOCKF_RAW is set and excluding them if not.
 * \param len		length of the data in bytes.
 * \param params	transmit parameters.
 */
int genavb_socket_tx_send(struct genavb_socket_tx *sock, void *buf, unsigned int len,
			struct genavb_socket_tx_send_params *params);

/** Socket transmit multiple packets with generic parameters.
 *
 * It requires the structure ::genavb_iovec to be created and populated.
 * \ingroup socket_tx
 * \return		number of packets transmitted (may be zero) or negative error code.
 * \param sock		Socket handle
 * \param iovec		array of packets to transmit, one entry per packet.
 * \param params	array of transmit parameters, one entry per packet.
 * \param n			number of array entries.
 */
int genavb_socket_tx_send_iov(struct genavb_socket_tx *sock, struct genavb_iovec *iovec,
			struct genavb_socket_tx_send_params *params, unsigned int n);

/** Retrieve timestamp after socket transmit with timestamp required
 * \ingroup socket_tx
 * \return		::GENAVB_SUCCESS or negative error code.
 * \param sock		Socket handle
 * \param ts		timestamp retrieved.
 * \param ts_priv	private data for the timestamp.
 */
int genavb_socket_tx_get_ts(struct genavb_socket_tx *sock, uint64_t *ts, unsigned int *ts_priv);

/** Socket receive single packet with possibility to retrieve receive timestamp.
 * \ingroup socket_rx
 * \return			length of data received (in bytes) or negative error code.
 * \param sock		Socket handle
 * \param buf		buffer where data is to be copied. The buffer must be contiguous.
 * \param len		length of the buffer in bytes.
 * \if RTOS
 *					This parameter is not used if the socket was opened with the ::GENAVB_SOCKF_ZEROCOPY flag.
 *
 *					If the ::GENAVB_SOCKF_ZEROCOPY flag is not used, the buffer length must be at least the minimum Ethernet layer 2 frame size.
 * \endif
 * \param ts		pointer to where to save receive timestamps.
 */
int genavb_socket_rx(struct genavb_socket_rx *sock, void *buf, unsigned int len, uint64_t *ts);

/** Socket receive multiple packets with generic parameters.
 *
 * Receive timestamps can be requested by setting ::GENAVB_SOCKET_RX_TS.
 * \ingroup socket_rx
 * \return		the number of packets received (may be zero) or negative error code.
 * \param sock		Socket handle
 * \param iovec		array of buffers for received packets, one entry per packet.
 * \if RTOS
 *					The len member of ::genavb_iovec is not used if the socket was opened with the ::GENAVB_SOCKF_ZEROCOPY flag.
 * \endif
 * \param params	array of receive parameters, one entry per packet.
 * \param n			number of array entries.
 */
int genavb_socket_rx_receive_iov(struct genavb_socket_rx *sock, struct genavb_iovec *iovec,
			struct genavb_socket_rx_receive_params *params, unsigned int n);

/** Close rx socket
 * \ingroup socket_rx
 * \param sock		Socket handle
 */
void genavb_socket_rx_close(struct genavb_socket_rx *sock);

/** Close tx socket
 * \ingroup socket_tx
 * \param sock		Socket handle
 */
void genavb_socket_tx_close(struct genavb_socket_tx *sock);

/* OS specific headers */
#include "os/socket.h"

#endif /* _GENAVB_PUBLIC_SOCKET_API_H_ */
