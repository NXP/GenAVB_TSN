/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief Stream handling functions
 @details
*/

#include "config.h"

#include "os/stdlib.h"
#include "os/clock.h"
#include "os/assert.h"

#include "common/log.h"
#include "common/avtp.h"
#include "common/net.h"
#include "common/61883_iidc.h"
#include "common/cvf.h"
#include "common/avdecc.h"
#include "common/clock.h"

#include "stream.h"

#include "61883_iidc.h"
#include "cvf.h"
#include "acf.h"
#include "aaf.h"
#include "crf.h"
#include "media_clock.h"

#define STREAM_DESTROYED_FREE_DELAY_NS			1000000		/* Time delay between destroying and freeing stream */
#define SET_DESTROYED_FREE_DELAY_NS			STREAM_DESTROYED_FREE_DELAY_NS	/* Time delay between destroying and freeing redundant set */
#define STREAM_TALKER_MIN_PRESENTATION_TIME_OFFSET_NS	125000		/* Minimum presentation time offset : 125us */
#define STREAM_TALKER_MAX_PRESENTATION_TIME_OFFSET_NS	50000000	/* Maximun presentation time offset : 50ms */

/* Keep the maximum value of this array <= NET_RX_BATCH */
static const unsigned int sr_class_max_pending_packets[SR_CLASS_MAX + 1] = {
	[SR_CLASS_A] = 8, // 1ms
	[SR_CLASS_B] = 4, // 1ms
	[SR_CLASS_C] = 1,
	[SR_CLASS_D] = 1,
	[SR_CLASS_E] = 1,
	[SR_CLASS_NONE] = 1
};

unsigned int avtp_stream_presentation_offset(struct stream_talker *stream)
{
	if (stream->set->subtype == AVTP_SUBTYPE_CRF)
		return crf_stream_presentation_offset(stream);
	else
		return _avtp_stream_presentation_offset(stream->set->max_transit_time, stream->latency);
}

static int redundant_set_talker_format_init(struct redundant_set_talker *set, struct ipc_avtp_connect *ipc, u64 const *stream_id)
{
	int rc = GENAVB_SUCCESS;

	if (set->stream_count > 1)
		goto out;

	if (set->format.u.s.v != AVTP_VERSION_0) { /* version: 0 describes an AVTP stream payload */
		os_log(LOG_ERR, "set_id(%u) stream_id(%016"PRIx64") version(%u) not supported (should be 0 for AVTP)\n",
				set->id, ntohll(*stream_id), set->format.u.s.v);
		goto err_format;
	}

	/* FIXME subject to improvement to cover also video formats with variable bit rate*/
	if ((set->common.flags & SET_FLAG_CUSTOM_TSPEC) && (ipc->subtype != AVTP_SUBTYPE_TSCF))
		goto err_format;

	switch (ipc->subtype) {
	case AVTP_SUBTYPE_61883_IIDC:
		rc = talker_61883_iidc_check(set, stream_id, ipc);
		break;

	case AVTP_SUBTYPE_TSCF:
		if (!(set->common.flags & SET_FLAG_CUSTOM_TSPEC))
			goto err_format;

		rc = talker_acf_tscf_check(set, stream_id, ipc);
		break;

	case AVTP_SUBTYPE_AAF:
		rc = talker_aaf_check(set, stream_id, ipc);
		break;

	case AVTP_SUBTYPE_CVF:
		rc = talker_cvf_check(set, stream_id, ipc);
		break;
	case AVTP_SUBTYPE_MMA_STREAM:
	case AVTP_SUBTYPE_SVF:
	case AVTP_SUBTYPE_RVF:
	default:
		os_log(LOG_ERR, "set_id(%u) stream_id(%016"PRIx64") subtype(%u) not supported\n", set->id, ntohll(*stream_id), ipc->subtype);
		goto err_format;
	}

out:
	return rc;

err_format:
	return -GENAVB_ERR_STREAM_PARAMS;
}

static int alternative_redundant_set_talker_format_init(struct redundant_set_talker *set, struct ipc_avtp_connect *ipc, u64 const *stream_id)
{
	int rc = GENAVB_SUCCESS;

	if (set->stream_count > 1)
		goto out;

	if ((set->common.flags & SET_FLAG_CUSTOM_TSPEC) && (ipc->subtype != AVTP_SUBTYPE_NTSCF))
		goto err_format;

	switch (ipc->subtype) {
	case AVTP_SUBTYPE_NTSCF:
		if (!(set->common.flags & SET_FLAG_CUSTOM_TSPEC))
			goto err_format;

		rc = talker_acf_ntscf_check(set, stream_id, ipc);
		break;

	case AVTP_SUBTYPE_CRF:
		rc = talker_crf_check(set, stream_id, ipc);
		break;

	default:
		os_log(LOG_ERR, "set_id(%u) stream_id(%016"PRIx64") subtype(%u) not supported\n", set->id, ntohll(*stream_id), ipc->subtype);
		goto err_format;
	}

out:
	return rc;

err_format:
	return -GENAVB_ERR_STREAM_PARAMS;
}


static int redundant_set_listener_format_init(struct redundant_set_listener *set, struct ipc_avtp_connect *ipc, u64 const *stream_id)
{
	int rc = GENAVB_SUCCESS;

	if (set->stream_count > 1)
		goto out;

	if (set->format.u.s.v != AVTP_VERSION_0) { /* version: 0 describes an AVTP stream payload */
		os_log(LOG_ERR, "set_id(%u) stream_id(%016"PRIx64") version(%u) not supported (should be 0 for AVTP)\n",
				set->id, ntohll(*stream_id), set->format.u.s.v);
		goto err_format;
	}

	/* Likely a misconfiguration since only a talker application is expected to specify the streams parameters */
	if (set->common.flags & SET_FLAG_CUSTOM_TSPEC)
		goto err_format;

	switch (ipc->subtype) {
	case AVTP_SUBTYPE_61883_IIDC:
		rc = listener_61883_iidc_check(set, stream_id, ipc);
		break;

	case AVTP_SUBTYPE_CVF:
		rc = listener_cvf_check(set, stream_id, ipc);
		break;

	case AVTP_SUBTYPE_TSCF:
		rc = listener_acf_tscf_check(set, stream_id, ipc);
		break;

	case AVTP_SUBTYPE_AAF:
		rc = listener_aaf_check(set, stream_id, ipc);
		break;

	case AVTP_SUBTYPE_MMA_STREAM:
	case AVTP_SUBTYPE_SVF:
	case AVTP_SUBTYPE_RVF:
	default:
		os_log(LOG_ERR, "set_id(%u) stream_id(%016"PRIx64") subtype(%u) not supported\n", set->id, ntohll(*stream_id), ipc->subtype);
		goto err_format;
	}

out:
	return rc;

err_format:
	return -GENAVB_ERR_STREAM_PARAMS;
}

static int alternative_redundant_set_listener_format_init(struct redundant_set_listener *set, struct ipc_avtp_connect *ipc, u64 const *stream_id)
{
	int rc = GENAVB_SUCCESS;

	if (set->stream_count > 1)
		goto out;

	/* Likely a misconfiguration since only a talker application is expected to specify the streams parameters */
	if (set->common.flags & SET_FLAG_CUSTOM_TSPEC)
		goto err_format;

	switch (ipc->subtype) {
	case AVTP_SUBTYPE_NTSCF:
		rc = listener_acf_ntscf_check(set, stream_id, ipc);
		break;

	case AVTP_SUBTYPE_CRF:
		rc = listener_crf_check(set, stream_id, ipc);
		break;

	default:
		os_log(LOG_ERR, "set_id(%u) stream_id(%016"PRIx64") subtype(%u) not supported\n", set->id, ntohll(*stream_id), ipc->subtype);
		goto err_format;
	}

out:
	return rc;

err_format:
	return -GENAVB_ERR_STREAM_PARAMS;
}

bool stream_check_interrupted(struct stream_listener *stream, u64 gptp_now)
{
	/* At least 100ms elapsed since the last receive, the stream was interrupted */
	if (stream->gptp_last_rx && (gptp_now > (stream->gptp_last_rx + AVTP_RECEIVE_TIMEOUT))) {
		if (!stream->stream_interrupted) {
			stream->sequence_valid = false;

			stream->stream_interrupted = true;

			/* The stream_interruption counter is implementation defined.
			 * We qualify an interrupted stream as a running stream that
			 * received two frames spaced more than AVTP_RECEIVE_TIMEOUT apart.
			 * In that case, AVTP will count it as one interruption until the stream is resumed.
			 */
			stream->stats.stream_interruption++;
		}

		return true;
	}

	return false;
}

void avtp_latency_stats(struct stream_listener *stream, struct avtp_rx_desc *desc)
{
	u32 now = (u32)stream->avtp_current;

	stats_update(&stream->stats.avb_delay, now - desc->desc.ts);
	stats_update(&stream->stats.avtp_delay, desc->avtp_timestamp - now);
}

void stream_talker_stats_print(struct ipc_avtp_talker_stats *msg)
{
	struct redundant_talker_stats *set_stats = &msg->set_stats;
	struct talker_stats *stats = &msg->stats;

	stats_compute(&stats->sched_intvl);

	os_log(LOG_INFO, "stream_id(%016"PRIx64")\n", ntohll(msg->stream_id));

	os_log(LOG_INFO, "rx:     %10u, clock:   %10u, tx: %10u, rx err: %10u, clock err: %10u, gptp err: %10u\n",
	       set_stats->media_rx, stats->clock_rx, stats->tx, set_stats->media_err, stats->clock_err, stats->gptp_err);
	os_log(LOG_INFO, "tx err: %10u, partial: %10u, media underrun: %10u  clock invalid: %10u sched intvl: % 10d/% 10d/% 10d (ns)\n",
		stats->tx_err, stats->partial, set_stats->media_underrun, stats->clock_invalid,
		stats->sched_intvl.min, stats->sched_intvl.mean, stats->sched_intvl.max);
}

