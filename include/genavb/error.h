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
 \file error.h
 \brief GenAVB public API
 \details Error code definitions

 \copyright Copyright 2018 NXP
*/
#ifndef _GENAVB_PUBLIC_ERROR_H_
#define _GENAVB_PUBLIC_ERROR_H_

/** GenAVB Return codes
 * \ingroup generic
 *
 */
typedef enum {
	GENAVB_SUCCESS = 0,		/**< Success. */
	GENAVB_ERR_NO_MEMORY,		/**< Out of memory */
	GENAVB_ERR_ALREADY_INITIALIZED,	/**< Library already initialized */
	GENAVB_ERR_INVALID,		/**< Invalid library handle */
	GENAVB_ERR_INVALID_PARAMS, 	/**< Invalid parameters */
	GENAVB_ERR_STREAM_API_OPEN,	/**< Stream open error */
	GENAVB_ERR_STREAM_BIND,		/**< Stream bind error */
	GENAVB_ERR_STREAM_TX,		/**< Stream data write error */
	GENAVB_ERR_STREAM_RX,		/**< Stream data read error */
	GENAVB_ERR_STREAM_INVALID,	/**< Stream handle invalid */
	GENAVB_ERR_STREAM_PARAMS,	/**< Stream parameters invalid */
	GENAVB_ERR_STREAM_TX_NOT_ENOUGH_DATA,	/**< Stream data write error due to not enough data*/
	GENAVB_ERR_STREAM_NO_CALLBACK,	/**< Stream callback no set */
	GENAVB_ERR_CTRL_TRUNCATED,	/**< Control message truncated */
	GENAVB_ERR_CTRL_INIT,		/**< Control init error */
	GENAVB_ERR_CTRL_ALLOC,		/**< Control message allocation error */
	GENAVB_ERR_CTRL_TX,		/**< Control message write error */
	GENAVB_ERR_CTRL_RX,		/**< Control message read error */
	GENAVB_ERR_CTRL_LEN,		/**< Invalid control message length */
	GENAVB_ERR_CTRL_TIMEOUT,	/**< Control message timeout */
	GENAVB_ERR_CTRL_INVALID,	/**< Invalid message on a control channel */
	GENAVB_ERR_CTRL_FAILED,		/**< Control command failed */
	GENAVB_ERR_CTRL_UNKNOWN,	/**< Control command unknown */
	GENAVB_ERR_STACK_NOT_READY,	/**< GenAVB stack not ready or not started. */
	GENAVB_ERR_SOCKET_INIT,		/**< Socket open error */
	GENAVB_ERR_SOCKET_PARAMS,	/**< Socket parameters invalid */
	GENAVB_ERR_SOCKET_AGAIN,	/**< Socket no data available */
	GENAVB_ERR_SOCKET_INVALID,	/**< Socket parameters invalid */
	GENAVB_ERR_SOCKET_FAULT,	/**< Socket invalid address */
	GENAVB_ERR_SOCKET_INTR,		/**< Socket blocking rx interrupted */
	GENAVB_ERR_SOCKET_TX,		/**< Socket transmit error */
	GENAVB_ERR_SOCKET_BUFLEN,	/**< Socket buffer length error */
	GENAVB_ERR_CLOCK,		/**< Clock error */
	GENAVB_ERR_TIMER,		/**< Timer error */
	GENAVB_ERR_ST,			/**< QoS Scheduled Traffic error */
 	GENAVB_ERR_CTRL_MAX
} genavb_return_codes_t;

/** Return human readable error message from error code
 * \ingroup generic
 * \return	error string
 * \param 	error error code returned by GenAVB library functions
 */
const char *genavb_strerror(int error);

#endif /* GENAVB_PUBLIC_ERROR_H */

