/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2019, 2021-2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief Stream handling functions
 @details
*/

#ifndef _STREAM_H_
#define _STREAM_H_

#include "os/sys_types.h"
#include "common/list.h"

#include "avtp.h"

#include "genavb/media.h"
#include "os/media.h"
#include "media_clock.h"

#define HEADER_TEMPLATE_SIZE 64

#define STREAM_FLAG_DESTROYED		(1 << 0)	/* Stream is destroyed and waits to be freed */

#define SET_FLAG_VLAN			(1 << 0)	/* Redundant set of streams that are vlan tagged */
#define SET_FLAG_SR			(1 << 1)	/* Redundant set of streams that have a stream reservation */
#define SET_FLAG_MEDIA_WAKEUP		(1 << 2)	/* Redundant set of streams with processing started by media interface (instead of clock generation) */
#define SET_FLAG_CLOCK_GENERATION	(1 << 3)	/* Redundant set of streams that requires clock generation */
#define SET_FLAG_NO_MEDIA		(1 << 4)	/* Redundant set of streams that don't exchange data with media interface */
#define SET_FLAG_CUSTOM_TSPEC		(1 << 5)	/* Redundant set of streams with params inherited from the media interface */
#define SET_FLAG_DESTROYED		(1 << 6)	/* Redundant set is destroyed and waits to be freed */
#define SET_FLAG_HAS_REDUNDANT_STREAM	(1 << 7)	/* Redundant set has actual redundant streams and not only a single non redundant stream */

struct stream_listener;
struct stream_talker;

/** Common stream context
 *
 * Common fields to Listener and Talker streams
 */
struct stream_common {
	struct avtp_ctx *avtp;
	struct list_head list;
	u64 destroy_time;
	unsigned int flags;
};

/** Common redundant set context
 *
 * Common fields to Listener and Talker redundant sets
 */
struct redundant_set_common {
	struct avtp_ctx *avtp;
	struct list_head list;
	u64 destroy_time;
	unsigned int flags;
};

/** Listener redundant stream context/set.
 *
 * Common context between redundant listener streams
 * Non-redundant streams also possess their own redundant context with ID REDUNDANT_SET_ID_INVALID
 */
struct redundant_set_listener {
	struct redundant_set_common common;		/* Must be placed at the start of the structure: used for deferred destruction as the elementary streams */
	unsigned int stream_count; 			/**< Number of elementary streams associated to this redundant set */
	u16 id;

	struct media_tx media;

	struct avdecc_format format;
	unsigned int subtype;
	sr_class_t class;
	struct clock_source *source;
	struct clock_domain *domain;

	u64 avtp_current;		/* AVTP media clock snapshot taken at the start of the stream batch processing for the current redundant stream providing samples. */
	os_clock_id_t clock_avtp;	/* Common AVTP clock for all streams of the redundant set. */

	int (*init)(struct redundant_set_listener *set);
	void (*exit)(struct redundant_set_listener *set);

	int (*stream_init)(struct stream_listener *stream);

	struct redundant_listener_stats {
		unsigned int media_tx;
		unsigned int media_tx_err;
		unsigned int media_tx_dropped;
		unsigned int clock_tx;
		unsigned int gptp_err;
		unsigned int drop_ts;		/* Packets dropped on first syncronization without valid timestamps. */
		unsigned int sync_success;	/* Synchronization success */
		unsigned int sync_lost;		/* Synchronization loss */
	} stats;

	union {
		struct crf_subtype_data {
			unsigned int ts_last;
			unsigned int ts_last_set;
			unsigned int received_ts_last;
			unsigned int state;
			unsigned int period_nominal;
			unsigned int period_err;
			unsigned int period;
			unsigned int timestamp;
			struct timer timer;
			unsigned int free_wheeling_to_locked_delay;
		} crf;
	} subtype_data;

	/* Merge strategy context */
	struct {
		u64 last_ts;					/* Timestamp of the last valid/used sample (sent to the media queue or CRF handler). */
		unsigned int samples_per_packet;
		unsigned int sample_rate;
		unsigned int timestamp_tolerance;
		unsigned int sample_time;

		u16 sync_state;					/* Bitmask (bit per stream index in the set): current sync state of the stream in this set. */
		u8 next_seq[CFG_AVTP_REDUNDANT_STREAMS_MAX];	/* Next sequence number expected per redundant stream when synced. (Valid only when the corresponding bit in sync_state is set). */
		u32 next_sample_offset[CFG_AVTP_REDUNDANT_STREAMS_MAX]; 	/* Next valid sample offset in the packet when synced (0 <= offset < samples_per_packet)*/
		union {
			struct {
				u32 accumulated_events;
			} crf;

