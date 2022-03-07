/*
* Copyright 2014-2015 Freescale Semiconductor, Inc.
* Copyright 2016, 2020 NXP
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
 @file		avdecc.h
 @brief   	AVDECC common definitions and functions
*/

#ifndef _PROTO_AVDECC_H_
#define _PROTO_AVDECC_H_

#include "genavb/avdecc.h"
#include "genavb/sr_class.h"

unsigned int avdecc_fmt_sample_stride(const struct avdecc_format *format);

static inline unsigned int avdecc_fmt_payload_size(const struct avdecc_format *format, sr_class_t sr_class)
{
	unsigned int max_frame_size, max_interval_frames;

	return __avdecc_fmt_payload_size(format, sr_class, &max_frame_size, &max_interval_frames);
}

#endif /* _PROTO_AVDECC_H_ */