static void stream_talker_stats_dump(struct stream_talker *stream, struct ipc_tx *tx)
{
	struct ipc_desc *desc;
	struct ipc_avtp_talker_stats *msg;

	desc = ipc_alloc(tx, sizeof(*msg));
	if (!desc)
		goto err_ipc_alloc;

	desc->type = IPC_AVTP_STREAM_TALKER_STATS;
	desc->len = sizeof(*msg);
	desc->flags = 0;

	msg = (struct ipc_avtp_talker_stats *)&desc->u;

	msg->stream_id = stream->id;
	os_memcpy(&msg->stats, &stream->stats, sizeof(stream->stats));
	os_memcpy(&msg->set_stats, &stream->set->stats, sizeof(stream->set->stats));

	if (ipc_tx(tx, desc) < 0)
		goto err_ipc_tx;

	clock_grid_consumer_stats_dump(&stream->consumer, tx);

	stats_reset(&stream->stats.sched_intvl);
	return;

err_ipc_tx:
	ipc_free(tx, desc);

err_ipc_alloc:
	return;
}

void stream_listener_stats_print(struct ipc_avtp_listener_stats *msg)
{
	struct listener_stats *stats = &msg->stats;

	os_log(LOG_INFO, "stream_id(%016"PRIx64") set(0x%x, %u)\n", ntohll(msg->stream_id), msg->set_id, msg->set_index);

	stats_compute(&stats->avb_delay);
	stats_compute(&stats->avtp_delay);
	stats_compute(&stats->batch);

	os_log(LOG_INFO, "  rx:    %10u, lost:  %10u, subtype err: %10u, subformat err: %10u, gptp err: %10u, clock: %10u\n",
		stats->rx, stats->pkt_lost, stats->subtype_err, stats->format_err, stats->gptp_err, stats->clock_tx);

	os_log(LOG_INFO, "  mr:    %10u, tu:    %10u, interrupted: %10u, ts dropped:    %10u, early:    %10u, late:  %10u\n",
		stats->mr, stats->tu, stats->stream_interruption, stats->ts_dropped, stats->early_timestamp, stats->late_timestamp);

	os_log(LOG_INFO,"  now-rx_ts %4d/%4d/%4d     avtp_ts-now %4d/%4d/%4d (us)     batch %2d/%2d/%2d/%2"PRIu64"\n",
		stats->avb_delay.min/1000, stats->avb_delay.mean/1000, stats->avb_delay.max/1000,
		stats->avtp_delay.min/1000, stats->avtp_delay.mean/1000, stats->avtp_delay.max/1000,
		stats->batch.min, stats->batch.mean, stats->batch.max, stats->batch.variance);

	if (msg->is_redundant) {
		stats_compute(&stats->redundancy_samples);

		os_log(LOG_INFO, "  Redundancy sync:\n");
		os_log(LOG_INFO, "  valid: %10u, skip:  %10u, success:     %10u, lost:          %10u, drop_ts:  %10u\n",
				stats->redundancy_valid, stats->redundancy_skip, stats->redundancy_sync_success, stats->redundancy_sync_lost,
				stats->redundancy_drop_ts);
		os_log(LOG_INFO, "  late:  %10u, early: %10u, ts_mismatch: %10u, undefined      %10u\n",
				stats->redundancy_sync_fail_late, stats->redundancy_sync_fail_early,
				stats->redundancy_sync_fail_ts_mismatch, stats->redundancy_sync_lost_undefined);
		os_log(LOG_INFO, "  valid samples %4d/%4d/%4d/%4"PRIu64"\n",
			stats->redundancy_samples.min, stats->redundancy_samples.mean, stats->redundancy_samples.max, stats->redundancy_samples.variance);
	}
}

void set_listener_stats_print(struct ipc_avtp_set_listener_stats *msg)
{
	struct redundant_listener_stats *set_stats = &msg->stats;

	os_log(LOG_INFO, "set(0x%x)\n", msg->set_id);

	os_log(LOG_INFO, "  media tx: %10u, tx err: %10u, clock: %10u, dropped: %10u, gptp err: %10u\n",
		set_stats->media_tx, set_stats->media_tx_err, set_stats->clock_tx, set_stats->media_tx_dropped, set_stats->gptp_err);

	if (msg->has_redundant_streams) {
		os_log(LOG_INFO, "  Redundancy sync:\n");
		os_log(LOG_INFO, "  success:  %10u, lost:  %10u\n",
			set_stats->sync_success, set_stats->sync_lost);
	}

	if (msg->clock_rec_enabled)
		media_clock_rec_stats_print(&msg->clock_stats);
}

static void stream_listener_stats_dump(struct stream_listener *stream, struct ipc_tx *tx)
{
	struct ipc_avtp_listener_stats *msg;
	struct ipc_desc *desc;
	u64 gptp_now = 0;

	desc = ipc_alloc(tx, sizeof(*msg));
	if (!desc)
		goto err_ipc_alloc;

	if (os_clock_gettime64(stream->clock_gptp, &gptp_now) < 0)
		stream->stats.gptp_err++;

	stream_check_interrupted(stream, gptp_now);

	desc->type = IPC_AVTP_STREAM_LISTENER_STATS;
	desc->len = sizeof(*msg);
	desc->flags = 0;

	msg = (struct ipc_avtp_listener_stats *)&desc->u;

	msg->stream_id = stream->id;
	msg->set_id = stream->set->id;
	msg->set_index = stream->set_index;
	os_memcpy(&msg->stats, &stream->stats, sizeof(stream->stats));

	msg->is_redundant = (stream->set->common.flags & SET_FLAG_HAS_REDUNDANT_STREAM) ? true : false;

	stats_reset(&stream->stats.avb_delay);
	stats_reset(&stream->stats.avtp_delay);
	stats_reset(&stream->stats.batch);

	if (msg->is_redundant)
		stats_reset(&stream->stats.redundancy_samples);

	if (ipc_tx(tx, desc) < 0)
		goto err_ipc_tx;

	return;

err_ipc_tx:
	ipc_free(tx, desc);

err_ipc_alloc:
	return;
}

static void set_listener_stats_dump(struct redundant_set_listener *set, struct ipc_tx *tx)
{
	struct ipc_avtp_set_listener_stats *msg;
	struct ipc_desc *desc;

	desc = ipc_alloc(tx, sizeof(*msg));
	if (!desc)
		goto err_ipc_alloc;


	desc->type = IPC_AVTP_SET_LISTENER_STATS;
	desc->len = sizeof(*msg);
	desc->flags = 0;

	msg = (struct ipc_avtp_set_listener_stats *)&desc->u;

	msg->set_id = set->id;
	os_memcpy(&msg->stats, &set->stats, sizeof(set->stats));

	msg->has_redundant_streams = (set->common.flags & SET_FLAG_HAS_REDUNDANT_STREAM) ? true : false;

	if (set->source) {
		struct clock_grid_producer_stream *producer = &set->source->grid.producer.u.stream;

		if (producer->rec) {
			msg->clock_rec_enabled = true;
			media_clock_rec_stats_dump(producer->rec, &msg->clock_stats);
		}
	} else
		msg->clock_rec_enabled = false;

	if (ipc_tx(tx, desc) < 0)
		goto err_ipc_tx;

	return;

err_ipc_tx:
	ipc_free(tx, desc);

err_ipc_alloc:
	return;
}

void stream_stats_dump(struct avtp_port *port, struct ipc_tx *tx)
{
	struct stream_talker *stream_talker;
	struct stream_listener *stream_listener;
	struct list_head *entry;

	for (entry = list_first(&port->talker); entry != &port->talker; entry = list_next(entry)) {
		stream_talker = container_of(entry, struct stream_talker, common.list);

		stream_talker_stats_dump(stream_talker, tx);
	}

	for (entry = list_first(&port->listener); entry != &port->listener; entry = list_next(entry)) {
		stream_listener = container_of(entry, struct stream_listener, common.list);

		stream_listener_stats_dump(stream_listener, tx);
	}
}

void set_stats_dump(struct avtp_ctx *avtp, struct ipc_tx *tx)
{
	struct redundant_set_listener *set;
	struct list_head *entry;

	for (entry = list_first(&avtp->redundant_set_listener); entry != &avtp->redundant_set_listener; entry = list_next(entry)) {
		set = container_of(entry, struct redundant_set_listener, common.list);

		set_listener_stats_dump(set, tx);
	}
}

/** Adds a stream to the port talker stream list
 *
 * \return		none
 * \param port		pointer to port context
 * \param stream	pointer to talker stream
 */
static void stream_talker_add(struct avtp_port *port, struct stream_talker *stream)
{
	list_add_tail(&port->talker, &stream->common.list);

	stream->common.avtp->stream_talker_count++;
}

/** Searches for a valid redundant context in the avtp redundant set list (based on redundant_set_id).
 *
 * \return			pointer to the matching redundant stream context, NULL it was not found
 * \param port			pointer to avtp global context
 * \param set_id		redundant set's id to match
 */
struct redundant_set_talker *redundant_set_talker_find(struct avtp_ctx *avtp, u16 set_id)
{
	struct redundant_set_talker *set;
	struct list_head *entry;

	for (entry = list_first(&avtp->redundant_set_talker); entry != &avtp->redundant_set_talker; entry = list_next(entry)) {
		set = container_of(entry, struct redundant_set_talker, common.list);

		if (set->id == set_id)
			return set;
	}

	return NULL;
}

/** Searches for a stream in the port talker stream list (based on stream id)
 *
 * \return		pointer to the matching stream, NULL if the stream was not found
 * \param port		pointer to port context
 * \param stream_id	stream id to match
 */
struct stream_talker *stream_talker_find(struct avtp_port *port, void *stream_id)
{
	struct stream_talker *stream;
	struct list_head *entry;

	for (entry = list_first(&port->talker); entry != &port->talker; entry = list_next(entry)) {
		stream = container_of(entry, struct stream_talker, common.list);

		if (cmp_64(&stream->id, stream_id))
			return stream;
	}

	return NULL;
}

/** Calculates transmit batch for the stream
 *
 */