			struct {
				unsigned int sample_stride;
			} media;
		} u;
	} merge;
};

/** Talker redundant stream context/set.
 *
 * Common context between redundant talkers streams
 * Non-redundant streams also possess their own redundant context with ID REDUNDANT_SET_ID_INVALID
 */
struct redundant_set_talker {
	struct redundant_set_common common;		/* Must be placed at the start of the structure: used for deferred destruction as the elementary streams */
	unsigned int stream_count; 			/**< Number of elementary streams associated to this redundant set */
	u16 id;

	struct list_head talker_stream;

	struct media_rx media;

	struct avdecc_format format;
	unsigned int subtype;
	sr_class_t class;

	unsigned int max_transit_time;

	void (*init)(struct redundant_set_talker *set);
	void (*exit)(struct redundant_set_talker *set);

	void (*stream_init)(struct stream_talker *stream, unsigned int *hdr_len);
	void (*stream_exit)(struct stream_talker *stream);

	struct redundant_talker_stats {
		unsigned int media_rx;
		unsigned int media_err;
		unsigned int media_underrun;
	} stats;
};

/** Listener stream context.
 *
 * Each listener stream contains a network receive context. There is a one to one mapping between streams
 * and receive queues.
 */
struct stream_listener {
	struct stream_common common;			/* Must be placed at the start of the structure: used for deferred destruction as the elementary streams */

	struct redundant_set_listener *set;

	u64 id;						/**< AVTP stream_id, stored as Big Endian */
	u8 dst_mac[6];
	u16 port;
	u16 set_index;					/**< Stream index in the redundant set */

	/*
	 * On receive path we have three types of timestamps:
	 * - receive socket timestamps (descriptor receive timestamps): Can be either in gPTP target or local domain
	 * - avtp_timestamps (received in packet payload): Are always in gPTP target domain
	 * - application avtp timestamps (event timestamps passed to applications): Can either be gPTP target or local domain
	 *
	 * AVTP media clock below is the domain which all above timestamps should be converted to (if needed) upon
	 * entry (on receive function) into AVTP layer and will be used/passed afterwards to upper layers (application
	 * and/or Media Clock layers). AVTP clock domain will be mapped in clock layer to either gPTP target or local.
	 * and on stream creation, we detect if gPTP target and AVTP clocks are the same or not. If not, needs_avtp_ts_conversion
	 * is set to true to indicate that a conversion of avtp_timestamps is needed upon packet reception between domains.
	 * Assumption: receive socket timestamps are assumed to be always in the AVTP media domain, so no conversion at AVTP layer
	 * is needed. (Either it is recieved in the right domain or a conversion at socket layer was already performed).
	 *
	 */
	os_clock_id_t clock_gptp;	/**< gPTP target domain. */
	os_clock_id_t clock_avtp;	/**< AVTP media domain. */
	bool needs_avtp_ts_conversion; /* Set to true, if gPTP target clock domain and AVTP media clock domain are different (e.g avtp_timestamps needs to be converted to AVTP domain) */

	avtp_direction_t direction;

	void (*net_rx)(struct stream_listener *, struct avtp_rx_desc **, unsigned int);

	struct net_rx rx;

	u8 sequence_num;
	unsigned int mr;
	unsigned int pkt_received;
	u64 gptp_current;		/* gptp snapshot taken at the start of the stream batch processing */
	u64 gptp_last_rx;		/* gptp snapshot taken at the receive time of the stream batch processing */
	u64 avtp_current;		/* AVTP media clock snapshot taken at the start of the stream batch processing (equals gptp_current if needs_avtp_ts_conversion is false) */
	bool sequence_valid;		/* true while received packets are valid (valid: format/subtype, sequence_num, not interrupted (100ms timeout)) */
	bool stream_interrupted;	/* if a packet is received AVTP_RECEIVE_TIMEOUT apart from the previous packet, the stream is considered to have been interrupted */

	u32 max_transit_time;
	u32 max_timing_uncertainty;

	union {
		struct {
			unsigned int current_frame_offset;
		} cvf;

		struct {
			unsigned int syt_interval_ln2;
			u8 syt_count;
		} iec61883_6;

		struct {
			u32 hdr[2];
			u32 hdr_mask[2];
		} aaf;

		struct {
			/* Header data to match on receive (starting at pull field) */
			u32 hdr[2];
			u8 type;
		} crf;
	} subtype_data;

