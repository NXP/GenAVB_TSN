/*
 * Copyright 2017 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _H264_CAMERA_H_
#define _H264_CAMERA_H_

#ifdef CFG_AVTP_1722A
#include "../common/gstreamer.h"
#include "../common/gst_pipeline_definitions.h"
#include "../common/gstreamer_single.h"

void h264_camera_init(void);
int h264_camera_connect(struct gstreamer_stream *h264_stream);
void h264_camera_disconnect(struct gstreamer_stream *h264_stream);


#endif


#endif /* _H264_CAMERA_H_ */
