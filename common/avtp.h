/*
* Copyright 2014-2015 Freescale Semiconductor, Inc.
* Copyright 2016-2018, 2020 NXP
* 
* NXP Confidential. This software is owned or controlled by NXP and may only 
* be used strictly in accordance with the applicable license terms.  By expressly 
* accepting such terms or by downloading, installing, activating and/or otherwise 
* using the software, you are agreeing that you have read, and that you agree to 
* comply with and are bound by, such license terms.  If you do not agree to be 
* bound by the applicable license terms, then you may not retain, install, activate 
* or otherwise use the software.
*/

/**
 @file		avtp.h
 @brief   	AVTP protocol common definitions
 @details	PDU and protocol definitions for all AVTP applications
*/

#ifndef _PROTO_AVTP_H_
#define _PROTO_AVTP_H_

#include "common/types.h"
#include "common/avdecc.h"
#include "config.h"
#include "genavb/ether.h"
#include "genavb/avtp.h"
#include "genavb/sr_class.h"
#include "genavb/streaming.h"

struct avtp_ctrl_hdr {
#ifdef CFG_AVTP_1722A_BROKEN	/* FIXME support of AVTP control according to 1722a needs more work */
#ifdef __BIG_ENDIAN__
	u32 subtype:8;

	u32 sv:1;
	u32 version:3;

	u32 format_specific_data:9;
	u32 control_data_length:11;

#else
	u32 subtype:8;

	u32 format_specific_data_1:4; /* FIXME this field is broken, since it crosses a byte boundary */
	u32 version:3;
	u32 sv:1;

	u32 control_data_length_1:3; /* FIXME this field is broken, since it crosses a byte boundary */
	u32 format_specific_data_2:5; /* FIXME this field is broken, since it crosses a byte boundary */

	u32 control_data_length_2:8; /* FIXME this field is broken, since it crosses a byte boundary */
#endif

#else
	u8 subtype;

#ifdef __BIG_ENDIAN__
	u8 sv:1;
	u8 version:3;
	u8 control_data:4;

	u16 control_data_len_status;
#else
	u8 control_data:4;
	u8 version:3;
	u8 sv:1;

	u16 control_data_len_status;
#endif

#endif /* !CFG_AVTP_1722A_BROKEN */
	// FIXME no stream_id?
};

#define AVTP_GET_STATUS(hdr) \
	((ntohs(hdr->control_data_len_status) >> 11) & 0x1F)

#define AVTP_GET_CTRL_DATA_LEN(hdr) \
	(ntohs(hdr->control_data_len_status) & 0x7FF)

#define AVTP_SET_CTRL_DATA_STATUS(hdr, status, cdl) \
	(hdr->control_data_len_status = htons(((status & 0x1F) << 11) | (cdl & 0x7FF)))

/*
  Minimal avtp presentation offset, in avtp stack, to avoid network late transmit/underflow (under
  worst case cpu load).
  If avtp stack does periodic processing, with a "latency" period, when cpu load approaches 100%, it will
  take up to "latency" time to process avtp data.
  Data that starts being processed at t0, may only finish processing at t0 + latency.
  This means avtp packets should have a transmit timestamp that is at least "latency" in the future,
  otherwise their transmit time may already be in the past when they reach the network transmit buffer
*/
static inline unsigned int _avtp_stream_presentation_offset(sr_class_t sr_class, unsigned int latency)
{
	return sr_class_max_transit_time(sr_class) + latency;
}

/*
  Minimal avtp presentation offset, in media stack, to avoid network late transmit/underflow
  If avtp stack does periodic processing, with a "latency" period, it may take up to "latency"
  time for it to process new data.
*/
static inline unsigned int stream_presentation_offset(sr_class_t sr_class, unsigned int latency)
{
	return _avtp_stream_presentation_offset(sr_class, latency) + latency;
}

static inline unsigned int avtp_fmt_sample_size(unsigned int subtype, const struct avdecc_format *format)
{
	unsigned int sample_size;

	switch (subtype) {
	case AVTP_SUBTYPE_NTSCF:  /* not avdecc defined format */
		sample_size = 1;
		break;
	default:
		sample_size = avdecc_fmt_sample_size(format);
		break;
	}

	return sample_size;
}

#endif /* _PROTO_AVTP_H_ */
