/*
 * Copyright 2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2020, 2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <sys/ioctl.h>
#include <arpa/inet.h>

#include "log.h"
#include "avb_stream.h"
#include "avb_stream_config.h"
#include "time.h"
#include <genavb/genavb.h>
#include <genavb/avdecc.h>
#include "msrp.h"
#include "common.h"

const unsigned int g_max_avb_talker_sets = MAX_AVB_TALKERS;
aar_avb_set_t g_avb_talker_sets[MAX_AVB_TALKERS];

const unsigned int g_max_avb_listener_sets = MAX_AVB_LISTENERS;
aar_avb_set_t g_avb_listener_sets[MAX_AVB_LISTENERS];

static struct avb_handle *s_avb_handle = NULL;

static aar_avb_stream_t *avbstream_get_talker_stream(unsigned int unique_id)
{
	if (unique_id >= g_max_avb_talker_streams)
		return NULL;

	return &g_avb_talker_streams[unique_id];
}

static aar_avb_set_t *avbstream_get_talker_set(unsigned int unique_id)
{
	aar_avb_set_t *avbset;

	if (unique_id >= g_max_avb_talker_sets)
		return NULL;

	avbset = &g_avb_talker_sets[unique_id];

	if (avbset->stream_count == 0) {
		/* First stream of the set. */

		/* Restart the statistic counter */
		avbset->stats.counter_stats.tx_err = 0;
		avbset->stats.counter_stats.rx_err = 0;
		avbset->stats.counter_stats.batch_rx = 0;
		avbset->stats.counter_stats.batch_tx = 0;

		/* Init the quality statistics */
		avbset->last_gptp_time = 0;
		avbset->is_first_wakeup = 1;

		stats_init(&avbset->stats.gptp_2cont_wakeup, 31, avbset, NULL);
	}

	avbset->stream_count++;

	return avbset;
}

static void avbstream_put_talker_set(aar_avb_set_t *set)
{
	if (!set)
		return;

	if (set->stream_count > 0)
		set->stream_count--;
}

static aar_avb_stream_t *avbstream_get_listener_stream(unsigned int unique_id)
{
	if (unique_id >= g_max_avb_listener_streams)
		return NULL;

	return &g_avb_listener_streams[unique_id];
}

static aar_avb_set_t *avbstream_get_listener_set(unsigned int unique_id)
{
	aar_avb_set_t *avbset;

	if (unique_id >= g_max_avb_listener_sets)
		return NULL;

	avbset = &g_avb_listener_sets[unique_id];

	if (avbset->stream_count == 0) {
		/* First stream of the set. */

		/* Restart the statistic counter */
		avbset->stats.counter_stats.tx_err = 0;
		avbset->stats.counter_stats.rx_err = 0;
		avbset->stats.counter_stats.batch_rx = 0;
		avbset->stats.counter_stats.batch_tx = 0;

		/* Init the quality statistics */
		avbset->last_gptp_time = 0;
		avbset->last_event_ts = 0;
		avbset->is_first_wakeup = 1;
		stats_init(&avbset->stats.gptp_2cont_wakeup, 31, avbset, NULL);
		stats_init(&avbset->stats.event_2cont_wakeup, 31, avbset, NULL);
		stats_init(&avbset->stats.event_gptp, 31, avbset, NULL);
	}

	avbset->stream_count++;

	return avbset;
}

static void avbstream_put_listener_set(aar_avb_set_t *set)
{
	if (!set)
		return;

	if (set->stream_count > 0)
		set->stream_count--;
}

int avbstream_init(void)
{
	int avb_result;
	int rc;

	avb_result = avb_init(&s_avb_handle, 0);
	if (avb_result != AVB_SUCCESS) {
		ERR("avb_init() failed: %s", avb_strerror(avb_result));
		// Initialization failed
		rc = -1;
		goto err_init;
	}

	DBG("avb_handle: %p", s_avb_handle);

	return 0;

err_init:
	return rc;
}

int avbstream_exit(void)
{
	avb_exit(s_avb_handle);

	s_avb_handle = NULL;

	return 0;
}


