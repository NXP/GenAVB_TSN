/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MAIN_AVB_H
#define MAIN_AVB_H

#include <genavb/genavb.h>
#include <unistd.h>
#include <limits.h>

#ifdef __cplusplus
extern "C" {
#endif


/* avdecc controller main context */
struct app_avb_cfg {
	unsigned int mode;
	unsigned int config;
	int connected;
	int listener_discovered; //=1 if avdecc connect received, =0 if not connect or avdecc disconnect received
	int talker_discovered; //=1 if avdecc connect received, =0 if not connect or avdecc disconnect received
	unsigned long long byte_count;
	int volume;
	avb_u8 media_track;
	avb_u64 talker_association_id;
	avb_u64 audio_listener_association_id;
	avb_u64 video_listener_association_id;
	avb_u64 listener_entity_id;
	avb_u64 talker_entity_id;

	struct avb_handle *avb_h;

	int ctrl_avdecc_control_fd;
	struct avb_control_handle *ctrl_avdecc_control_h;

	char **media_tracks;
	avb_u8 media_track_count;
};


int app_avb_setup(struct app_avb_cfg *cfg);

void app_avb_send_aecp_volume_control(struct app_avb_cfg *cfg, avb_u8 volume_percent);
void app_avb_send_aecp_playstop_control(struct app_avb_cfg *cfg, avb_u8 play_stop);
void app_avb_send_avdecc_media_track_control(struct app_avb_cfg *cfg, avb_u8 media_track);
void app_avb_send_avdecc_prevnext_control(struct app_avb_cfg *cfg, avb_u8 next);
int app_avb_handle_avdecc_controller(struct app_avb_cfg *cfg, avb_msg_type_t *msg_type, avb_u16 *ctrl_index, void *ctrl_value);

#ifdef __cplusplus
}
#endif

#endif // MAIN_AVB_H
