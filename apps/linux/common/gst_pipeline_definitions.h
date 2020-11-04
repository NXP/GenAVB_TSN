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

#ifndef _GST_PIPELINE_DEFINITIONS_H_
#define _GST_PIPELINE_DEFINITIONS_H_

#include <gst/gst.h>

struct gstreamer_pipeline;

struct gstreamer_pipeline_definition {
	char const * const pipeline_string;
	unsigned int const num_sources;
	unsigned int const num_sinks;
	GstClockTime const latency;

	int (*prepare)(struct gstreamer_pipeline *gst_pipeline);
};

extern struct gstreamer_pipeline_definition pipeline_talker_file_61883_4;
extern struct gstreamer_pipeline_definition pipeline_talker_file_61883_4_61883_6;
extern struct gstreamer_pipeline_definition pipeline_talker_file_61883_4_61883_6_h264;
extern struct gstreamer_pipeline_definition pipeline_talker_file_61883_4_61883_6_h264_preview;
extern struct gstreamer_pipeline_definition pipeline_talker_file_61883_4_preview;
extern struct gstreamer_pipeline_definition pipeline_talker_file_61883_4_61883_6_preview;
extern struct gstreamer_pipeline_definition pipeline_talker_file_61883_4_audio_mp3;
extern struct gstreamer_pipeline_definition pipeline_talker_file_61883_4_audio_m4a;
extern struct gstreamer_pipeline_definition pipeline_talker_file_61883_6_audio_mp3;
extern struct gstreamer_pipeline_definition pipeline_talker_file_61883_6_audio_m4a;
extern struct gstreamer_pipeline_definition pipeline_talker_file_61883_6_audio_wav;
extern struct gstreamer_pipeline_definition pipeline_talker_file_61883_4_61883_6_audio_mp3;
extern struct gstreamer_pipeline_definition pipeline_talker_file_61883_4_61883_6_audio_m4a;
extern struct gstreamer_pipeline_definition pipeline_talker_file_cvf_h264;
extern struct gstreamer_pipeline_definition pipeline_talker_file_cvf_h264_preview;

extern struct gstreamer_pipeline_definition pipeline_listener_61883_4_audio_video;
extern struct gstreamer_pipeline_definition pipeline_listener_61883_4_audio_only;
extern struct gstreamer_pipeline_definition pipeline_listener_61883_4_video_only;
extern struct gstreamer_pipeline_definition pipeline_listener_61883_6;
extern struct gstreamer_pipeline_definition pipeline_listener_cvf_mjpeg;
extern struct gstreamer_pipeline_definition pipeline_listener_cvf_h264;
extern struct gstreamer_pipeline_definition pipeline_cvf_mjpeg_four_cameras;
extern struct gstreamer_pipeline_definition pipeline_cvf_mjpeg_decode_only;
extern struct gstreamer_pipeline_definition pipeline_split_screen;
extern struct gstreamer_pipeline_definition pipeline_split_screen_compo;
extern struct gstreamer_pipeline_definition pipeline_cvf_mjpeg_four_cameras_compo;
extern struct gstreamer_pipeline_definition pipeline_listener_debug;

#define TSDEMUX_LATENCY			100000000	/* Must match latency compiled in gstreamer plugin */
#define ALSA_LATENCY			55000000	/* Must match the sum of  buffer time (size) and period time in alsasink plugin */
#define MPEGTS_PIPELINE_LATENCY		(TSDEMUX_LATENCY + ALSA_LATENCY) /*MPEGTS pipeline have tsdemux plugin and alsasink plugin advertising their minimum latency*/
#define H264_PIPELINE_LATENCY		0 /*H264 pipeline have no plugin advertising a minimum latency*/

#define LOCAL_PTS_OFFSET		150000000		/* PTS offset to compensate for global pipeline processing time: 150ms (in ns) */
#define DEFAULT_PTS_OFFSET		(LOCAL_PTS_OFFSET + MPEGTS_PIPELINE_LATENCY) /*Default and common PTS offset to guarantee sync between different listener pipelines*/
#define MAX_PTS_OFFSET			(1 * GST_SECOND)

#define MJPEG_PIPELINE_LATENCY		8000000
#define SALSA_LATENCY			33000000
#define CVF_MJPEG_PTS_OFFSET		(SALSA_LATENCY + MJPEG_PIPELINE_LATENCY)
#define OVERLAY_PIPELINE_LATENCY	2000000

#endif /* _GST_PIPELINE_DEFINITIONS_H_ */