static void print_stream_param(struct avb_stream_params * stream_params)
{
	INF("direction:            %d", stream_params->direction);
	INF("port:                 %d", stream_params->port);
	INF("stream_class:         %d", stream_params->stream_class);
	INF("clock_domain:         %d", stream_params->clock_domain);
	INF("flags:                %d", stream_params->flags);
	INF("talker.latency:       %d", stream_params->talker.latency);
	INF("stream_id:            " STREAM_STR_FMT, STREAM_STR(stream_params->stream_id));
	INF("dst_mac:              " MAC_STR_FMT, MAC_STR(stream_params->dst_mac));
	INF("format.u.s.v:         %d", stream_params->format.u.s.v);
	INF("format.u.s.subtype:   %d", stream_params->format.u.s.subtype);
	if (stream_params->format.u.s.subtype == AVTP_SUBTYPE_61883_IIDC) {
		INF("format.u.s.subtype_u.iec61883.r:   %d", stream_params->format.u.s.subtype_u.iec61883.r);
		INF("format.u.s.subtype_u.iec61883.sf:  0x%X", stream_params->format.u.s.subtype_u.iec61883.sf);
		INF("format.u.s.subtype_u.iec61883.fmt: 0x%X", stream_params->format.u.s.subtype_u.iec61883.fmt);
		if (stream_params->format.u.s.subtype_u.iec61883.sf == IEC_61883_SF_61883) {
			if (stream_params->format.u.s.subtype_u.iec61883.fmt == IEC_61883_CIP_FMT_6) {
				INF("format.u.s.subtype_u.iec61883.format_u.iec61883_6.fdf_u.fdf.evt: 0x%X", stream_params->format.u.s.subtype_u.iec61883.format_u.iec61883_6.fdf_u.fdf.evt);
				INF("format.u.s.subtype_u.iec61883.format_u.iec61883_6.fdf_u.fdf.sfc: %d", stream_params->format.u.s.subtype_u.iec61883.format_u.iec61883_6.fdf_u.fdf.sfc);
				INF("format.u.s.subtype_u.iec61883.format_u.iec61883_6.dbs:           %d", stream_params->format.u.s.subtype_u.iec61883.format_u.iec61883_6.dbs);
				INF("format.u.s.subtype_u.iec61883.format_u.iec61883_6.sc:            %d", stream_params->format.u.s.subtype_u.iec61883.format_u.iec61883_6.sc);
				INF("format.u.s.subtype_u.iec61883.format_u.iec61883_6.ut:            %d", stream_params->format.u.s.subtype_u.iec61883.format_u.iec61883_6.ut);
				INF("format.u.s.subtype_u.iec61883.format_u.iec61883_6.nb:            %d", stream_params->format.u.s.subtype_u.iec61883.format_u.iec61883_6.nb);
				INF("format.u.s.subtype_u.iec61883.format_u.iec61883_6.b:             %d", stream_params->format.u.s.subtype_u.iec61883.format_u.iec61883_6.b);
				INF("format.u.s.subtype_u.iec61883.format_u.iec61883_6.label_iec_60958_cnt: %d", stream_params->format.u.s.subtype_u.iec61883.format_u.iec61883_6.label_iec_60958_cnt);
				INF("format.u.s.subtype_u.iec61883.format_u.iec61883_6.label_mbla_cnt:      %d", stream_params->format.u.s.subtype_u.iec61883.format_u.iec61883_6.label_mbla_cnt);
				INF("format.u.s.subtype_u.iec61883.format_u.iec61883_6.label_midi_cnt:      %d", stream_params->format.u.s.subtype_u.iec61883.format_u.iec61883_6.label_midi_cnt);
				INF("format.u.s.subtype_u.iec61883.format_u.iec61883_6.label_smptecnt:      %d", stream_params->format.u.s.subtype_u.iec61883.format_u.iec61883_6.label_smptecnt);
			}
		}
	} else if (stream_params->format.u.s.subtype == AVTP_SUBTYPE_CVF) {
		INF("format.u.s.subtype_u.cvf.format: %d", stream_params->format.u.s.subtype_u.cvf.format);
		INF("format.u.s.subtype_u.cvf.subtype: %d", stream_params->format.u.s.subtype_u.cvf.subtype);
		if (stream_params->format.u.s.subtype_u.cvf.subtype == CVF_FORMAT_SUBTYPE_MJPEG) {
			INF("format.u.s.subtype_u.cvf.format_u.mjpeg.p:      %d", stream_params->format.u.s.subtype_u.cvf.format_u.mjpeg.p);
			INF("format.u.s.subtype_u.cvf.format_u.mjpeg.type:   %d", stream_params->format.u.s.subtype_u.cvf.format_u.mjpeg.type);
			INF("format.u.s.subtype_u.cvf.format_u.mjpeg.width:  %d", stream_params->format.u.s.subtype_u.cvf.format_u.mjpeg.width);
			INF("format.u.s.subtype_u.cvf.format_u.mjpeg.height: %d", stream_params->format.u.s.subtype_u.cvf.format_u.mjpeg.height);
		} else if (stream_params->format.u.s.subtype_u.cvf.subtype == CVF_FORMAT_SUBTYPE_JPEG2000) {
			INF("format.u.s.subtype_u.cvf.format_u.jpeg2000.p:   %d", stream_params->format.u.s.subtype_u.cvf.format_u.jpeg2000.p);
		}
	} else if (stream_params->format.u.s.subtype == AVTP_SUBTYPE_CRF) {
		INF("format.u.s.subtype_u.crf.type:                   %d", stream_params->format.u.s.subtype_u.crf.type);
		INF("format.u.s.subtype_u.crf.pull:                   %d", stream_params->format.u.s.subtype_u.crf.pull);
		INF("format.u.s.subtype_u.crf.timestamps_per_pdu:     %x", stream_params->format.u.s.subtype_u.crf.timestamps_per_pdu);
		INF("format.u.s.subtype_u.crf.timestamp_interval_msb: %x", stream_params->format.u.s.subtype_u.crf.timestamp_interval_msb);
		INF("format.u.s.subtype_u.crf.timestamp_interval_lsb: %x", stream_params->format.u.s.subtype_u.crf.timestamp_interval_lsb);
#ifdef __BIG_ENDIAN__
		INF("format.u.s.subtype_u.crf.base_frequency:         %x", stream_params->format.u.s.subtype_u.crf.base_frequency);
#else
		INF("format.u.s.subtype_u.crf.base_frequency_msb:     %x", stream_params->format.u.s.subtype_u.crf.base_frequency_msb);
		INF("format.u.s.subtype_u.crf.base_frequency_lsb:     %x", stream_params->format.u.s.subtype_u.crf.base_frequency_lsb);
#endif // __BIG_ENDIAN__
	}
	INF("format binary: %lx", *(long unsigned int *)&stream_params->format);
	INF("sample_rate: %d", avdecc_fmt_sample_rate(&stream_params->format));
	INF("sample_size: %d", avdecc_fmt_sample_size(&stream_params->format));
}

