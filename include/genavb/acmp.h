/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
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
 \file acmp.h
 \brief GenAVB public API
 \details 1722.1 ACMP helper definitions.

 \copyright Copyright 2015 Freescale Semiconductor, Inc.
*/
#ifndef _GENAVB_PUBLIC_ACMP_H_
#define _GENAVB_PUBLIC_ACMP_H_


/**
 * \ingroup control
 * ACMP message types.
 * Follows definition of table 8.1 from IEEE1722.1-2013 */
typedef enum{
	ACMP_CONNECT_TX_COMMAND			= 0,	/**< Connect Talker Stream source command */
	ACMP_CONNECT_TX_RESPONSE		= 1,	/**< Connect Talker Stream source response */
	ACMP_DISCONNECT_TX_COMMAND		= 2,	/**< Disconnect Talker Stream source command. */
	ACMP_DISCONNECT_TX_RESPONSE		= 3,	/**< Disconnect Talker Stream source response. */
	ACMP_GET_TX_STATE_COMMAND		= 4,	/**< Get Talker Stream source connection state command. */
	ACMP_GET_TX_STATE_RESPONSE		= 5,	/**< Get Talker Stream source connection state response. */
	ACMP_CONNECT_RX_COMMAND			= 6,	/**< Connect Listener Stream sink command. */
	ACMP_CONNECT_RX_RESPONSE		= 7,	/**< Connect Listener Stream sink response. */
	ACMP_DISCONNECT_RX_COMMAND		= 8,	/**< Disconnect Listener Stream sink command. */
	ACMP_DISCONNECT_RX_RESPONSE		= 9,	/**< Disconnect Listener Stream sink response. */
	ACMP_GET_RX_STATE_COMMAND		= 10,	/**< Get Listener Stream sink connection state command. */
	ACMP_GET_RX_STATE_RESPONSE		= 11,	/**< Get Listener Stream sink connection state response. */
	ACMP_GET_TX_CONNECTION_COMMAND		= 12,	/**< Get a specific Talker connection info command. */
	ACMP_GET_TX_CONNECTION_RESPONSE		= 13	/**< Get a specific Talker connection info response. */
} acmp_message_type_t;

/**
 * \ingroup control
 * ACMP Status codes returned by the stack to the application.
 * Follows definition of table 8.2 from IEEE1722.1-2013 */
typedef enum{
	ACMP_STAT_SUCCESS			= 0,	/**< Command executed successfully. */
	ACMP_STAT_LISTENER_UNKNOWN_ID		= 1,	/**< Listener does not have the specified unique identifier. */
	ACMP_STAT_TALKER_UNKNOWN_ID		= 2,	/**< Talker does not have the specified unique identifier. */
	ACMP_STAT_TALKER_DEST_MAC_FAIL		= 3,	/**< Talker could not allocate a destination MAC for the Stream. */
	ACMP_STAT_TALKER_NO_STREAM_INDEX	= 4,	/**< Talker does not have an available Stream index for the Stream. */
	ACMP_STAT_TALKER_NO_BANDWIDTH		= 5,	/**< Talker could not allocate bandwidth for the Stream. */
	ACMP_STAT_TALKER_EXCLUSIVE		= 6,	/**< Talker already has an established Stream and only supports one Listener. */
	ACMP_STAT_LISTENER_TALKER_TIMEOUT	= 7,	/**< Listener had timeout for all retries when trying to send command to Talker. */
	ACMP_STAT_LISTENER_EXCLUSIVE		= 8,	/**< The AVDECC Listener already has an established connection to a Stream. */
	ACMP_STAT_STATE_UNAVAILABLE		= 9,	/**< Could not get the state from the AVDECC Entity. */
	ACMP_STAT_NOT_CONNECTED			= 10,	/**< Trying to disconnect when not connected or not connected to the AVDECC Talker specified. */
	ACMP_STAT_NO_SUCH_CONNECTION		= 11,	/**< Trying to obtain connection info for an AVDECC Talker connection which does not exist. */
	ACMP_STAT_COULD_NOT_SEND_MESSAGE	= 12,	/**< The AVDECC Listener failed to send the message to the AVDECC Talker. */
	ACMP_STAT_TALKER_MISBEHAVING		= 13,	/**< Talker was unable to complete the command because an internal error occurred. */
	ACMP_STAT_LISTENER_MISBEHAVING		= 14,	/**< Listener was unable to complete the command because an internal error occurred. */
	ACMP_STAT_CONTROLLER_NOT_AUTHORIZED	= 16,	/**< The AVDECC Controller with the specified Entity ID is not authorized to change Stream connections. */
	ACMP_STAT_INCOMPATIBLE_REQUEST		= 17,	/**< The AVDECC Listener is trying to connect to an AVDECC Talker that is already streaming with a different traffic class, etc. or does not support the requested traffic class. */
	ACMP_STAT_NOT_SUPPORTED			= 31,	/**< The command is not supported. */
} acmp_status_t;


/**
 * \ingroup control
 * ACMP message flags.
 * Follows definition of table 8.3 from IEEE 1722.1-2013 */
#define ACMP_FLAG_CLASS_B			(1 << 0) /**< Indicates that the Stream is Class B instead of Class A (default 0 is class A). */
#define ACMP_FLAG_FAST_CONNECT			(1 << 1) /**< Fast Connect Mode, the connection is being attempted in fast connect mode. */
#define ACMP_FLAG_SAVED_STATE			(1 << 2) /**< Connection has saved state (used in Get State only). */
#define ACMP_FLAG_STREAMING_WAIT		(1 << 3) /**< The AVDECC Talker does not start streaming until explicitly being told to by the control protocol. */
#define ACMP_FLAG_SUPPORTS_ENCRYPTED		(1 << 4) /**< Indicates that the Stream supports streaming with encrypted PDUs. */
#define ACMP_FLAG_ENCRYPTED_PDU			(1 << 5) /**< Indicates that the Stream is using encrypted PDUs. */
#define ACMP_FLAG_TALKER_FAILED			(1 << 6) /**< Indicates that the listener has registered an SRP Talker Failed attribute for the Stream. */

#endif /* _GENAVB_PUBLIC_ACMP_H_ */