	struct listener_stats {
		unsigned int rx;
		unsigned int pkt_lost;
		unsigned int mr;
		unsigned int tu;
		unsigned int subtype_err;
		unsigned int format_err;
		unsigned int gptp_err;
		unsigned int early_timestamp;
		unsigned int late_timestamp;
		unsigned int timestamp_valid;
		unsigned int timestamp_invalid;
		unsigned int stream_interruption;
		unsigned int ts_dropped;
		unsigned int clock_tx;
		unsigned int redundancy_skip;
		unsigned int redundancy_valid;
		unsigned int redundancy_sync_fail_late;
		unsigned int redundancy_sync_fail_early;
		unsigned int redundancy_sync_fail_ts_mismatch;
		unsigned int redundancy_sync_lost;
		unsigned int redundancy_sync_lost_undefined;
		unsigned int redundancy_sync_success;
		unsigned int redundancy_drop_ts;

		struct stats avb_delay;
		struct stats avtp_delay;
		struct stats batch;
		struct stats redundancy_samples; /* Redundancy valid samples provided by this stream. */
	} stats;
};

/** Talker stream context
 *
 * Each talker stream contains a network transmit context. There is a one to one mapping between streams
 * and transmit queues.
 */
struct stream_talker {
	struct stream_common common;			/* Must be placed at the start of the structure: used for deferred destruction as the elementary streams */

	struct clock_domain *domain;
	unsigned int locked_count;

	struct redundant_set_talker *set;
	struct list_head set_entry;			/* Membership in redundant_set streams list */

	u64 id;

	avtp_direction_t direction;

	u16 port;
	u16 set_index;					/**< Stream index in the redundant set */

	/*
	 * On transmit path we have three types of timestamps:
	 * - application avtp timestamps (event timestamps from applications): Can either be gPTP target or local domain
	 * - avtp_timestamps (set in packet payload): Are always in gPTP target domain
	 * - grid generated timestamps (use to create avtp_timestamps and/or set packet's launch time): Can either be gPTP target or local domain.
	 *
	 * AVTP media clock below is the domain of the grid's generated timestamps. AVTP clock domain will be mapped in clock layer
	 * to either gPTP target or local. And on stream creation, we detect if gPTP target and AVTP clocks are the same or not. If not,
	 * needs_avtp_ts_conversion is set to true to indicate that a conversion of the grid's timestamp between the two
	 * domains (From AVTP to target gPTP domain) is needed.
	 *
	 */
	os_clock_id_t clock_gptp;	/**< gPTP target domain. */
	os_clock_id_t clock_avtp;	/**< AVTP media domain. */
	bool needs_avtp_ts_conversion; /* Set to true, if gPTP target clock domain and AVTP media clock domain are different (e.g generated timestamps needs to be converted to gPTP target domain) */

	void (*net_tx)(struct stream_talker *);

	struct net_tx tx;

	unsigned int payload_size;
	unsigned int frames_per_interval;
	unsigned int frames_per_packet;
	unsigned int media_count;
	unsigned int sample_rate;
	unsigned int samples_per_timestamp;

	struct clock_grid_consumer consumer;	// Uses a grid with a MULT producer, itself using a source with a HW producer
	bool consumer_enabled;

	unsigned int ts_n;
	unsigned int tx_batch;
	unsigned int late;
	unsigned int frame_with_ts;
	unsigned int time_per_packet;
	unsigned int ts_last;
	u32 ts_launch;
	u64 gptp_current; 	/* gptp snapshot taken at the start of the stream batch processing */
	u64 avtp_current;	/* AVTP media clock snapshot taken at the start of the stream batch processing (equals gptp_current if needs_avtp_ts_conversion is false) */

	unsigned int ts_media_prev;

	unsigned int header_len;

	unsigned int tx_event_enabled;
	unsigned long priv;

	union {
		struct {
			unsigned int syt_interval_ln2;
			struct iec_61883_hdr *iec_hdr;
		} iec61883;

		struct {
			unsigned int frames_per_timestamp;
			unsigned int sparse;
			unsigned int tx_count;
		} aaf;

		struct {
			u32 ts_msb;
			u32 ts_period;
			struct os_timer t;
		} crf;

		struct {
			struct cvf_h264_hdr *h264_hdr;
			unsigned int prev_incomplete_nal;
			unsigned int is_nalu_ts_valid;
			u32 h264_timestamp;
			u8 nalu_header;
		} cvf_h264;
	} subtype_data;

	unsigned int latency;

	struct avtp_data_hdr *avtp_hdr;
	u8 header_template[HEADER_TEMPLATE_SIZE];