static unsigned int stream_tx_batch(const struct stream_talker *stream)
{
	unsigned int tx_batch = 0;
	unsigned int align_batch, min_batch, avtp_min_batch, max_batch, avtp_max_batch;
	unsigned int samples_per_packet;
	unsigned int packet_rate_p, packet_rate_q;

	switch (stream->set->subtype) {
	case AVTP_SUBTYPE_AAF:
	case AVTP_SUBTYPE_61883_IIDC:
	case AVTP_SUBTYPE_CVF:
	case AVTP_SUBTYPE_TSCF:
	case AVTP_SUBTYPE_NTSCF:
		samples_per_packet = stream->frames_per_packet;

		/* align wakeup period to whole packets */
		align_batch = stream->frames_per_interval;

		/* Set wakeup period so that generated packet number, per period, is less than half the maximum supported transmit burst */
		max_batch = (STREAM_TX_BATCH / align_batch) * align_batch;

		packet_rate_p = stream->sample_rate;
		packet_rate_q = samples_per_packet;

		min_batch = align_batch;

		avtp_min_batch = ((u64)CFG_AVTP_MIN_LATENCY * packet_rate_p + (u64)packet_rate_q * NSECS_PER_SEC - 1) / ((u64)packet_rate_q * NSECS_PER_SEC);

		avtp_max_batch = ((u64)CFG_AVTP_MAX_LATENCY * packet_rate_p) / ((u64)packet_rate_q * NSECS_PER_SEC);
		avtp_max_batch = (avtp_max_batch / align_batch) * align_batch;

		while (min_batch < avtp_min_batch)
			min_batch += align_batch;

		if (max_batch > avtp_max_batch)
			max_batch = avtp_max_batch;

		/* For high sample rates/low samples per packet or low sample rates/high samples per packet */
		if (min_batch > max_batch) {
			os_log(LOG_ERR, "invalid latency/batch target min_batch: %d, max_batch: %d, latency_min_batch: %d, latency_max_batch: %d\n", min_batch, max_batch, avtp_min_batch, avtp_max_batch);
			goto err;
		}

		tx_batch = ((u64)stream->latency * packet_rate_p) / ((u64)packet_rate_q * NSECS_PER_SEC);
		tx_batch = (tx_batch / align_batch) * align_batch;

		if (tx_batch < min_batch)
			tx_batch = min_batch;
		else if (tx_batch > max_batch)
			tx_batch = max_batch;

		break;

	case AVTP_SUBTYPE_CRF:
		tx_batch = CRF_TX_BATCH;
		break;

	default:
		goto err;
		break;
	}

	os_assert(tx_batch <= NET_TX_BATCH);

	return tx_batch;
err:
	return 0;
}

int stream_clock_consumer_enable(struct stream_talker *stream)
{
	unsigned int ts_freq_p, ts_freq_q, packet_freq_p, packet_freq_q;
	unsigned int wake_freq_p, wake_freq_q;
	unsigned int ps;

	if (!(stream->set->common.flags & SET_FLAG_CLOCK_GENERATION))
		return 0;

	ts_freq_p = stream->sample_rate;

	if (stream->set->subtype == AVTP_SUBTYPE_CRF)
		ts_freq_p *= 2;

	ts_freq_q = stream->samples_per_timestamp;

	if (!ts_freq_p || !ts_freq_q) {
		os_log(LOG_ERR, "talker(%p) invalid ts_freq: %u/%u\n", stream, ts_freq_p, ts_freq_q);
		return -1;
	}

	packet_freq_p = stream->sample_rate;
	packet_freq_q = stream->frames_per_packet;

	stream->time_per_packet = ((u64)NSECS_PER_SEC * packet_freq_q) / packet_freq_p;

	stream->latency = stream->tx_batch * stream->time_per_packet;

	wake_freq_p = packet_freq_p;
	wake_freq_q = packet_freq_q * stream->tx_batch;

	if (!wake_freq_p || !wake_freq_q) {
		os_log(LOG_ERR, "talker(%p) invalid wake_freq: %u/%u\n", stream, wake_freq_p, wake_freq_q);
		return -1;
	}

	os_log(LOG_INFO, "talker(%p) media_rx(%p) frames per packet %u, payload size %u, syt interval %u, syt freq %u, latency %u, batch %u\n",
		stream, &stream->set->media, stream->frames_per_packet, stream->payload_size,
		stream->samples_per_timestamp, ts_freq_p / ts_freq_q, stream->latency, stream->tx_batch);

	/* Align on media clock grid */
	/* IEEE 1722-2016 4.3.5 and 10.8 */
	/* n x Ps - Ps / 20 < Toffset < n x Ps + Ps / 20. n > 0 */
	/* In practice we just round up to a multiple of Ps = (NSECS_PER_SEC / sample_rate) */
	ps = (NSECS_PER_SEC + stream->sample_rate / 2) / stream->sample_rate;

	if (clock_domain_init_consumer(stream->domain, &stream->consumer, avtp_stream_presentation_offset(stream),
		ts_freq_p, ts_freq_q, ps, sr_class_prio(stream->set->class)) < 0)
		goto err_init;

	if (stream->set->subtype == AVTP_SUBTYPE_CRF) {
		if (os_timer_create(&stream->subtype_data.crf.t, stream->domain->source->clock_id, 0, &crf_os_timer_handler, stream->priv) < 0)
			goto err_timer_create;

		if (os_timer_start(&stream->subtype_data.crf.t, 0, wake_freq_p, wake_freq_q, 0) < 0)
			goto err_timer_start;
	} else {
		if (clock_domain_init_consumer_wakeup(stream->domain, &stream->consumer, wake_freq_p, wake_freq_q) < 0)
			goto err_init_wakeup;
	}

	stream->consumer_enabled = true;

	return 0;

err_timer_start:
	os_timer_destroy(&stream->subtype_data.crf.t);

err_timer_create:
err_init_wakeup:
	clock_domain_exit_consumer(&stream->consumer);

err_init:
	return -1;
}

void stream_clock_consumer_disable(struct stream_talker *stream)
{
	if ((stream->set->common.flags & SET_FLAG_CLOCK_GENERATION) && (stream->consumer_enabled)) {
		if (stream->set->subtype == AVTP_SUBTYPE_CRF)
			os_timer_destroy(&stream->subtype_data.crf.t);
		else
			clock_domain_exit_consumer_wakeup(&stream->consumer);

		clock_domain_exit_consumer(&stream->consumer);

		stream->consumer_enabled = false;
	}
}

/** Initializes redundant set's common runtime attributes according to its first stream's attributes.
 *
 * \return			0 upon initializing the redundant set, -1 in case of an error.
 * \param set			pointer to the stream's redundant context.
 * \param ipc			pointer to the ipc connect message (with all the stream parameters).
 * \param stream_id		stream's id. Used to init the media channel for non redundant stream, otherwise the set's id is used.
 * \param stream_header_len	stream's header_len.
 * \param stream_latency	stream's latency.
 */
static int redundant_set_talker_init(struct redundant_set_talker *set, struct ipc_avtp_connect *ipc,
				    u64 *stream_id, unsigned int stream_header_len, unsigned int stream_latency)
{
	unsigned int flags = 0;
	u64 media_id;

	if (set->stream_count > 1)
		goto out;

	if (set->init)
		set->init(set);

	if (set->common.flags & SET_FLAG_MEDIA_WAKEUP)
		flags |= MEDIA_FLAG_WAKEUP;

	if (!(set->common.flags & SET_FLAG_NO_MEDIA)) {
		media_id = set->id;

		if (media_rx_init(&set->media, &media_id, set->common.avtp->priv, flags, stream_header_len, stream_presentation_offset(set->max_transit_time, stream_latency)) < 0)
			goto err;
	}

	os_log(LOG_INFO, "set(0x%x, %p) initialized\n", set->id, set);

out:
	return 0;

err:
	return -1;
}

/** Clears the redundant context before destroying it when removing its last stream from the set.
 * Closes the media channel attached to the redundant context.
 *
 * \return			none
 * \param set			pointer to the stream's redundant context.
 */
static void redundant_set_talker_exit(struct redundant_set_talker *set)
{
	if (set->stream_count > 1)
		goto out;

	/* Close media tx channel when removing the last stream of the redundant set. */
	if (!(set->common.flags & SET_FLAG_NO_MEDIA))
		media_rx_exit(&set->media);

	if (set->exit)
		set->exit(set);

out:
	return;
}

/** Checks that the stream's attributes match its associated redundant set's attributes,
 * if they do, the stream can belong to the redundant set.
 *
 * \return			true if the stream's attributes match the redundant set's ones, false otherwise.
 * \param avtp			pointer to the global AVTP context.
 * \param set			pointer to the redundant set context.
 * \param ipc			pointer to the ipc connect message (with all the stream parameters).
 * \param stream_transit_time	stream's max_transit_time.
 */
static bool redundant_set_talker_check(struct avtp_ctx *avtp, struct redundant_set_talker *set, struct ipc_avtp_connect *ipc, unsigned int stream_transit_time)
{
	bool rc = true;

	if ((set->common.avtp != avtp) ||
	    (set->max_transit_time != stream_transit_time) ||
	    (set->subtype != ipc->subtype) ||
	    os_memcmp(&set->format, &ipc->format, sizeof(struct avdecc_format)) ||
	    os_memcmp(&set->class, &ipc->stream_class, sizeof(sr_class_t))) {
		os_log(LOG_ERR, "stream_id(%016"PRIx64"): avtp(%p) format(%016"PRIx64") subtype(%u) class(%d) max_transit_time(%u)\n",
			get_ntohll(ipc->stream_id), avtp, get_ntohll(ipc->format.u.raw), ipc->subtype, ipc->stream_class, stream_transit_time);
		os_log(LOG_ERR, "set(0x%x, %p): avtp(%p) format(%016"PRIx64") subtype(%u) class(%d) max_transit_time(%u)\n",
			set->id, set, set->common.avtp, get_ntohll(set->format.u.raw), set->subtype, set->class, set->max_transit_time);

		rc = false;
	}

	return rc;
}

/** Allocates a talker stream's redundant set.
 * Or, if it exists, check that the new stream's attributes match the redundant set,
 * and if it does, count the new stream in the redundant set.
 *
 * \return			pointer to the common redundant context or NULL if it's invalid.
 * \param avtp			pointer to the global AVTP context.
 * \param ipc			pointer to the ipc connect message (with all the stream parameters).
 * \param stream_entry		pointer to the stream's list_head, to add it in the list of streams included in the redundant set.
 */
static struct redundant_set_talker *redundant_set_talker_create(struct avtp_ctx *avtp, struct ipc_avtp_connect *ipc, struct list_head *stream_entry)
{
	struct redundant_set_talker *set;
	unsigned int stream_transit_time;
	u16 set_id = ipc->set_id;