unsigned int avbstream_batch_size(unsigned int batch_size_ns, struct avb_stream_params *params)
{
	return (((uint64_t)batch_size_ns * avdecc_fmt_sample_rate(&params->format) * avdecc_fmt_sample_size(&params->format) + NSECS_PER_SEC - 1) / NSECS_PER_SEC);
}

int avbstream_listener_add(unsigned int unique_id, struct avb_stream_params *params, aar_avb_stream_t **stream)
{
	struct avb_stream_params *stream_params;
	aar_avb_stream_t *avbstream;
	aar_avb_set_t *avbset;
	int result;

	DBG("unique_id: %d", unique_id);

	if (!params) {
		ERR("Invalid AVB stream params for stream(%u)\n", unique_id);
		return -1;
	}

	avbstream = avbstream_get_listener_stream(unique_id);
	if (!avbstream) {
		ERR("Could not get listener AVB stream unique id %d", unique_id);
		return -1;
	} else if (avbstream->stream_handle) {
		ERR("Stream (%p) already used.", avbstream);
		return -1;
	}

	avbset = avbstream_get_listener_set(params->set_id);
	if (!avbset) {
		ERR("Could not get listener AVB set unique id %u", params->set_id);
		return -1;
	}

	avbstream->set = avbset;

	stream_params = &avbstream->stream_params;

	memcpy(stream_params, params, sizeof(*params));

	stream_params->talker.latency = 0;

	DBG("avbstream: %p, avbset: %p, stream_params: %p", avbstream, avbset, stream_params);

	print_stream_param(stream_params);

	avbstream->batch_size_ns = max(avbstream->batch_size_ns, sr_class_interval_p(avbstream->stream_params.stream_class) / sr_class_interval_q(avbstream->stream_params.stream_class));

	INF("stream_id: " STREAM_STR_FMT, STREAM_STR(stream_params->stream_id));
	INF("dst_mac: " MAC_STR_FMT, MAC_STR(stream_params->dst_mac));

	// Initialize listener stream for RX
	avbstream->cur_batch_size = avbstream_batch_size(avbstream->batch_size_ns, stream_params);

	result = avb_stream_create(s_avb_handle, &avbstream->stream_handle, stream_params, &avbstream->cur_batch_size, 0);
	if (result != 0) {
		ERR("create listener stream failed, err %d", result);
		return -1;
	}

	avbstream->clock_avtp = genavb_stream_avtp_clock(avbstream->stream_handle);

	INF("batch: %u, clock_avtp %d", avbstream->cur_batch_size, avbstream->clock_avtp);

	if (stream)
		*stream = avbstream;


	DBG("avb stream created, stream: %p", avbstream);

	return 0;
}

