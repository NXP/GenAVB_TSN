/*
 * Copyright 2018, 2023 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 \file error.h
 \brief GenAVB public API
 \details Error code definitions

 \copyright Copyright 2018, 2023 NXP
*/
#ifndef _GENAVB_PUBLIC_ERROR_H_
#define _GENAVB_PUBLIC_ERROR_H_

typedef enum {
	GENAVB_ERR_TYPE_GENERAL = 0,
	GENAVB_ERR_TYPE_STREAM,
	GENAVB_ERR_TYPE_CTRL,
	GENAVB_ERR_TYPE_SOCKET,
	GENAVB_ERR_TYPE_TIMER,
	GENAVB_ERR_TYPE_CLOCK,
	GENAVB_ERR_TYPE_ST,
	GENAVB_ERR_TYPE_SF,
	GENAVB_ERR_TYPE_SG,
	GENAVB_ERR_TYPE_FM,
	GENAVB_ERR_TYPE_MAX
} genavb_err_type_t;

#define GENAVB_ERR(type, num)	(((GENAVB_ERR_TYPE_ ## type) << 10) | ((num) & 0x3ff))
#define GENAVB_ERR_TYPE(err)	((err) >> 10)
#define GENAVB_ERR_NUM(err)	((err) & 0x3ff)

/** GenAVB Return codes
 * \ingroup generic
 *
 */
