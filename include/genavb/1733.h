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
 \file 1733.h
 \brief IEEE1733 public API
 \details API definition for the IEEE1733 library

 \copyright Copyright 2015 Freescale Semiconductor, Inc.
*/
#ifndef _GENAVB_PUBLIC_1733_H_
#define _GENAVB_PUBLIC_1733_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "sr_class.h"
#include "avtp.h"
#include "rtp.h"

/**
 * 1733 Return codes
 */
enum {
	AVB_1733_SUCCESS = 0,
	AVB_1733_ERR_NO_MEMORY,
	AVB_1733_ERR_SHMEM_INIT,
	AVB_1733_ERR_STREAM_CREATE,
	AVB_1733_ERR_STREAM_EXISTS,
	AVB_1733_ERR_STREAM_TX,
	AVB_1733_ERR_STREAM_TX_LEN,
	AVB_1733_ERR_STREAM_TX_ALLOC,
	AVB_1733_ERR_STREAM_RX,
	AVB_1733_ERR_STREAM_INVALID,
	AVB_1733_ERR_STREAM_DIRECTION,
	AVB_1733_ERR_STREAM_CONFIG,
	AVB_1733_ERR_MAX
};

typedef enum {
	AVB1733_MEDIA_CLOCK_DOMAIN_SW = 0,	/**< Clock domain based on media clock (interpolated in software), should be used for talker media capture */
	AVB1733_MEDIA_CLOCK_DOMAIN_PTP,	/**< Clock domain based on gPTP clock, should be used for talker media streaming */
	AVB1733_MEDIA_CLOCK_DOMAIN_STREAM	/**< Clock domain based on stream recovered clock, should be use for listener media playback */
} avb1733_clock_domain_t;


struct avb_1733_handle; // 1733 handle forward declaration
struct avb_1733_stream_handle; // 1733 stream handle forward declaration


/**
 * 1733 Stream creation parameters
 */
struct avb_1733_stream_params {
	int ip_version; /**< IPv4/IPv6 */

	/* IP (V4) */
	unsigned int daddrip; /**< Destination IP address */
	unsigned int saddrip; /**< Source IP address */

	/* UDP */
	unsigned short dport; /**< Destination UDP port */
	unsigned short sport; /**< Source UDP port */

	/* RTP */
	unsigned int ssrc; /**< SSRC Sync source identifier */
	unsigned int pt; /**< RTP payload type */

	avtp_direction_t direction;	/**< Stream direction */
	unsigned int channels;		/**< Number of channels per frame in the stream */
	unsigned int sampling_rate;	/**< Sampling rate for the stream in Hz */

	avb_u16 port;			/**< Network physical port (eth0, ...) */
	sr_class_t stream_class;	/**< Stream class */
	avb_u8 stream_id[8];		/**< Stream ID (in network order) */
	avb_u16 flags;
	avb1733_clock_domain_t clock_domain;		/**< Media clock domain used for this stream */

	struct {
		avb_u8 dst_mac[6];	/**< Stream destination mac address (in network order) */
		avb_u16 vlan_id;
	} talker; /**< Talker specific parameters */

	struct {
		unsigned int max_latency; /**< Drop packets if above (in nb of 64 frames packets) */
		unsigned int min_latency; /**< Pad with silence if below (in nb of 64 frames packets) */
	} listener; /**< Listener specific parameters */
};



/** Initialize the GenAVB library.
 * \return		AVB_1733_SUCCESS or negative error code. In case of success avb argument is updated with library handle
 * \param avb		pointer to 1733 library handle pointer
 * \param flags		bitmap of configuration options. For future expansion, for now always pass 0.
 */
int avb_1733_init(struct avb_1733_handle **avb, unsigned int flags);


/** Exit the GenAVB library.
 * \return		AVB_1733_SUCCESS or negative error code.
 * \param avb 		pointer to GenAVB library handle structure
 */
int avb_1733_exit(struct avb_1733_handle *avb);


/** Create a new 1733 stream.
 * \return 		AVB_1733_SUCCESS or negative error code. In case of success: stream argument is updated with stream handle, batch_size argument is updated with actual configured batch size.
 * \param avb		pointer to GenAVB library handle (input)
 * \param stream	pointer to stream handle pointer (output)
 * \param params	pointer to struct containing all required params. (input)
 */
int avb_1733_stream_create(struct avb_1733_handle *avb, struct avb_1733_stream_handle **streamh, struct avb_1733_stream_params const
							*params);



/** Retrieve the file descriptor associated with a given 1733 stream.
 * \return		socket/file descriptor (to be used with poll/select) for the stream, or negative error code.
 * \param stream	stream handle returned by genavb_stream_create.
 */
int avb_1733_stream_fd(struct avb_1733_stream_handle const *streamh);


/** Receive media data from a given avb stream.
* \return amount copied (in bytes, or negative error code (e.g invalid handle for stream receive). May be less than requested by data_len in case:
* * not enough data is available,
* \param stream			stream handle returned by ::genavb_stream_create.
* \param data			buffer where stream data is to be copied.
* \param data_len		length of the data in bytes.
 */
int avb_1733_stream_receive(struct avb_1733_stream_handle *handle, void *data, unsigned int data_len);


/** Receive media data from a given avb stream.
* \return amount copied (in bytes, or negative error code (e.g invalid handle for stream receive). May be less than requested by data_iov in case:
* * not enough data is available,
* \param stream			stream handle returned by ::genavb_stream_create.
* \param data_iov		iovec array where stream data is to be copied.
* \param data_iov_len		length of the data_iov array.
 */
int avb_1733_stream_receive_iov(struct avb_1733_stream_handle *handle, struct genavb_iovec *data_iov, unsigned int data_iov_len);



/** Send media data on a given AVTP stream.
 * \return 			amount copied (in bytes), or negative error code (e.g invalid fd for stream receive). May be less than requested in case:
 * * not enough network buffers were available,
 * \param stream		stream handle returned by ::genavb_stream_create.
 * \param data			buffer containing the data to send.
 * \param data_len		length of the data in bytes.
 */
int avb_1733_stream_send(struct avb_1733_stream_handle *stream, void const *data, unsigned int data_len);


/** Send media data on a given AVTP stream.
 * \return 			amount copied (in bytes), or negative error code (e.g invalid fd for stream receive). May be less than requested in case:
 * * not enough network buffers were available
 * \param stream		stream handle returned by ::genavb_stream_create.
 * \param data_iov		iovec array containing the data to send.
 * \param data_iov_len		length of the data_iov array.
 */
int avb_1733_stream_send_iov(struct avb_1733_stream_handle *stream, struct genavb_iovec const *data_iov, unsigned int data_iov_len);


/** Destroy a given AVTP stream.
 * \return		AVB_1733_SUCCESS or negative error code.
 * \param stream	stream handle for the stream to be destroyed.
 */
int avb_1733_stream_destroy(struct avb_1733_stream_handle *stream);

/** Return human readable error message from error code
 * \return	error string
 * \param 	error error code returned by 1733 AVB library functions
 */
const char *avb_1733_strerror(int error);

#ifdef __cplusplus
}
#endif

#endif /* _GENAVB_PUBLIC_1733_H_ */

