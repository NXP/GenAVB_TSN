/*
 * Copyright 2017-2018, 2020, 2022, 2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <time.h>
#include <string.h>
#include <sys/epoll.h>

#include "log.h"
#include "alsa_stream.h"
#include "alsa2.h"
#include "avb_stream.h"
#include "stats.h"
#include "stream_stats.h"
#include "thread.h"

void alsa_stats_dump(struct alsa_stream_stats *stats)
{
	aar_alsa_counter_stats_t *alsa_counter_stats = &stats->alsa_stats.counter_stats;

	stream_stats_dump(&stats->gen_stats);

	stats_compute(&stats->alsa_stats.alsa_avail_samples);

	INF_LOG("   alsa(%p) dev %d, dir %d, available samples %4d/%4d/%4d (frames)",
		(void *)stats->alsa_handle_ptr, stats->alsa_device,
		stats->alsa_direction,
		stats->alsa_stats.alsa_avail_samples.min,
		stats->alsa_stats.alsa_avail_samples.mean,
		stats->alsa_stats.alsa_avail_samples.max);

	if (stats->gen_stats.is_listener) {
		stats_compute(&stats->alsa_stats.alsa_latency);
		INF_LOG("   alsa(%p) latency %4d/%4d/%4d (us)",
			(void *)stats->alsa_handle_ptr,
			stats->alsa_stats.alsa_latency.min,
			stats->alsa_stats.alsa_latency.mean,
			stats->alsa_stats.alsa_latency.max);
	}

	// Also print ALSA counter stats
	INF_LOG("   alsa(%p) tx_err: %d, rx_err: %d, period_tx: %d, period_rx: %d (frames)",
		(void *)stats->alsa_handle_ptr, alsa_counter_stats->tx_err,
		alsa_counter_stats->rx_err, alsa_counter_stats->period_tx,
		alsa_counter_stats->period_rx);

	if (stats->gen_stats.is_listener) {
		INF_LOG("   alsa(%p) tx_start: %d, tx_start_err: %d, tx_start_drop: %d, tx_start_no_data: %d",
			(void *)stats->alsa_handle_ptr, alsa_counter_stats->tx_start, alsa_counter_stats->tx_start_err,
			alsa_counter_stats->tx_start_drop, alsa_counter_stats->tx_start_no_data);
	}
}


static void alsa_stats_store(struct alsa_stream *stream)
{
	struct alsa_stream_stats *stats = &stream->stats;
	aar_alsa_stats_t *alsa_stats = &stream->alsa_handle.stats;
	aar_avb_stats_t *avb_stats = &stream->avb_stream[stream->current_set_index]->set->stats;

	// Ignore if statistic handle is updated but not printed
	if (stream_stats_is_updated(&stats->gen_stats)) {
		ERR("ALSA stream(%p) Store stats failed", stream);
	} else {
		memcpy(&stats->alsa_stats, alsa_stats, sizeof(aar_alsa_stats_t));
		stream_stats_store(&stats->gen_stats, avb_stats);

		// Reset statistics
		stats_reset(&alsa_stats->alsa_avail_samples);
		stats_reset(&alsa_stats->alsa_latency);
	}
}

int talker_alsa_handler(void *data, unsigned int events)
{
	struct alsa_stream *talker = (struct alsa_stream *)data;
	aar_alsa_handle_t *alsa_handle = &talker->alsa_handle;
	aar_avb_stream_t *avb_stream = talker->avb_stream[0];
	pthread_mutex_t *lock = &talker->thread_slot->slot_lock;
	int result = -1;

	/* Lock the slot */
	pthread_mutex_lock(lock);

	if (stream_stats_is_time(&talker->stats.gen_stats))
		alsa_stats_store(talker);

	result = alsa_rx(alsa_handle, avb_stream);

	pthread_mutex_unlock(lock);

	return result;
}

int talker_alsa_connect(struct alsa_stream *talker, struct avb_stream_params *params)
{
	int trigger_fd;

	if (talker->stream_count)
		goto err_params;

	if (!params) {
		ERR("Error: ALSA stream(%p, %u) Invalid AVB stream params\n", talker, talker->index);
		goto err_params;
	}

	/* Use a one to one mapping between stream index and input device */
	talker->alsa_handle.device = talker->index;
	talker->alsa_handle.direction = AAR_DATA_DIR_INPUT;

	if (alsa_rx_init(&talker->alsa_handle, params, talker->alsa_device) < 0)
		goto err_alsa;

	params->clock_domain = AVB_CLOCK_DOMAIN_0;

	if (avbstream_talker_add(talker->index, params, &talker->avb_stream[0]) < 0)
		goto err_stream;

#if 0
	trigger_fd = avb_stream_fd(talker->avb_stream->stream_handle);
	if (thread_slot_add(THR_CAP_STREAM_TALKER | THR_CAP_ALSA | THR_CAP_STREAM_AUDIO, trigger_fd, EPOLLOUT, talker, talker_alsa_handler, &talker->thread_slot) < 0)
#else
	if (alsa_get_fd_from_handle(&talker->alsa_handle, &trigger_fd) < 0)
		goto err_fd;

	if (thread_slot_add(THR_CAP_STREAM_TALKER | THR_CAP_ALSA | THR_CAP_STREAM_AUDIO, trigger_fd, EPOLLIN, talker, talker_alsa_handler, NULL, 0, &talker->thread_slot) < 0)
#endif
		goto err_thread;

	alsa_rx_start(&talker->alsa_handle, talker->avb_stream[0]);

	talker->stream_count = 1;
	talker->current_set_index = 0;

	return 0;

err_thread:
err_fd:
	avbstream_talker_remove(talker->index);

err_stream:
	alsa_rx_exit(&talker->alsa_handle);

err_alsa:
err_params:
	return -1;
}