	if (ipc->flags & IPC_AVTP_FLAGS_MAX_TRANSIT_TIME_VALID) {
		if ((ipc->talker.max_transit_time < STREAM_TALKER_MIN_PRESENTATION_TIME_OFFSET_NS) ||
		    (ipc->talker.max_transit_time > STREAM_TALKER_MAX_PRESENTATION_TIME_OFFSET_NS)) {
			os_log(LOG_ERR, "stream_id(%016"PRIx64"), stream max_transit_time(%u) out of range [%u, %u]\n",
				get_ntohll(ipc->stream_id), ipc->talker.max_transit_time, STREAM_TALKER_MIN_PRESENTATION_TIME_OFFSET_NS, STREAM_TALKER_MAX_PRESENTATION_TIME_OFFSET_NS);
			goto err;
		}

		stream_transit_time = ipc->talker.max_transit_time;
	} else {
		stream_transit_time = sr_class_max_transit_time(ipc->stream_class);
	}

	/* Re-use the same redundant context if possible. */
	set = redundant_set_talker_find(avtp, set_id);
	if (set) {
		/* Redundant set for static stream */
		if (!(ipc->flags & IPC_AVTP_FLAGS_SET_ID_VALID)) {
			os_log(LOG_ERR, "talker static stream(%016"PRIx64")'s set(0x%x) conflicts with another already existing set(0x%x, %p)\n",
				get_ntohll(ipc->stream_id), set_id, set_id, set);
			goto err;
		}

		/* Redundant set with a single non redundant stream */
		if (!(ipc->flags & IPC_AVTP_FLAGS_REDUNDANT_STREAM)) {
			os_log(LOG_ERR, "set(0x%x) should only contain a single non redundant stream but another set(0x%x, %p) with the same set_id exists\n",
				set_id, set_id, set);
			goto err;
		}

		if (redundant_set_talker_check(avtp, set, ipc, stream_transit_time))
			goto out;
		else
			goto err;
	}

	/* New redundant set */
	set = os_malloc(sizeof(*set));
	if (!set)
		goto err;

	os_memset(set, 0, sizeof(*set));

	set->id = set_id;
	set->common.avtp = avtp;
	set->format = ipc->format;
	set->subtype = ipc->subtype;
	set->class = ipc->stream_class;
	set->max_transit_time = stream_transit_time;

	set->common.flags |= SET_FLAG_SR | SET_FLAG_VLAN;

	/* Redundant set has actual redundant streams and not only a single non redundant stream */
	if (ipc->flags & IPC_AVTP_FLAGS_REDUNDANT_STREAM) {
		os_log(LOG_ERR, "set(0x%x, %p): Milan redundancy not supported for Talkers\n",
				set->id, set);
		goto err_redundancy;
	}

	/* Stream params inherited from the application (i.e. not retrieved from AVDDECC format) */
	if (ipc->flags & GENAVB_STREAM_FLAGS_CUSTOM_TSPEC)
		set->common.flags |= SET_FLAG_CUSTOM_TSPEC;

	list_head_init(&set->talker_stream);

	/* Add the newly created and initialized redundant set to the list. */
	list_add_tail(&set->common.avtp->redundant_set_talker, &set->common.list);

	os_log(LOG_INFO, "set(0x%x, %p) created\n", set->id, set);

out:
	list_add_tail(&set->talker_stream, stream_entry);

	set->stream_count++;

	return set;

err_redundancy:
	os_free(set);
err:
	return NULL;
}

/** Upon a talker stream being destroyed, removes it from its redundant set.
 * The redundant set, if not referencing any more streams, is destroyed.
 *
 * \return			none
 * \param set			pointer to the redundant set context.
 * \param stream_entry		pointer to the stream's list_head, to remove it from the list of streams included in the redundant set.
 * \param immediate_free	If true, redundant context memory can be freed immediately. Otherwise, schedule for deferred free.
 */
static void redundant_set_talker_destroy(struct redundant_set_talker *set, struct list_head *stream_entry, bool immediate_free)
{
	set->stream_count--;

	list_del(stream_entry);

	if (set->stream_count == 0) {
		/* Clean redundant context if it doesn't reference any more streams */
		list_del(&set->common.list);

		os_log(LOG_INFO, "set(0x%x, %p) destroyed\n", set->id, set);

		if (immediate_free)
			os_free(set);
		else
			list_add_tail(&set->common.avtp->redundant_set_destroyed, &set->common.list);
	}
}

/** Creates a talker stream context
 *
 * Allocates memory for the stream context and initializes handles to media stack, media clock capture and network layers
 *
 * \return	pointer to created stream or NULL if the stream couldn't be created/already exists.
 * \param avtp	pointer to avtp global context
 * \param ipc	ipc connect message (with all the stream parameters)
 */
struct stream_talker *stream_talker_create(struct avtp_ctx *avtp, struct avtp_port *port, struct ipc_avtp_connect *ipc)
{
	struct redundant_set_talker *set;
	struct stream_talker *stream;
	unsigned int hdr_len = 0;
	struct net_address addr;
	int rc;

	if (!sr_class_enabled(ipc->stream_class)) {
		os_log(LOG_ERR, "stream_id(%016"PRIx64") class(%d) is invalid\n", get_ntohll(ipc->stream_id), ipc->stream_class);
		goto err_class_invalid;
	}

	stream = stream_talker_find(port, &ipc->stream_id);
	if (stream) {
		os_log(LOG_ERR, "stream_id(%016"PRIx64") destroying previously existing stream\n", get_ntohll(ipc->stream_id));
		stream_talker_destroy(stream, NULL);
	}

	stream = os_malloc(sizeof(*stream));
	if (!stream)
		goto err_alloc_stream;

	os_memset(stream, 0, sizeof(*stream));

	set = redundant_set_talker_create(avtp, ipc, &stream->set_entry);
	if (!set)
		goto err_alloc_redundant_set;

	stream->set = set;
	stream->set_index = ipc->set_index;

	copy_64(&stream->id, &ipc->stream_id);

	/*
	* Initialize stream parameters and check format - hdr_len is an output
	*/
	if (is_avtp_stream(ipc->subtype))
		rc = redundant_set_talker_format_init(set, ipc, &stream->id);
	else if (is_avtp_alternative(ipc->subtype))
		rc = alternative_redundant_set_talker_format_init(set, ipc, &stream->id);
	else
		rc = -1;

	if (rc < 0) {
		os_log(LOG_ERR, "stream(%016"PRIx64", %p) set(0x%x, %p) stream format check failed: rc = %d\n", ntohll(stream->id), stream, set->id, set, rc);
		goto err_format;
	}

	stream->direction = AVTP_DIRECTION_TALKER;

	stream->media_count = 0;

	stream->port = ipc->port;

	stream->clock_gptp = port->clock_gptp;
	stream->clock_avtp = avtp_to_clock_avtp(stream->port);
	stream->needs_avtp_ts_conversion = os_clock_is_match(stream->clock_gptp, stream->clock_avtp) ? false : true;

	stream->latency = ipc->talker.latency;

	stream->consumer_enabled = false;

	stream->common.avtp = avtp;

	stream->priv = avtp->priv;

	os_memset(stream->header_template, 0, HEADER_TEMPLATE_SIZE);

	stream->header_len = net_add_eth_header(stream->header_template, ipc->dst_mac, ETHERTYPE_VLAN);

	/* Vlan id and priority are overriden by the network layer */
	stream->header_len += net_add_vlan_header(stream->header_template + stream->header_len, ETHERTYPE_AVTP, 0, 0, 0);

	stream->avtp_hdr = (struct avtp_data_hdr *)(stream->header_template + stream->header_len);

	if (set->common.flags & SET_FLAG_CUSTOM_TSPEC) {
		if (avtp_fmt_sample_size(ipc->subtype, &ipc->format))
			stream->frames_per_packet = ipc->talker.max_frame_size / avtp_fmt_sample_size(ipc->subtype, &ipc->format);
		else
			stream->frames_per_packet = ipc->talker.max_frame_size;

		stream->payload_size = ipc->talker.max_frame_size;

		stream->frames_per_interval = ipc->talker.max_interval_frames;

		stream->sample_rate = ((u64)stream->payload_size * NSECS_PER_SEC * sr_class_interval_q(ipc->stream_class)) / sr_class_interval_p(ipc->stream_class);

		stream->samples_per_timestamp = samples_per_interval(stream->sample_rate, ipc->stream_class);
	} else {
		stream->frames_per_packet = __avdecc_fmt_samples_per_packet(&ipc->format, ipc->stream_class, &stream->frames_per_interval);

		stream->payload_size = stream->frames_per_packet * avdecc_fmt_sample_stride(&ipc->format);

		stream->sample_rate = avdecc_fmt_sample_rate(&ipc->format);

		stream->samples_per_timestamp = avdecc_fmt_samples_per_timestamp(&ipc->format, ipc->stream_class);
	}

	if (set->stream_init)
		set->stream_init(stream, &hdr_len);

	stream->header_len += hdr_len;

	rc = redundant_set_talker_init(set, ipc, &stream->id, stream->header_len, stream->latency);
	if (rc < 0) {
		os_log(LOG_ERR, "set(0x%x, %p) init failed\n", set->id, set);
		goto err_set_init;
	}

	addr.ptype = PTYPE_AVTP;
	addr.port = ipc->port;
	addr.vlan_id = ipc->talker.vlan_id;
	addr.priority = ipc->talker.priority;
	addr.u.avtp.subtype = ipc->subtype;
	addr.u.avtp.sr_class = ipc->stream_class;
	copy_64(addr.u.avtp.stream_id, &stream->id);

	if (net_tx_init(&stream->tx, &addr) < 0)
		goto err_tx_init;

	stream->tx_batch = stream_tx_batch(stream);
	if (!stream->tx_batch)
		goto err_tx_batch;

	stream->domain = clock_domain_get(avtp, ipc->clock_domain);
	if (!stream->domain) {
		os_log(LOG_ERR, "stream(%016"PRIx64", %p) set(0x%x, %p) clock_domain_get(%d) failed\n", ntohll(stream->id), stream, set->id, set, ipc->clock_domain);
		goto err_clock_domain;
	}

	stream->locked_count = stream->domain->locked_count;

	/* Legacy support, set the domain source internally */
	if (ipc->clock_domain < GENAVB_CLOCK_DOMAIN_0) {
		if (clock_domain_set_source_legacy(stream->domain, avtp, ipc) < 0)
			goto err_clock_domain;
	}

	if (stream_clock_consumer_enable(stream) < 0)
		goto err_clock_enable;

	stats_init(&stream->stats.sched_intvl, 31, NULL, NULL);