typedef enum {
	GENAVB_SUCCESS = 			GENAVB_ERR(GENERAL, 0), /**< Success. */
	GENAVB_ERR_NO_MEMORY =			GENAVB_ERR(GENERAL, 1), /**< Out of memory */
	GENAVB_ERR_ALREADY_INITIALIZED =	GENAVB_ERR(GENERAL, 2), /**< Library already initialized */
	GENAVB_ERR_INVALID =			GENAVB_ERR(GENERAL, 3), /**< Invalid library handle */
	GENAVB_ERR_INVALID_PARAMS = 		GENAVB_ERR(GENERAL, 4), /**< Invalid parameters */
	GENAVB_ERR_INVALID_PORT = 		GENAVB_ERR(GENERAL, 5), /**< Invalid port */

	GENAVB_ERR_STREAM_API_OPEN =		GENAVB_ERR(STREAM, 0), /**< Stream open error */
	GENAVB_ERR_STREAM_BIND =		GENAVB_ERR(STREAM, 1), /**< Stream bind error */
	GENAVB_ERR_STREAM_TX = 			GENAVB_ERR(STREAM, 2), /**< Stream data write error */
	GENAVB_ERR_STREAM_RX = 			GENAVB_ERR(STREAM, 3), /**< Stream data read error */
	GENAVB_ERR_STREAM_INVALID = 		GENAVB_ERR(STREAM, 4), /**< Stream handle invalid */
	GENAVB_ERR_STREAM_PARAMS = 		GENAVB_ERR(STREAM, 5), /**< Stream parameters invalid */
	GENAVB_ERR_STREAM_TX_NOT_ENOUGH_DATA = 	GENAVB_ERR(STREAM, 6), /**< Stream data write error due to not enough data*/
	GENAVB_ERR_STREAM_NO_CALLBACK = 	GENAVB_ERR(STREAM, 7), /**< Stream callback no set */

	GENAVB_ERR_CTRL_TRUNCATED = 		GENAVB_ERR(CTRL, 0), /**< Control message truncated */
	GENAVB_ERR_CTRL_INIT = 			GENAVB_ERR(CTRL, 1), /**< Control init error */
	GENAVB_ERR_CTRL_ALLOC = 		GENAVB_ERR(CTRL, 2), /**< Control message allocation error */
	GENAVB_ERR_CTRL_TX = 			GENAVB_ERR(CTRL, 3), /**< Control message write error */
	GENAVB_ERR_CTRL_RX = 			GENAVB_ERR(CTRL, 4), /**< Control message read error */
	GENAVB_ERR_CTRL_LEN = 			GENAVB_ERR(CTRL, 5), /**< Invalid control message length */
	GENAVB_ERR_CTRL_TIMEOUT = 		GENAVB_ERR(CTRL, 6), /**< Control message timeout */
	GENAVB_ERR_CTRL_INVALID = 		GENAVB_ERR(CTRL, 7), /**< Invalid message on a control channel */
	GENAVB_ERR_CTRL_FAILED = 		GENAVB_ERR(CTRL, 8), /**< Control command failed */
	GENAVB_ERR_CTRL_UNKNOWN = 		GENAVB_ERR(CTRL, 9), /**< Control command unknown */
	GENAVB_ERR_STACK_NOT_READY = 		GENAVB_ERR(CTRL, 10), /**< GenAVB stack not ready or not started. */
	GENAVB_ERR_PTP_DOMAIN_INVALID = 	GENAVB_ERR(CTRL, 11), /**< PTP domain invalid */

	GENAVB_ERR_SOCKET_INIT = 		GENAVB_ERR(SOCKET, 0),	/**< Socket open error */
	GENAVB_ERR_SOCKET_PARAMS = 		GENAVB_ERR(SOCKET, 1),	/**< Socket parameters invalid */
	GENAVB_ERR_SOCKET_AGAIN = 		GENAVB_ERR(SOCKET, 2),	/**< Socket no data available */
	GENAVB_ERR_SOCKET_INVALID = 		GENAVB_ERR(SOCKET, 3),	/**< Socket parameters invalid */
	GENAVB_ERR_SOCKET_FAULT = 		GENAVB_ERR(SOCKET, 4),	/**< Socket invalid address */
	GENAVB_ERR_SOCKET_INTR = 		GENAVB_ERR(SOCKET, 5),	/**< Socket blocking rx interrupted */
	GENAVB_ERR_SOCKET_TX = 			GENAVB_ERR(SOCKET, 6),	/**< Socket transmit error */
	GENAVB_ERR_SOCKET_BUFLEN = 		GENAVB_ERR(SOCKET, 7),	/**< Socket buffer length error */

	GENAVB_ERR_CLOCK = 			GENAVB_ERR(CLOCK, 0),	/**< Clock error */

	GENAVB_ERR_TIMER = 			GENAVB_ERR(TIMER, 0),	/**< Timer error */

	GENAVB_ERR_ST = 			GENAVB_ERR(ST, 0),	/**< QoS Scheduled Traffic error */
	GENAVB_ERR_ST_NOT_SUPPORTED = 			GENAVB_ERR(ST, 1),	/**< QoS Scheduled Traffic not supported error */
	GENAVB_ERR_ST_HW_CONFIG = 			GENAVB_ERR(ST, 2),	/**< QoS Scheduled Traffic hardware config error */
	GENAVB_ERR_ST_MAX_SDU_NOT_SUPPORTED = 		GENAVB_ERR(ST, 3), /**< QoS Scheduled Traffic max SDU config error */

	GENAVB_ERR_SF_NOT_SUPPORTED = 			GENAVB_ERR(SF, 0),	/**< Stream Filter not supported */

	GENAVB_ERR_SG_NOT_SUPPORTED =			GENAVB_ERR(SG, 0),	/**< Stream Gate not supported */
	GENAVB_ERR_SG_INVALID_CYCLE_PARAMS =	GENAVB_ERR(SG, 1),	/**< Stream Gate invalid cycle parameters */
	GENAVB_ERR_SG_INVALID_CYCLE_TIME =		GENAVB_ERR(SG, 2),	/**< Stream Gate invalid cycle time */
	GENAVB_ERR_SG_INVALID_BASETIME =		GENAVB_ERR(SG, 3),	/**< Stream Gate invalid base time */
	GENAVB_ERR_SG_GETTIME =					GENAVB_ERR(SG, 4),	/**< Stream Gate gettime error */

	GENAVB_ERR_FM_NOT_SUPPORTED = 			GENAVB_ERR(FM, 0),	/**< Flow Meter not supported */
} genavb_err_t;

/** Return human readable error message from error code
 * \ingroup generic
 * \return	error string
 * \param 	error error code returned by GenAVB library functions
 */
const char *genavb_strerror(int error);

#endif /* GENAVB_PUBLIC_ERROR_H */
