/*
* Copyright 2018, 2020 NXP
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
 \file streaming.h
 \brief GenAVB API private includes
 \details private definitions for the GenAVB library

 \copyright Copyright 2018, 2020 NXP
*/

#ifndef _LINUX_PRIVATE_STREAMING_H_
#define _LINUX_PRIVATE_STREAMING_H_

#include "common/list.h"
#include "include/genavb/genavb.h"

#include "init.h"

struct genavb_stream_handle {
	struct list_head list;
	int fd;
	struct genavb_handle *genavb;
	struct genavb_stream_params params;
	unsigned int max_payload_size;	/* Transmit maximum packet payload size (in byte units) */
	unsigned int partial_iovec;
	int expect_new_frame;
	unsigned int batch;		/* Transmit batch (in packet units) */
};

#endif /* _LINUX_PRIVATE_STREAMING_H_ */