	if (os_clock_gettime64(stream->clock_gptp, &stream->gptp_current) < 0)
		stream->stats.gptp_err++;

	stream_talker_add(port, stream);

	os_log(LOG_INFO, "talker_stream_id(%016"PRIx64") set(0x%x) set_index(%u) class(%d) format(%016"PRIx64") domain(%p) domain_id(%d) needs_avtp_ts_conversion(%d)\n",
		ntohll(stream->id), set->id, stream->set_index, stream->set->class, get_ntohll(set->format.u.raw), stream->domain, stream->domain->id,
		stream->needs_avtp_ts_conversion);

	return stream;

err_clock_enable:
err_clock_domain:
err_tx_batch:
	net_tx_exit(&stream->tx);

err_tx_init:
	redundant_set_talker_exit(set);

err_set_init:
	if (set->stream_exit)
		set->stream_exit(stream);

err_format:
	redundant_set_talker_destroy(set, &stream->set_entry, true);

err_alloc_redundant_set:
	os_free(stream);

err_alloc_stream:
err_class_invalid:
	return NULL;
}

/** Destroys a talker stream context
 *
 * \return		none
 * \param stream	pointer to talker stream to destroy
 */
void stream_talker_destroy(struct stream_talker *stream, struct ipc_tx *tx)
{
	os_log(LOG_INFO, "talker stream(%016"PRIx64", %p) set(0x%x, %p)\n", ntohll(stream->id), stream, stream->set->id, stream->set);

	if (stream->set->stream_exit)
		stream->set->stream_exit(stream);

	if (tx)
		stream_talker_stats_dump(stream, tx);

	stream_clock_consumer_disable(stream);

	net_tx_exit(&stream->tx);

	redundant_set_talker_exit(stream->set);

	redundant_set_talker_destroy(stream->set, &stream->set_entry, false);

	list_del(&stream->common.list);
	list_add_tail(&stream->common.avtp->stream_destroyed, &stream->common.list);

	stream->common.avtp->stream_talker_count--;
}

/** Adds a stream to the port listener stream list
 *
 * \return		none
 * \param port		pointer to port context
 * \param stream	pointer to listener stream
 */
static void stream_listener_add(struct avtp_port *port, struct stream_listener *stream)
{
	list_add_tail(&port->listener, &stream->common.list);

	stream->common.avtp->stream_listener_count++;
}

/** Searches for a valid redundant context in the avtp redundant set list (based on redundant_set_id).
 *
 * \return			pointer to the matching redundant stream context, NULL it was not found
 * \param port			pointer to avtp global context
 * \param set_id		redundant set's id to match
 */
struct redundant_set_listener *redundant_set_listener_find(struct avtp_ctx *avtp, u16 set_id)
{
	struct redundant_set_listener *set;
	struct list_head *entry;

	for (entry = list_first(&avtp->redundant_set_listener); entry != &avtp->redundant_set_listener; entry = list_next(entry)) {
		set = container_of(entry, struct redundant_set_listener, common.list);

		if (set->id == set_id)
			return set;
	}

	return NULL;
}

/** Searches for a stream in the port listener stream list (based on stream id)
 *
 * \return		pointer to the matching stream, NULL if the stream was not found
 * \param port		pointer to port context
 * \param stream_id	stream id to match
 */
struct stream_listener *stream_listener_find(struct avtp_port *port, void *stream_id)
{
	struct stream_listener *stream;
	struct list_head *entry;

	for (entry = list_first(&port->listener); entry != &port->listener; entry = list_next(entry)) {
		stream = container_of(entry, struct stream_listener, common.list);

		if (cmp_64(&stream->id, stream_id))
			return stream;
	}

	return NULL;
}


static unsigned int stream_rx_batch(const struct stream_listener *stream, unsigned int *latency)
{
	unsigned int rx_batch;

	/* FIXME, take into account media queue batch size */

	switch (stream->set->subtype) {
	case AVTP_SUBTYPE_AAF:
	case AVTP_SUBTYPE_61883_IIDC:
	case AVTP_SUBTYPE_CVF:
		rx_batch = sr_class_max_pending_packets[stream->set->class];
		*latency = (rx_batch * sr_class_interval_p(stream->set->class)) / sr_class_interval_q(stream->set->class);
		break;

	case AVTP_SUBTYPE_TSCF:
	case AVTP_SUBTYPE_NTSCF:
	case AVTP_SUBTYPE_CRF:
		rx_batch = CRF_RX_BATCH;
		*latency = CRF_RX_LATENCY;
		break;

	default:
		rx_batch = 0;
		break;
	}

	return rx_batch;
}

/** Initializes redundant set's common runtime attributes according to its first stream's attributes.
 *
 * \return			0 upon initializing the redundant set, -1 in case of an error.
 * \param set			pointer to the stream's redundant context.
 * \param ipc			pointer to the ipc connect message (with all the stream parameters).
 * \param stream_id		stream's id. Used to init the media channel for non redundant stream, otherwise the set's id is used.
 */
static int redundant_set_listener_init(struct redundant_set_listener *set, struct ipc_avtp_connect *ipc, u64 *stream_id)
{
	struct avtp_ctx *avtp = set->common.avtp;
	u64 media_id;

	if (set->stream_count > 1)
		goto out;

	if (set->init)
		if (set->init(set) < 0)
			goto err_init;

	if (!(set->common.flags & SET_FLAG_NO_MEDIA)) {
		media_id = set->id;

		if (media_tx_init(&set->media, &media_id) < 0)
			goto err_media;
	}

	set->domain = clock_domain_get(avtp, ipc->clock_domain);
	if (!set->domain) {
		os_log(LOG_ERR, "set(0x%x, %p) stream_id(%016"PRIx64"), clock_domain_get(%d) failed\n",
			set->id, set, ntohll(*stream_id), ipc->clock_domain);
		goto err_clock;
	}

	/* Legacy support, set the domain source internally */
	if (ipc->clock_domain < GENAVB_CLOCK_DOMAIN_0) {
		if (ipc->flags & IPC_AVTP_FLAGS_REDUNDANT_STREAM) {
			os_log(LOG_ERR, "set(0x%x, %p) stream_id(%016"PRIx64") legacy domain(%d) not supported with redundant streams.\n",
				set->id, set, ntohll(*stream_id), ipc->clock_domain);
			goto err_clock;
		}

		if (clock_domain_set_source_legacy(set->domain, avtp, ipc) < 0)
			goto err_clock;
	}

	/* Setup stream source (new and legacy clock API) if domain has this set as source */
	if ((ipc->flags & IPC_AVTP_FLAGS_MCR)
	&& (clock_domain_is_source_set(set->domain, set->id, stream_id))) {
		if (__clock_domain_update_source(set->domain, set->domain->source, set) < 0) {
			os_log(LOG_ERR, "set(0x%x, %p) stream_id(%016"PRIx64"), __clock_domain_update_source failed\n",
				set->id, set, ntohll(*stream_id));
			goto err_clock;
		}
	}

	/*
	 * FIXME, we should really fail at this stage.
	 * Need to fix AVDECC/entities
	 */
	if (set->source) {
		os_log(LOG_INFO, "stream set is a clock source\n");

		if (set->source->grid.producer.u.stream.rec)
			os_log(LOG_INFO, "MCR enabled\n");
		else
			os_log(LOG_INFO, "MCR disabled\n");
	}

	set->merge.sync_state = 0;

	os_log(LOG_INFO, "set(0x%x, %p): initialized\n", set->id, set);

out:
	return 0;

err_clock:
	if (!(set->common.flags & SET_FLAG_NO_MEDIA))
		media_tx_exit(&set->media);
err_media:
err_init:
	return -1;
}

/** Clears the redundant context before destroying it when removing its last stream from the set.
 * Closes the media channel attached to the redundant context.
 *
 * \return			none
 * \param set			pointer to the stream's redundant context.
 */
static void redundant_set_listener_exit(struct redundant_set_listener *set)
{
	if (set->stream_count > 1)
		goto out;

	if (set->source)
		clock_source_close(set->source);

	/* Close media tx channel when removing the last stream of the redundant set. */
	if (!(set->common.flags & SET_FLAG_NO_MEDIA))
		media_tx_exit(&set->media);

	if (set->exit)
		set->exit(set);

out:
	return;
}

/** Checks that the stream's attributes match its associated redundant set's attributes,
 * if they do, the stream can belong to the redundant set.
 *
 * \return			true if the stream's attributes match the redundant set's ones, false otherwise.
 * \param avtp			pointer to the global AVTP context.
 * \param set			pointer to the redundant set context.
 * \param ipc			pointer to the ipc connect message (with all the stream parameters).
 */
static bool redundant_set_listener_check(struct avtp_ctx *avtp, struct redundant_set_listener *set, struct ipc_avtp_connect *ipc)
{
	bool rc = true;

	if ((set->common.avtp != avtp) ||
	    (set->subtype != ipc->subtype) ||
	    os_memcmp(&set->format, &ipc->format, sizeof(struct avdecc_format)) ||
	    os_memcmp(&set->class, &ipc->stream_class, sizeof(sr_class_t))) {
		os_log(LOG_ERR, "stream_id(%016"PRIx64"): avtp(%p) format(%016"PRIx64") subtype(%u) class(%d)\n",
			get_ntohll(ipc->stream_id), avtp, get_ntohll(ipc->format.u.raw), ipc->subtype, ipc->stream_class);
		os_log(LOG_ERR, "set(0x%x, %p): avtp(%p) format(%016"PRIx64") subtype(%u) class(%d)\n",
			set->id, set, set->common.avtp, get_ntohll(set->format.u.raw), set->subtype, set->class);

		rc = false;
	}

	if (!os_clock_is_match(set->clock_avtp, avtp_to_clock_avtp(ipc->port))) {
		os_log(LOG_ERR, "set(0x%x, %p) and stream_id(%016"PRIx64") have different avtp clocks\n",
			set->id, set, get_ntohll(ipc->stream_id));
		rc = false;
	}

	return rc;
}

/** Allocates a listener stream's redundant set.
 * Or, if it exists, check that the new stream's attributes match the redundant set,
 * and if it does, count the new stream in the redundant set.
 *
 * \return			pointer to the common redundant context or NULL if it's invalid.
 * \param avtp			pointer to the global AVTP context.
 * \param ipc			pointer to the ipc connect message (with all the stream parameters).
 */
