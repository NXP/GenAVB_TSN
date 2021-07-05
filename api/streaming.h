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
 \brief GenAVB API private streaming API includes
 \details private definitions for the GenAVB library streaming API

 \copyright Copyright 2018, 2020 NXP
*/

#ifndef _PRIVATE_STREAMING_H_
#define _PRIVATE_STREAMING_H_

#include "genavb/genavb.h"

#include "api/init.h"
#include "api_os/streaming.h"

int connect_avtp(struct genavb_handle *genavb, struct genavb_stream_handle *stream);

int disconnect_avtp(struct genavb_handle *genavb, struct genavb_stream_params const *params);

#endif /* _PRIVATE_STREAMING_H_ */
