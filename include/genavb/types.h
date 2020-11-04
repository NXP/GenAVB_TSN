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
 \file types.h
 \brief GenAVB public API
 \details  Basic types OS abstraction.

 \copyright Copyright 2014 Freescale Semiconductor, Inc.
*/
#ifndef _GENAVB_PUBLIC_TYPES_H_
#define _GENAVB_PUBLIC_TYPES_H_

#include "os/types.h"
#include "compat.h"

typedef uint64_t	avb_u64;
typedef uint32_t	avb_u32;
typedef uint16_t	avb_u16;
typedef uint8_t		avb_u8;
typedef int64_t		avb_s64;
typedef int32_t		avb_s32;
typedef int16_t		avb_s16;
typedef int8_t		avb_s8;

struct genavb_handle;
struct genavb_stream_handle;
struct genavb_control_handle;
struct genavb_stream_params;
struct genavb_timer;

/**
 * \ingroup socket
 * Socket rx parameters
 */
typedef enum {
	GENAVB_SOCKF_NONBLOCK = 0x01, /**< Non-blocking mode (only applies to receive socket) */
	GENAVB_SOCKF_ZEROCOPY = 0x02, /**< Zero-copy mode (not implemented) */
	GENAVB_SOCKF_RAW = 0x04	      /**< Raw socket (only applies to transmit socket) */
} genavb_sock_f_t;

typedef enum {
	GENAVB_TIMERF_ABS = 0x01,
} genavb_timer_f_t;

#ifndef NULL
#define NULL ((void*)0)
#endif


/**
 * \ingroup stream
 * Scatter/gather array items.
 * The definition of this structure is copied from the standard Linux iovec struct, used e.g. in recvmsg, sendmsg calls.
 */
struct genavb_iovec {
	void *iov_base;              /**< Starting address */
	unsigned int iov_len;        /**< Number of bytes/events to transfer */
};


/**
 * \ingroup stream
 * Receive events:
 * * ::AVTP_MEDIA_CLOCK_RESTART will be set if the mr bit is set for the first packet in the batch.
 * * ::AVTP_PACKET_LOST will be set if a packet (or several) was lost at the start of the batch.
 * * ::AVTP_END_OF_FRAME will be set based on information contained in the AVTP header, such as the M
 * bit in AVTP compressed video streams.
 *
 * As described in ::genavb_stream_receive, ::AVTP_MEDIA_CLOCK_RESTART and ::AVTP_PACKET_LOST shall only be set for
 * the first genavb_event of a batch. If ::AVTP_PACKET_LOST is set, event_data will be used to store the amount of bytes lost.
 *
 * Send events:
 * * ::AVTP_SYNC can be set to synchronize batch transmit/avtp timestamp to the gptp time specified in the event ts field.
 * * ::AVTP_FLUSH can be set as the first event to flush all stream data, up to the end of the data being sent through the single genavb_stream_send/genavb_stream_send_iov call that will include that event.
 * * ::AVTP_FRAME_END can be set as the first event to signal that the last byte of the data being sent corresponds to the end of a frame (e.g end of a NALU in a CVF H264 stream). This will have the same effect as ::AVTP_FLUSH also.
*/
struct genavb_event {
	unsigned int event_mask;		/**< Receive event mask: ::AVTP_TIMESTAMP_INVALID, ::AVTP_TIMESTAMP_UNCERTAIN, ::AVTP_MEDIA_CLOCK_RESTART, ::AVTP_PACKET_LOST, ::AVTP_END_OF_FRAME\n
							Send event mask: ::AVTP_SYNC, ::AVTP_FLUSH, ::AVTP_FRAME_END */
	unsigned int index;			/**< Receive/Send, offset of the event relative to the start of the batch, in bytes */
	unsigned int ts;			/**< Receive AVTP timestamp of the event, if ::AVTP_TIMESTAMP_INVALID is not set in event_mask\n
							Send AVTP timestamp of the event, if ::AVTP_SYNC is set in event_mask */
	unsigned int event_data;	/**< Additional data, whose interpretation will depend on the type(s) of event */
};


#endif /* _GENAVB_PUBLIC_TYPES_H_ */