static struct redundant_set_listener *redundant_set_listener_create(struct avtp_ctx *avtp, struct ipc_avtp_connect *ipc)
{
	struct redundant_set_listener *set;
	u16 set_id = ipc->set_id;

	/* Only valid redundant streams can re-use the same redundant context. */
	set = redundant_set_listener_find(avtp, set_id);
	if (set) {
		/* Redundant set for static stream */
		if (!(ipc->flags & IPC_AVTP_FLAGS_SET_ID_VALID)) {
			os_log(LOG_ERR, "listener static stream(%016"PRIx64")'s set(0x%x) conflicts with another already existing set(0x%x, %p)\n",
				get_ntohll(ipc->stream_id), set_id, set_id, set);
			goto err;
		}

		/* Redundant set with a single non redundant stream */
		if (!(ipc->flags & IPC_AVTP_FLAGS_REDUNDANT_STREAM)) {
			os_log(LOG_ERR, "set(0x%x) should only contain a single non redundant stream but another set(0x%x, %p) with the same set_id exists\n",
				set_id, set_id, set);
			goto err;
		}

		if (redundant_set_listener_check(avtp, set, ipc))
			goto out;
		else
			goto err;
	}

	/* New redundant set */
	set = os_malloc(sizeof(*set));
	if (!set)
		goto err;

	os_memset(set, 0, sizeof(*set));

	set->id = set_id;
	set->common.avtp = avtp;
	set->format = ipc->format;
	set->subtype = ipc->subtype;
	set->class = ipc->stream_class;
	set->clock_avtp = avtp_to_clock_avtp(ipc->port);

	/* Redundant set has actual redundant streams and not only a single non redundant stream */
	if (ipc->flags & IPC_AVTP_FLAGS_REDUNDANT_STREAM) {

		if (!avdecc_fmt_supports_milan_redundancy(&set->format, set->class)) {

			os_log(LOG_ERR, "set(0x%x, %p): format(%016"PRIx64") class(%d) do not support Milan redundancy\n",
					set->id, set, get_ntohll(set->format.u.raw), set->class);
			goto err_redundancy;
		}

		set->common.flags |= SET_FLAG_HAS_REDUNDANT_STREAM;

		if (ipc->subtype == AVTP_SUBTYPE_CRF) {
			/* The common avdecc functions assume that one CRF sample is one timestamp. While, the CRF merge
			 * algorithm assumes samples are media events carried in the timestamps_interval.
			 * So, fixup both samples_per_packet (should be timestamps_per_pdu x timestamp_interval) and sample rate for CRF.
			 */
			set->merge.sample_rate = avdecc_fmt_sample_rate(&ipc->format) * AVDECC_FMT_CRF_TIMESTAMP_INTERVAL(&ipc->format);
			set->merge.samples_per_packet = __avdecc_fmt_samples_per_packet(&ipc->format, ipc->stream_class, NULL) * AVDECC_FMT_CRF_TIMESTAMP_INTERVAL(&ipc->format);
		} else {
			set->merge.samples_per_packet = __avdecc_fmt_samples_per_packet(&ipc->format, ipc->stream_class, NULL);
			set->merge.sample_rate = avdecc_fmt_sample_rate(&ipc->format);
			/* sample_stride is only used for non-CRF (media based) streams */
			set->merge.u.media.sample_stride = avdecc_fmt_sample_stride(&ipc->format);
		}

		set->merge.timestamp_tolerance = (u64)NSECS_PER_SEC / (4ULL * set->merge.sample_rate); /* 25% sample time */
		set->merge.sample_time = (u64)NSECS_PER_SEC / set->merge.sample_rate;

		os_log(LOG_INFO, "set(0x%x, %p) supports redundancy merge: sample_time (%u ns) samples_per_packet (%u) sample_rate(%u) tolerance(%u)\n",
			set->id, set, set->merge.sample_time, set->merge.samples_per_packet, set->merge.sample_rate,
			set->merge.timestamp_tolerance);

	}

	/* Add the newly created redundant set to the list. */
	list_add_tail(&avtp->redundant_set_listener, &set->common.list);

	os_log(LOG_INFO, "set(0x%x, %p) created\n", set->id, set);

out:
	set->stream_count++;

	return set;

err_redundancy:
	os_free(set);
err:
	return NULL;
}

/** Upon a listener stream being destroyed, removes it from its redundant set.
 * The redundant set, if not referencing any more streams, is destroyed.
 *
 * \return			none
 * \param set			pointer to the redundant set context.
 * \param immediate_free	If true, redundant context memory can be freed immediately. Otherwise, schedule for deferred free.
 * \param tx			If not null, the ipc channel to be used for dumping the set stats.
 */
static void redundant_set_listener_destroy(struct redundant_set_listener *set, bool immediate_free, struct ipc_tx *tx)
{
	set->stream_count--;

	if (set->stream_count == 0) {
		list_del(&set->common.list);

		os_log(LOG_INFO, "set(0x%x, %p) destroyed\n", set->id, set);

		if (tx)
			set_listener_stats_dump(set, tx);

		if (immediate_free)
			os_free(set);
		else
			list_add_tail(&set->common.avtp->redundant_set_destroyed, &set->common.list);
	}
}

/** Creates a listener stream context
 *
 * Allocates memory for the stream context and initializes handles to media stack, media clock recovery and network layers
 *
 * \return	pointer to created stream or NULL if the stream couldn't be created.
 * \param avtp	pointer to avtp global context
 * \param ipc	ipc connect message (with all the stream parameters)
 */
struct stream_listener *stream_listener_create(struct avtp_ctx *avtp, struct avtp_port *port, struct ipc_avtp_connect *ipc)
{
	struct redundant_set_listener *set;
	unsigned int rx_batch, rx_latency;
	struct stream_listener *stream;
	struct net_address addr;
	int rc;

	if (!sr_class_enabled(ipc->stream_class)) {
		os_log(LOG_ERR, "stream_id(%016"PRIx64") class(%d) is invalid\n", get_ntohll(ipc->stream_id), ipc->stream_class);
		goto err_class_invalid;
	}

	stream = stream_listener_find(port, ipc->stream_id);
	if (stream) {
		os_log(LOG_ERR, "stream_id(%016"PRIx64") destroying previously existing stream\n", get_ntohll(ipc->stream_id));
		stream_listener_destroy(stream, NULL);
	}

	stream = os_malloc(sizeof(*stream));
	if (!stream)
		goto err_alloc_stream;

	os_memset(stream, 0, sizeof(*stream));

	set = redundant_set_listener_create(avtp, ipc);
	if (!set)
		goto err_alloc_redundant_set;

	stream->set = set;
	stream->set_index = ipc->set_index;

	/* Clear the stream sync status to avoid any residual previous state. */
	redundant_set_stream_unsynced(set, stream->set_index);

	copy_64(&stream->id, ipc->stream_id);

	/*
	* Initialize stream parameters and check format
	*/
	if (is_avtp_stream(ipc->subtype))
		rc = redundant_set_listener_format_init(set, ipc, &stream->id);
	else if (is_avtp_alternative(ipc->subtype))
		rc = alternative_redundant_set_listener_format_init(set, ipc, &stream->id);
	else
		rc = -1;

	if (rc < 0) {
		os_log(LOG_ERR, "stream(%016"PRIx64", %p) set(0x%x, %p) stream format check failed: rc = %d\n", ntohll(stream->id), stream, set->id, set, rc);
		goto err_format;
	}

	stream->max_timing_uncertainty = sr_class_max_timing_uncertainty(ipc->stream_class);

	stream->max_transit_time = sr_class_max_transit_time(ipc->stream_class);

	stream->direction = AVTP_DIRECTION_LISTENER;

	stream->stream_interrupted = true;

	stream->sequence_valid = false;

	stream->common.avtp = avtp;

	if (set->stream_init)
		if ((rc = set->stream_init(stream)) < 0)
			goto err_stream_init;


	rc = redundant_set_listener_init(set, ipc, &stream->id);
	if (rc < 0) {
		os_log(LOG_ERR, "set(0x%x, %p) init failed\n", set->id, set);
		goto err_set_init;
	}

	addr.ptype = PTYPE_AVTP;
	addr.port = ipc->port;
	addr.u.avtp.subtype = ipc->subtype;
	addr.u.avtp.sr_class = set->class;
	copy_64(addr.u.avtp.stream_id, &stream->id);

	rx_batch = stream_rx_batch(stream, &rx_latency);
	if (!rx_batch)
		goto err_rx_batch;

	if (is_avtp_stream(ipc->subtype))
		rc = net_rx_init_multi(&stream->rx, &addr, &avtp_stream_net_rx, rx_batch, rx_latency, avtp->priv);
	else
		rc = net_rx_init_multi(&stream->rx, &addr, &avtp_alternative_net_rx, rx_batch, rx_latency, avtp->priv);

	if (rc < 0)
		goto err_net;

	os_memcpy(stream->dst_mac, ipc->dst_mac, 6);
	stream->port = ipc->port;

	if (net_add_multi(&stream->rx, stream->port, stream->dst_mac) < 0) {
		os_log(LOG_ERR, "stream(%016"PRIx64", %p) set(0x%x, %p) cannot register MC address\n", ntohll(stream->id), stream, set->id, set);
		goto err_multi;
	}

	stream->clock_gptp = port->clock_gptp;
	stream->clock_avtp = avtp_to_clock_avtp(stream->port);
	stream->needs_avtp_ts_conversion = os_clock_is_match(stream->clock_gptp, stream->clock_avtp) ? false : true;

	stats_init(&stream->stats.avb_delay, 31, NULL, NULL);
	stats_init(&stream->stats.avtp_delay, 31, NULL, NULL);
	stats_init(&stream->stats.batch, 31, NULL, NULL);
	stats_init(&stream->stats.redundancy_samples, 31, NULL, NULL);

	stream_listener_add(port, stream);

	os_log(LOG_INFO, "listener stream(%016"PRIx64", %p) set(0x%x) set_index(%u) class(%d) format(%016"PRIx64") domain(%p) domain_id(%d) needs_avtp_ts_conversion(%d)\n",
		ntohll(stream->id), stream, set->id, stream->set_index, set->class, get_ntohll(set->format.u.raw), set->domain,
		set->domain->id, stream->needs_avtp_ts_conversion);