int avbstream_talker_add(unsigned int unique_id, struct avb_stream_params *params, aar_avb_stream_t **stream)
{
	struct avb_stream_params *stream_params;
	aar_avb_stream_t *avbstream;
	aar_avb_set_t *avbset;
	int result;

	DBG("unique_id: %d", unique_id);

	if (!params) {
		ERR("Invalid AVB stream params for stream(%u)\n", unique_id);
		return -1;
	}

	avbstream = avbstream_get_talker_stream(unique_id);
	if (!avbstream) {
		ERR("Could not get talker AVB stream unique id %d", unique_id);
		return -1;
	} else if (avbstream->stream_handle) {
		ERR("Stream (%p) already used.", avbstream);
		return -1;
	}

	avbset = avbstream_get_talker_set(params->set_id);
	if (!avbset) {
		ERR("Could not get listener AVB set unique id %u", params->set_id);
		return -1;
	}

	avbstream->set = avbset;

	stream_params = &avbstream->stream_params;

	memcpy(stream_params, params, sizeof(*params));

	DBG("avbstream: %p, stream_params: %p", avbstream, stream_params);

	print_stream_param(stream_params);

	avbstream->batch_size_ns = max(avbstream->batch_size_ns, sr_class_interval_p(avbstream->stream_params.stream_class) / sr_class_interval_q(avbstream->stream_params.stream_class));

	INF("stream_id: " STREAM_STR_FMT, STREAM_STR(stream_params->stream_id));
	INF("dst_mac: " MAC_STR_FMT, MAC_STR(stream_params->dst_mac));

	// Initialize talker stream for TX
	avbstream->cur_batch_size = avbstream_batch_size(avbstream->batch_size_ns, stream_params);

	result = avb_stream_create(s_avb_handle, &avbstream->stream_handle, stream_params, &avbstream->cur_batch_size, 0);
	if (result != 0) {
		ERR("create talker stream failed, err %d", result);
		return -1;
	}

	avbstream->clock_avtp = genavb_stream_avtp_clock(avbstream->stream_handle);

	INF("batch: %u, clock_avtp %d", avbstream->cur_batch_size, avbstream->clock_avtp);

	if (stream)
		*stream = avbstream;

	DBG("avb stream created, stream: %p", avbstream);

	return 0;
}

int avbstream_listener_remove(unsigned int unique_id)
{
	aar_avb_stream_t *avbstream;
	int result;

	DBG("unique_id: %d", unique_id);

	avbstream = avbstream_get_listener_stream(unique_id);
	if (!avbstream) {
		ERR("Could not get listener AVB stream unique id %d", unique_id);
		return -1;
	} else if (!avbstream->stream_handle) {
		ERR("Stream (%p) not created.", avbstream);
		return -1;
	}

	// Destroy listener stream
	result = avb_stream_destroy(avbstream->stream_handle);
	if (result != 0) {
		ERR("router remove failed");
		return -1;
	}

	avbstream_put_listener_set(avbstream->set);

	avbstream->stream_handle = NULL;
	avbstream->set = NULL;

	return 0;
}

int avbstream_talker_remove(unsigned int unique_id)
{
	aar_avb_stream_t *avbstream;
	int result;

	DBG("unique_id: %d", unique_id);

	avbstream = avbstream_get_talker_stream(unique_id);
	if (!avbstream) {
		ERR("Could not get talker AVB stream unique id %d", unique_id);
		return -1;
	} else if (!avbstream->stream_handle) {
		ERR("Stream (%p) not created.", avbstream);
		return -1;
	}

	// destroy talker stream
	result = avb_stream_destroy(avbstream->stream_handle);
	if (result != 0) {
		ERR("router remove failed");
		return -1;
	}

	avbstream_put_talker_set(avbstream->set);

	avbstream->stream_handle = NULL;
	avbstream->set = NULL;

	return 0;
}

unsigned int avbstream_get_max_transit_time(aar_avb_stream_t *stream) {
	return sr_class_max_transit_time(stream->stream_params.stream_class) / NSECS_PER_MSEC;
}

struct avb_handle *avbstream_get_avb_handle(void)
{
	return s_avb_handle;
}