	struct talker_stats {
		unsigned int tx;
		unsigned int tx_err;
		unsigned int mr;
		unsigned int clock_rx;
		unsigned int clock_err;
		unsigned int partial;
		unsigned int clock_invalid;
		unsigned int gptp_err;
		unsigned int timestamp_valid;
		unsigned int timestamp_invalid;

		struct stats sched_intvl;
	} stats;
};

struct ipc_avtp_listener_stats {
	avb_u64 stream_id;
	avb_u16 set_id;
	avb_u16 set_index;
	struct listener_stats stats;
	bool is_redundant;
};

struct ipc_avtp_set_listener_stats {
	avb_u16 set_id;
	struct redundant_listener_stats stats;
	bool clock_rec_enabled;
	bool has_redundant_streams;

	struct ipc_avtp_clock_rec_stats clock_stats;
};

struct ipc_avtp_talker_stats {
	avb_u64 stream_id;
	struct talker_stats stats;
	struct redundant_talker_stats set_stats;
};

#define stream_destroy(stream, ipc_tx) \
{	\
	if ((stream)->direction == AVTP_DIRECTION_LISTENER)	\
		stream_listener_destroy((struct stream_listener *)stream, ipc_tx);	\
	else	\
		stream_talker_destroy((struct stream_talker *)stream, ipc_tx);	\
}

struct redundant_set_listener *redundant_set_listener_find(struct avtp_ctx *avtp, u16 set_id);

struct stream_listener *stream_listener_find(struct avtp_port *port, void *stream_id);
struct stream_talker *stream_talker_find(struct avtp_port *port, void *stream_id);

void avtp_latency_stats(struct stream_listener *stream, struct avtp_rx_desc *desc);

struct stream_listener *stream_listener_create(struct avtp_ctx *avtp, struct avtp_port *port, struct ipc_avtp_connect *params);
void stream_listener_destroy(struct stream_listener *stream, struct ipc_tx *tx);

struct stream_talker *stream_talker_create(struct avtp_ctx *avtp, struct avtp_port *port, struct ipc_avtp_connect *params);
void stream_talker_destroy(struct stream_talker *stream, struct ipc_tx *tx);
int stream_media_rx(struct stream_talker *stream, struct media_rx_desc **media_desc_array, unsigned int *flags, unsigned int *alignment_ts);

void stream_media_audio_tx_merge(struct stream_listener *stream, struct media_desc **desc, unsigned int *n);

int stream_tx_flow_control(struct stream_talker *stream, unsigned int *tx_batch);

void set_stats_dump(struct avtp_ctx *avtp, struct ipc_tx *tx);
void set_listener_stats_print(struct ipc_avtp_set_listener_stats *msg);

void stream_stats_dump(struct avtp_port *port, struct ipc_tx *tx);
void stream_talker_stats_print(struct ipc_avtp_talker_stats *msg);
void stream_listener_stats_print(struct ipc_avtp_listener_stats *msg);

void redundant_set_free_all(struct avtp_ctx *avtp);
void stream_free_all(struct avtp_ctx *avtp);

int stream_clock_consumer_enable(struct stream_talker *stream);
void stream_clock_consumer_disable(struct stream_talker *stream);

unsigned int avtp_stream_presentation_offset(struct stream_talker *stream);

bool stream_check_interrupted(struct stream_listener *stream, u64 gptp_now);

static inline void stream_media_clock_reset(struct stream_talker *stream)
{
	avtp_data_header_toggle_mcr(stream->avtp_hdr);

	stream->stats.mr++;
}

static inline void stream_net_tx_handler(struct stream_talker *stream)
{
	u64 current_time;

	if (os_clock_gettime64(stream->clock_gptp, &current_time) < 0) {
		stream->stats.gptp_err++;
	} else {
		stats_update(&stream->stats.sched_intvl, current_time - stream->gptp_current);
		stream->gptp_current = current_time;
	}

	if (stream->needs_avtp_ts_conversion) {
		if (os_clock_gettime64(stream->clock_avtp, &stream->avtp_current) < 0)
			stream->stats.gptp_err++;
	} else {
		stream->avtp_current = stream->gptp_current;
	}

	stream->net_tx(stream);
}

static inline int stream_net_tx(struct stream_talker *stream, struct media_rx_desc **desc, unsigned int n)
{
	int i, rc;

	/* Send packets */
	rc = net_tx_multi(&stream->tx, (struct net_tx_desc **)desc, n);
	if (rc < (int)n) {
		stream->stats.tx += rc;
		stream->stats.tx_err += n - rc;

		for (i = rc; i < n; i++)
			net_tx_free((struct net_tx_desc *)desc[i]);

		return -1;
	}

	stream->stats.tx += rc;

	return 0;
}