	return stream;

err_multi:
	net_rx_exit(&stream->rx);

err_net:
err_rx_batch:
	redundant_set_listener_exit(set);

err_set_init:
err_stream_init:
err_format:
	redundant_set_listener_destroy(set, true, NULL);

err_alloc_redundant_set:
	os_free(stream);

err_alloc_stream:
err_class_invalid:
	return NULL;
}

/** Destroys a listener stream context
 *
 * \return		none
 * \param stream	pointer to listener stream to destroy
 */
void stream_listener_destroy(struct stream_listener *stream, struct ipc_tx *tx)
{
	os_log(LOG_INFO, "listener stream(%016"PRIx64", %p) set(0x%x, %p)\n", ntohll(stream->id), stream, stream->set->id, stream->set);

	if (tx)
		stream_listener_stats_dump(stream, tx);

	net_del_multi(&stream->rx, stream->port, stream->dst_mac);

	net_rx_exit(&stream->rx);

	redundant_set_listener_exit(stream->set);

	redundant_set_listener_destroy(stream->set, false, tx);

	list_del(&stream->common.list);
	list_add_tail(&stream->common.avtp->stream_destroyed, &stream->common.list);

	stream->common.avtp->stream_listener_count--;
}

void redundant_set_free_all(struct avtp_ctx *avtp)
{
	struct redundant_set_common *set;
	struct list_head *entry;

	entry = list_first(&avtp->redundant_set_destroyed);
	while (entry != &avtp->redundant_set_destroyed) {
		set = container_of(entry, struct redundant_set_common, list);
		entry = list_next(entry);

		list_del(&set->list);
		os_free(set);
	}
}

void stream_free_all(struct avtp_ctx *avtp)
{
	struct stream_common *stream;
	struct list_head *entry;

	entry = list_first(&avtp->stream_destroyed);
	while (entry != &avtp->stream_destroyed) {
		stream = container_of(entry, struct stream_common, list);
		entry = list_next(entry);

		list_del(&stream->list);
		os_free(stream);
	}
}

void avtp_redundant_set_free(void *avtp_ctx, u64 current_time)
{
	struct avtp_ctx *avtp = (struct avtp_ctx*)avtp_ctx;
	struct redundant_set_common *set;
	struct list_head *entry;

	entry = list_first(&avtp->redundant_set_destroyed);
	while (entry != &avtp->redundant_set_destroyed) {
		set = container_of(entry, struct redundant_set_common, list);
		entry = list_next(entry);

		if (!(set->flags & SET_FLAG_DESTROYED)) {
			set->flags |= SET_FLAG_DESTROYED;
			set->destroy_time = current_time;
		} else if ((current_time - set->destroy_time) >= SET_DESTROYED_FREE_DELAY_NS) {
			list_del(&set->list);

			os_log(LOG_INFO, "set(%p)\n", set);

			os_free(set);
		}
	}
}

void avtp_stream_free(void *avtp_ctx, u64 current_time)
{
	struct avtp_ctx *avtp = (struct avtp_ctx*)avtp_ctx;
	struct stream_common *stream;
	struct list_head *entry;

	entry = list_first(&avtp->stream_destroyed);
	while (entry != &avtp->stream_destroyed) {
		stream = container_of(entry, struct stream_common, list);
		entry = list_next(entry);

		if (!(stream->flags & STREAM_FLAG_DESTROYED)) {
			stream->flags |= STREAM_FLAG_DESTROYED;
			stream->destroy_time = current_time;
		} else if ((current_time - stream->destroy_time) >= STREAM_DESTROYED_FREE_DELAY_NS) {
			list_del(&stream->list);

			os_log(LOG_INFO, "stream(%p)\n", stream);

			os_free(stream);
		}
	}
}

int stream_media_rx(struct stream_talker *stream, struct media_rx_desc **media_desc_array, unsigned int *flags, unsigned int *alignment_ts)
{
	u32 i;
	int rc;
	struct media_rx_desc *media_desc;
	unsigned int do_align = 0, underrun = 0, tx_batch;

	/* Get samples from media stack. If running behind (late > 0), try to read one extra packet to progressively recover. */
	if (stream->late)
		tx_batch = stream->tx_batch + STREAM_TX_LATE_EXTRA_MEDIA;
	else
		tx_batch = stream->tx_batch;

	if (unlikely((rc = media_rx(&stream->set->media, media_desc_array, tx_batch)) < 0)) {
		stream->set->stats.media_err++;
//		os_log(LOG_ERR, "talker(%p) media.rx failed\n", stream);
		goto media_rx_fail;
	}

	/* Keep track of late packets to trigger underrun condition
	 * Only trigger underrun if we have some received some data.
	 * "Late" is relative to number of "number of wakeups" x "tx_batch",
	 * we increment it each time we read less than stream->tx_batch packets from the stack,
	 * we decrement it each time we read more */
	if ((stream->media_count != 0) || (rc != 0))
		stream->late += stream->tx_batch - (unsigned int)rc;

	stream->set->stats.media_rx += rc;

	for (i = 0; i < rc; i++) {
		media_desc = media_desc_array[i];
		if (media_desc->ts_n) {
			do_align = 1;
			stream->late = 0;
			*alignment_ts = media_desc->avtp_ts[0].val; // Take the first timestamp of the first packet with timestamps
			//FIXME if the timestamp is not for the first byte of the first packet, this can add an error of up to batch size
			break;
		}
	}

	if ((stream->late > stream->tx_batch) && rc) {
		stream->set->stats.media_underrun++;
		stream->late = 0;
		underrun = 1;
	}

	if (unlikely((((stream->media_count == 0) && rc) || underrun || stream_domain_phase_change(stream)) && !do_align)) {
		do_align = 1;
		*alignment_ts = (u32)stream->avtp_current + avtp_stream_presentation_offset(stream);
	}

	if (unlikely(do_align))
		*flags |= MCG_FLAGS_DO_ALIGN;

	return rc;

media_rx_fail:
	stream->late = 0;

	return rc;
}


int stream_tx_flow_control(struct stream_talker *stream, unsigned int *tx_batch)
{
	unsigned int tx_avail;
	int rc = 0;

	/* check the amount of free space in the network transmit queue */
	tx_avail = net_tx_available(&stream->tx);

	if (tx_avail < *tx_batch) {
		/* transmit is congested, enable flow control if not yet done */
		if(!stream->tx_event_enabled) {
			/*disable media queue events so we stop dequeuing data from the media interface */
			media_rx_event_disable(&stream->set->media);

			/* register event notification from the network interface to signal
			the avtp thread when there is free space again in the transmit queue */
			net_tx_event_enable(&stream->tx, stream->priv);

			stream->tx_event_enabled = 1;

			/* if the transmit queue is full, we do not transmit any data at this round,
			else we just transmit what can be transmitted */
			if (!tx_avail)
				rc = -1;
			else
				*tx_batch = tx_avail;
		} else
			/* we are already in flow controlled state, nothing should be transmitted */
			rc = -1;
	} else {
		/* network transmit queue is not congested (at least one batch of free slots in the queue), make sure flow control is now disabled */
		if (stream->tx_event_enabled) {
			/* disable network queue events, i.e. do not listen anymore to the event from network interface */
			net_tx_event_disable(&stream->tx);

			/* enable media queue events and allow dequeuing from the media interface */
			media_rx_event_enable(&stream->set->media);

			/* back to normal operation, no restriction on transmit */
			stream->tx_event_enabled = 0;
		}
	}

	os_log(LOG_DEBUG, "stream(%p) tx_batch %u tx_avail %u flow_control_enabled %u rc %d\n", stream, *tx_batch, tx_avail, stream->tx_event_enabled, rc);

	return rc;
}

static inline void stream_adjust_desc_samples(struct stream_listener *stream, struct media_desc *desc, unsigned int sample_offset)
{
	unsigned int size = stream->set->merge.u.media.sample_stride * sample_offset;

	desc->l2_offset += size;
	desc->len -= size;

	/* Adjust the timestamp as well (if valid)*/
	if (!(desc->avtp_ts[0].flags & AVTP_FLAGS_TO_MEDIA_DESC(AVTP_TIMESTAMP_INVALID)))
		desc->avtp_ts[0].val += (u32)SAMPLE_OFFSET_NS(stream->set->merge.sample_time, sample_offset);
}

/* Synchronize an unsynced redundant set with its first stream.
 * returns 0 if packet is valid to be sent
 * returns -1 if packet to be freed
 */
static int stream_redundant_listener_sync_first(struct stream_listener *stream, struct media_desc *desc, u8 current_seq,
						 u64 packet_ts, bool packet_ts_valid)
{
	struct redundant_set_listener *set = stream->set;
	u16 set_index = stream->set_index;
	int rc = 0;

	if (packet_ts_valid) {
		set->merge.last_ts = packet_ts + SAMPLE_OFFSET_NS(set->merge.sample_time, set->merge.samples_per_packet - 1);
	} else {
		/* Unable to sync without a valid timestamp, drop the packet (FIXME Should we send and let the upper layer decide?). */
		stream->stats.redundancy_drop_ts++;
		set->stats.drop_ts++;

		stats_update(&stream->stats.redundancy_samples, 0);
		rc = -1;
		goto exit;
	}

	set->merge.next_seq[set_index] = current_seq + 1;
	set->merge.next_sample_offset[set_index] = 0;
	redundant_set_stream_synced(set, set_index);

	stream->stats.redundancy_valid++;
	stats_update(&stream->stats.redundancy_samples, set->merge.samples_per_packet);

	stream->stats.redundancy_sync_success++;

	set->stats.sync_success++;

	os_log(LOG_DEBUG, "set(0x%x, %p): first synchronized with stream(0x%x, %p): packet_ts(%"PRIu64") last_ts(%"PRIu64"), next expected seq_num(%u)\n",
		set->id, set, set_index, stream, packet_ts, set->merge.last_ts, set->merge.next_seq[set_index]);

exit:
	return rc;
}

/* Synchronize a redundant listener stream to an already synced set.
 * returns 0 if packet is valid and to be sent to media queue
 * returns -1 if packet to be freed
 */
