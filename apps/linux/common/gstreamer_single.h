/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2017 NXP
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

#ifndef _GSTREAMER_SINGLE_H_
#define _GSTREAMER_SINGLE_H_

#include <unistd.h>
#include <limits.h>
#include <string.h>

#include <genavb/genavb.h>
#include "stats.h"
#include "time.h"
#include "ts_parser.h"
#include "gstreamer.h"
#include "gst_pipeline_definitions.h"
#include "common.h"
#include "genavb/avdecc.h"

#define FILENAME_SIZE	SCHAR_MAX

// We currently need a low batch size to improve timestamping and synchronization accuracy,
//so we set it to the minimum accepted value
#define BATCH_SIZE		2048	// in bytes
#define BATCH_SIZE_CVF		65536   //131072 //in bytes
#define MAX_PKT_SIZE_CVF	2000

#define AVTP_TS_MIN_DELTA	50000000
#define AVTP_TS_MAX_DELTA	50000000
#define AVTP_TS_MIN_DELTA_STOP	150000000
#define AVTP_TS_MAX_DELTA_STOP	50000000

#define GST_MEMORY_OBJ_SIZE (300000)

#define GST_PRIORITY	1 /* RT_FIFO priority to be used for the process */

/**
 * Generic gstreamer stream parameters
 */
struct gstreamer_pipeline_source {
	struct gstreamer_pipeline *gst_pipeline;

	unsigned long long byte_count;
	unsigned int count;
	unsigned long long late_count;
	unsigned long long ontime_count;

	unsigned int dropping;
	unsigned int pipeline_state;

	GstBuffer *buffer;
	GstMapInfo info;
	GstMemory *memory;
	unsigned char *data_buf;
	GstAppSrc *source;
	GstAppSink *sink;

	int buffer_byte_count;
	int memory_byte_count;

	uint64_t tlast;
};

/**
 * Generic mpeg_ts stats
 */
struct mpeg_ts_stats {

	struct stats pcr_delay;
	struct stats pcr_period;
	unsigned long long pcr_prev;
};

/**
 * Generic stream stats
 */
struct gstreamer_stats {

	unsigned long long byte_count_prev;
	unsigned long long buffer_pts_prev;

	unsigned long long ts_err;
	unsigned long long pkt_lost;
	unsigned long long mr;

	struct stats write_delay;
	struct stats delay;
	struct stats period;
	struct stats rate;

	struct mpeg_ts_stats mpeg_ts;
};

struct gstreamer_stream {

	unsigned int source_index;

	struct avb_stream_handle *stream_h;

	unsigned int created;
	unsigned int data_received;

	void *thread;

	unsigned int state;
	int started;
	int stream_fd;

	struct avb_stream_params params;
	unsigned int batch_size;
	unsigned int frame_size;
	unsigned int flags;
	unsigned long long previous_frame_time;

	struct gstreamer_pipeline_source pipe_source;
	struct gstreamer_stats stream_stats;
	struct ts_parser ts_parser;

	int (*listener_gst_handler)(struct gstreamer_stream *stream, unsigned int events);

	void *data;	/*Will point to talker_gst_multi_app for talker stream*/

	int timer_fd;
	void *thread_timer;
};

void stream_init_stats(struct gstreamer_stream *stream);
void stream_update_stats(struct gstreamer_stream *stream, unsigned long long byte_count, unsigned long long now, unsigned long long buffer_pts);
void stream_dump_stats(struct gstreamer_stream *stream);
void stream_61883_4_update_stats(struct gstreamer_stream *stream, unsigned char *buf, unsigned long long buffer_pts);
void dump_stream_infos(struct gstreamer_stream *stream);
int listener_gst_handler_cvf(struct gstreamer_stream *stream, unsigned int events);
int listener_gst_handler(struct gstreamer_stream *stream, unsigned int events);
void apply_stream_params(struct gstreamer_stream *stream, struct avb_stream_params *stream_params);

#define CVF_MJPEG_SPLASH_FILENAME "/home/media/cvf_splash_screen.mjpg"
#define CVF_MJPEG_SPLASH_MAX_FRAME_SIZE 60000
#define CVF_MJPEG_SPLASH_FPS 30
int gst_cvf_mjpeg_warm_up_pipeline(struct gstreamer_pipeline *gst, unsigned int nframes);

#endif /* _GSTREAMER_SINGLE_H_ */