void talker_alsa_disconnect(struct alsa_stream *talker)
{
	if (talker->stream_count) {
		thread_slot_free(talker->thread_slot);

		alsa_rx_exit(&talker->alsa_handle);

		avbstream_talker_remove(talker->index);

		talker->stream_count = 0;
	}
}

int listener_alsa_handler(void *data, unsigned int events)
{
	struct alsa_stream *listener = (struct alsa_stream *)data;
	pthread_mutex_t *lock = &listener->thread_slot->slot_lock;
	aar_alsa_handle_t *alsa_handle = &listener->alsa_handle;
	aar_avb_stream_t *avb_stream;
	int result = -1;

	/* Lock the alsa stream to access the avb_stream */
	pthread_mutex_lock(&listener->lock);

	avb_stream = listener->avb_stream[listener->current_set_index];

	/* Lock the slot */
	pthread_mutex_lock(lock);

	if (stream_stats_is_time(&listener->stats.gen_stats))
		alsa_stats_store(listener);

	result = alsa_tx(alsa_handle, avb_stream);

	pthread_mutex_unlock(lock);
        pthread_mutex_unlock(&listener->lock);

	return result;
}


int listener_alsa_connect(struct alsa_stream *listener, unsigned int avdecc_index, struct avb_stream_params *params)
{
	int trigger_fd;

	if (!params) {
		ERR("Error: ALSA stream(%p, %u) Invalid AVB stream params\n", listener, listener->index);
		goto err_init;
	}

	if (params->set_index >= MAX_REDUNDANT_STREAMS) {
		ERR("Error: ALSA stream(%p, %u) set_index(%u) exceed max number of redundant streams(%u)\n",
		    listener, listener->index, params->set_index, MAX_REDUNDANT_STREAMS);
		goto err_init;
	}

	if (!listener->stream_count) {
		listener->set_id = params->set_id;

		if (params->flags & GENAVB_STREAM_FLAGS_REDUNDANT)
			listener->flags |= ALSA_STREAM_FLAG_HAS_REDUNDANT;
	} else {
		if (listener->stream_count >= MAX_REDUNDANT_STREAMS) {
			ERR("Error: ALSA stream(%p, %u) already connected to max number of streams(%u)\n",
			    listener, listener->index, MAX_REDUNDANT_STREAMS);
			goto err_init;
		}

		if (params->flags & GENAVB_STREAM_FLAGS_REDUNDANT) {
			if (!(listener->flags & ALSA_STREAM_FLAG_HAS_REDUNDANT)) {
				ERR("Error: ALSA stream(%p, %u) already connected to non-redundant stream\n",
				    listener, listener->index);
				goto err_init;
			} else if (listener->set_id != params->set_id) {
				ERR("Error: ALSA stream(%p, %u) already connected to a different set ID (%u vs expected %u)\n",
				    listener, listener->index, params->set_id, listener->set_id);
				goto err_init;
			}
		}
	}

	/* Alsa init on first stream connection. */
	if (!listener->stream_count) {
		/* Use a one to one mapping between stream index and output device */
		listener->alsa_handle.device = listener->index;
		listener->alsa_handle.direction = AAR_DATA_DIR_OUTPUT;

		if (alsa_tx_init(&listener->alsa_handle, params, listener->alsa_device) < 0)
			goto err_alsa;
	}

	params->clock_domain = AVB_CLOCK_DOMAIN_0;

	if (avbstream_listener_add(avdecc_index, params, &listener->avb_stream[params->set_index]) < 0)
		goto err_stream;

	if (!listener->stream_count) {
		/* First stream of the set: use it as current index (no need to lock the mutex here) */
		listener->current_set_index = params->set_index;

		trigger_fd = avb_stream_fd(listener->avb_stream[params->set_index]->stream_handle);
		if (thread_slot_add(THR_CAP_STREAM_LISTENER | THR_CAP_ALSA | THR_CAP_STREAM_AUDIO, trigger_fd, EPOLLIN,
				    listener, listener_alsa_handler, NULL, 0, &listener->thread_slot) < 0)
			goto err_thread;
	}

	listener->stream_count++;
	listener->stream_mask |= (1 << params->set_index);

	return 0;

err_thread:
	avbstream_listener_remove(listener->index);

err_stream:
	alsa_tx_exit(&listener->alsa_handle);

err_alsa:
err_init:
	return -1;
}

void listener_alsa_disconnect(struct alsa_stream *listener, unsigned int avdecc_index, int set_index)
{
	if (!listener->stream_count)
		return;

	listener->stream_count--;
	listener->stream_mask &= ~(1 << set_index);

	if (listener->stream_count == 0) {
		thread_slot_free(listener->thread_slot);

		alsa_tx_exit(&listener->alsa_handle);
	} else {
		/* If the current stream used for API access is being disconnected, switch to another
		 * stream of the set to keep streaming going.
		 * NOTE: The accesses to current_set_index in the control thread on connect/disconnect() are serialized by
		 * design (a single control thread), so no need to lock for read here (As only the control thread is updating
		 * the variable below for the data thread to use it.)
		 */
		if (listener->current_set_index == set_index) {
			unsigned short new_set_index = ffs(listener->stream_mask) - 1;

			INF_LOG("listener(%p): disconnect redundant stream(%u, set_index %u), switch to another stream(set_index %u) in the set(%u)\n",
				listener, avdecc_index, set_index, new_set_index, listener->set_id);

			pthread_mutex_lock(&listener->lock);
			listener->current_set_index = new_set_index;
			pthread_mutex_unlock(&listener->lock);
		}
	}

	avbstream_listener_remove(avdecc_index);
}