static int stream_redundant_listener_sync_to_set(struct stream_listener *stream, struct media_desc *desc, u8 current_seq,
						 u64 packet_ts, bool packet_ts_valid)
{
	struct redundant_set_listener *set = stream->set;
	u16 set_index = stream->set_index;
	u32 sample_offset;
	int rc = 0;

	if (!packet_ts_valid) {
		/* Unable to sync without a valid timestamp, drop the packet. */
		stream->stats.redundancy_drop_ts++;

		stats_update(&stream->stats.redundancy_samples, 0);
		rc = -1;
		goto exit;
	}

	if (packet_ts < set->merge.last_ts - (u64)SAMPLE_OFFSET_NS(set->merge.sample_time, set->merge.samples_per_packet - 1) - set->merge.timestamp_tolerance) {
		/* Last sample is at least one packet after this packet. */
		stats_update(&stream->stats.redundancy_samples, 0);
		stream->stats.redundancy_sync_fail_late++;
		rc = -1;
		goto exit;
	} else if ((packet_ts >= set->merge.last_ts
					- (u64)SAMPLE_OFFSET_NS(set->merge.sample_time, set->merge.samples_per_packet - 1)
					- set->merge.timestamp_tolerance)
		    && (packet_ts <= set->merge.last_ts + set->merge.timestamp_tolerance) ) {

		/* Last valid sample is somewhere inside the current packet. */

		u32 to_send;

		/* Get the last valid sample's index inside the this packet: should be in [0 .. samples_per_packet - 1] */
		if (get_sample_index(packet_ts, set->merge.last_ts, set->merge.sample_time, &sample_offset) < 0) {
			stream->stats.redundancy_sync_fail_ts_mismatch++;

			stats_update(&stream->stats.redundancy_samples, 0);
			rc = -1;
			goto exit;
		}

		/* Get the number of samples to be sent from this packet. */
		to_send = set->merge.samples_per_packet - (sample_offset + 1);
		if (to_send) {
			stream_adjust_desc_samples(stream, desc, (sample_offset + 1));
			stream->stats.redundancy_valid++;
			redundant_set_increment_synced_other(set, set_index, to_send);
		} else {
			/* Last valid sample matched the last sample of the packet:
			 * Nothing to send. Just sync the stream and free the buffer.
			 */
			rc = -1;
		}

		stats_update(&stream->stats.redundancy_samples, to_send);
		set->merge.next_seq[set_index] = current_seq + 1;
		redundant_set_stream_synced(set, set_index);
		/* In all cases, the very next sample from this stream is the next valid one. */
		set->merge.next_sample_offset[set_index] = 0;

		os_log(LOG_DEBUG, "set(0x%x, %p) stream(0x%x, %p): synchronized with last sent sample: "
				 "packet ts (%"PRIu64"), last_ts (%"PRIu64") (sample_offset %"PRIu32") next expected seq_num(%u)\n",
				  set->id, set, set_index, stream, packet_ts, set->merge.last_ts, sample_offset,
				  set->merge.next_seq[set_index]);

		set->merge.last_ts = packet_ts + SAMPLE_OFFSET_NS(set->merge.sample_time, set->merge.samples_per_packet - 1);
		stream->stats.redundancy_sync_success++;
	} else {
		/* This packet has future samples. */

		if (get_sample_index(set->merge.last_ts + set->merge.sample_time, packet_ts,
					set->merge.sample_time, &sample_offset)) {
			stream->stats.redundancy_sync_fail_ts_mismatch++;

			stats_update(&stream->stats.redundancy_samples, 0);
			rc = -1;
			goto exit;
		}

		if (sample_offset > 0) {
			/* Packet does not start with the very next sample, too early. */
			stream->stats.redundancy_sync_fail_early++;
			os_log(LOG_DEBUG, "set(0x%x, %p) stream(0x%x, %p): seq(%u): last_ts (%"PRIu64") packet_ts (%"PRIu64") sample_offset(%"PRIu32")\n",
					  set->id, set, set_index, stream, current_seq, set->merge.last_ts, packet_ts, sample_offset);

			stats_update(&stream->stats.redundancy_samples, 0);
			rc = -1;
			goto exit;
		} else {
			/* Packet starts with the very next sample. Send it in totality */

			set->merge.next_seq[set_index] = current_seq + 1;
			redundant_set_stream_synced(set, set_index);

			stream->stats.redundancy_valid++;
			stats_update(&stream->stats.redundancy_samples, set->merge.samples_per_packet);

			redundant_set_increment_synced_other(set, set_index, set->merge.samples_per_packet);
			set->merge.next_sample_offset[set_index] = 0;
			os_log(LOG_DEBUG, "set(0x%x, %p) stream(0x%x, %p): synchronized with next sample: "
					 "packet ts (%"PRIu64"), last_ts (%"PRIu64") next expected seq_num(%u)\n",
					  set->id, set, set_index, stream, packet_ts, set->merge.last_ts,
					  set->merge.next_seq[set_index]);

			set->merge.last_ts = packet_ts + SAMPLE_OFFSET_NS(set->merge.sample_time, set->merge.samples_per_packet - 1);
			stream->stats.redundancy_sync_success++;
		}
	}

exit:
	return rc;
}

void stream_media_audio_tx_merge(struct stream_listener *stream, struct media_desc **desc, unsigned int *n)
{
	struct redundant_set_listener *set = stream->set;
	struct media_desc *valid_desc[NET_RX_BATCH];
	u16 set_index = stream->set_index;
	unsigned int valid_desc_n = 0;
	u8 current_seq;
	int i;

	if (!*n)
		goto out;

	/* sequence_num is the last sequence number of the batch, deduce first packet's sequence number. */
	current_seq = (u8)(stream->sequence_num - (u8)*n + 1);

	/* If we had a sequence number discontinuity (appears only on the first packet of the batch),
	 * unsync the stream right away.
	 */
	if (desc[0]->flags & AVTP_PACKET_LOST) {
		/* Lost sync for this stream:
		 * - If no other redundant stream is synced: invalidate the set and start over.
		 * - If there is another redundant stream synced: invalidate only this stream and try to sync.
		 */

		redundant_set_stream_unsynced(set, set_index);

		if (!is_redundant_set_synced(set)) {
			set->stats.sync_lost++;
		} else {
			/* Declare lost packet only when there is no other synced stream. */
			desc[0]->flags &= ~(AVTP_PACKET_LOST);
		}

		stream->stats.redundancy_sync_lost++;
	}

	for (i = 0; i < *n; i++, current_seq++) {
		bool packet_ts_valid = !(desc[i]->avtp_ts[0].flags & AVTP_FLAGS_TO_MEDIA_DESC(AVTP_TIMESTAMP_INVALID));
		u64 packet_ts = packet_ts_valid ? genavb_avtp_ts_32to64(desc[i]->avtp_ts[0].val, stream->avtp_current) : 0;

#if defined(STREAM_AUDIO_MERGE_DEBUG)
		os_log(LOG_DEBUG, "set(0x%x, %p, sync_state 0x%x, last_ts (%"PRIu64")) stream(0x%x, %p, next_seq %u next_sample_offset %u) "
				  "packet: [%u/%u] seq_num(%u) ts (%"PRIu64") \n",
				  set->id, set, set->merge.sync_state, set->merge.last_ts,
				  set_index, stream, set->merge.next_seq[set_index], set->merge.next_sample_offset[set_index],
				  i, *n, current_seq, packet_ts);
#endif

		if (unlikely(!is_redundant_set_synced(set))) {
			/* Neither of streams is synced, take the packet as is. */

				if (!stream_redundant_listener_sync_first(stream, desc[i], current_seq, packet_ts, packet_ts_valid)) {
					valid_desc[valid_desc_n++] = desc[i];
				} else {
					net_rx_free((struct net_rx_desc *)desc[i]);
					continue; /* Go to next packet. */
				}
		} else {
			/* We have, at least, one synced stream and we already sent data to media queue. */
			if (is_redundant_stream_synced(set, set_index)) {
				/* This stream is already synced. */
				if (set->merge.next_seq[set_index] == current_seq) {
					unsigned int to_send = set->merge.samples_per_packet - set->merge.next_sample_offset[set_index];

					/* This is the next packet expected for the set. Send the right number of samples to media queue. */

					stream_adjust_desc_samples(stream, desc[i], set->merge.next_sample_offset[set_index]);

					valid_desc[valid_desc_n++] = desc[i];
					stream->stats.redundancy_valid++;
					stats_update(&stream->stats.redundancy_samples, to_send);

					set->merge.next_seq[set_index] = set->merge.next_seq[set_index] + 1;
					set->merge.next_sample_offset[set_index] = 0;

					/* Valid packet: set last_ts and increment expected next seq for all synced streams. */
					if (packet_ts_valid) {
						set->merge.last_ts = packet_ts + SAMPLE_OFFSET_NS(set->merge.sample_time, set->merge.samples_per_packet - 1);
					} else {
						/* FIXME Redundant streams should always have valid timestamps in the packet anyway */
						set->merge.last_ts += SAMPLE_OFFSET_NS(set->merge.sample_time, set->merge.samples_per_packet);
					}

					redundant_set_increment_synced_other(set, set_index, to_send);

					continue; /* Go to next packet. */
				} else {
					/* This packet is not the next expected: Either we already sent equivalent or we lost sync. */
					if (seq_number_after(set->merge.next_seq[set_index], current_seq)) {
						/* The current sequence number is behind the next valid packet for this
						 * stream (packet already sent by another redundant stream).
						 */
						stream->stats.redundancy_skip++;
						stats_update(&stream->stats.redundancy_samples, 0);
						net_rx_free((struct net_rx_desc *)desc[i]);
						continue; /* Go to next packet. */
					} else {
						/* Something went wrong:
						 * - There is no reported lost packet (AVTP_PACKET_LOST) for this stream
						 * - But it may be that it fell too much behind the next valid packet
						 */
						redundant_set_stream_unsynced(set, set_index);

						if (!is_redundant_set_synced(set))
							set->stats.sync_lost++;

						stream->stats.redundancy_sync_lost_undefined++;
						stream->stats.redundancy_sync_lost++;
						net_rx_free((struct net_rx_desc *)desc[i]);
						continue; /* Go to next packet. */
					}
				}
			} else {
				/* This stream is not synced yet */
				if (!stream_redundant_listener_sync_to_set(stream, desc[i], current_seq, packet_ts, packet_ts_valid)) {
					valid_desc[valid_desc_n++] = desc[i];
				} else {
					net_rx_free((struct net_rx_desc *)desc[i]);
					continue; /* Go to next packet. */
				}
			}
		}
	}

	if (valid_desc_n)
		os_memcpy(desc, valid_desc, valid_desc_n * sizeof(struct media_desc *));

	*n = valid_desc_n;

out:
	return;
}