static inline int stream_media_tx(struct stream_listener *stream, struct media_desc **desc, unsigned int n)
{
	int rc = 0;

	if (stream->set->common.flags & SET_FLAG_HAS_REDUNDANT_STREAM) {
		/* Only specific audio stream formats support Milan redundancy. */
		stream_media_audio_tx_merge(stream, desc, &n);
		if (!n)
			goto out;
	}

	rc = media_tx(&stream->set->media, desc, n);
	if (rc < (int)n) {
		if (rc < 0) {
			stream->set->stats.media_tx_err++;
			stream->set->stats.media_tx_dropped += n;
		}
		else
			stream->set->stats.media_tx_dropped += n - rc;
	}

	if (rc > 0)
		stream->set->stats.media_tx += rc;

out:
	return rc;
}

static inline unsigned int stream_domain_phase_change(struct stream_talker *stream)
{
	if (stream->locked_count != stream->domain->locked_count) {
		stream->locked_count = stream->domain->locked_count;
		return 1;
	}

	return 0;
}

static inline void stream_listener_check_timestamps(struct stream_listener *stream, avb_u32 rx_ts, avb_u32 avtp_ts, unsigned int tsamples)
{
	if (is_avtp_ts_late(rx_ts, avtp_ts, tsamples))
		stream->stats.late_timestamp++;
	else if (is_avtp_ts_early(rx_ts, avtp_ts, stream->max_transit_time, stream->max_timing_uncertainty, tsamples))
		stream->stats.early_timestamp++;
}

static inline bool is_redundant_set_synced(struct redundant_set_listener *set)
{
	return (set->merge.sync_state != 0);
}

static inline bool is_redundant_stream_synced(struct redundant_set_listener *set, u16 set_index)
{
	return (set->merge.sync_state & (1U << set_index));
}

static inline void redundant_set_stream_synced(struct redundant_set_listener *set, u16 set_index)
{
	set->merge.sync_state |= (1U << set_index);
}

static inline void redundant_set_stream_unsynced(struct redundant_set_listener *set, u16 set_index)
{
	set->merge.sync_state &= ~(1U << set_index);
}

/* Track the next valid sample offset and sequence number for (other) synchronized streams */
static inline void redundant_set_increment_synced_other(struct redundant_set_listener *set, u16 set_index, u32 sent_samples)
{
	int i;

	for (i = 0; i < CFG_AVTP_REDUNDANT_STREAMS_MAX; i++) {
		if (i == set_index)
			continue;

		if (is_redundant_stream_synced(set, i)) {
			u32 total_sample_offset = set->merge.next_sample_offset[i] + sent_samples;
			u8 packet_increment = total_sample_offset / set->merge.samples_per_packet;

			set->merge.next_seq[i] = (set->merge.next_seq[i] + packet_increment) & 0xff;
			set->merge.next_sample_offset[i] = total_sample_offset - packet_increment * set->merge.samples_per_packet;
		}
	}
}

/* The precision error introduced per sample by using multiplication of sample_time here is still lower than
 * the 25% sample tolerance.
 */
#define SAMPLE_OFFSET_NS(sample_time, sample_index)	((sample_time) * (sample_index))
#define seq_number_after(a , b)	((s8)((u8)(a) - (u8)(b)) > 0)

/* Get the sample index from the start of a packet based on the sample timestamp
 * with a tolerance of 25% of sample time
 * The sample ts needs to be: ts >= start_ts - 25% sample_time
 */
static inline int get_sample_index(u64 start_ts, u64 ts, u32 sample_time_ns, u32 *sample_index)
{
	u32 index, rem;
	u64 delta;

	if (ts <= start_ts) {
		delta = start_ts - ts;

		/* Accept start_ts - 25% sample_time <= ts <= start_ts and return the right sample index */
		if (delta <= (sample_time_ns >> 2U)) {
			index = 0;
			goto out;
		} else {
			goto err;
		}
	}

	delta = ts - start_ts;
	index = delta / sample_time_ns;
	rem = delta - index * sample_time_ns;

	/* Tolerance of 25% sample time:
	 * - If remainder is >= 75% sample time, the index is the next divider
	 * - If remainder is <= 25% sample time, the index is the current divider
	 */
	if (rem >= ((sample_time_ns * 3U) >> 2U))
		index++;
	else if (rem > (sample_time_ns >> 2U))
		goto err;

out:
	if (sample_index)
		*sample_index = index;

	return 0;

err:
	return -1;
}
#endif /* _STREAM_H_ */
