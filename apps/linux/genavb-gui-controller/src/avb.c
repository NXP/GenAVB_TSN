/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <sys/time.h>
#include <arpa/inet.h>

#include <genavb/genavb.h>
#include <genavb/srp.h>
#include <genavb/aecp.h>
#include <genavb/aem.h>
#include <genavb/net_types.h>

#include "avb.h"

#include "../../common/aecp.h"

#ifdef __cplusplus
extern "C" {
#endif



int app_avb_adp_start_dump_entities(struct app_avb_cfg *cfg)
{
	struct avb_adp_msg msg;
	int rc;

	msg.msg_type = ADP_ENTITY_DISCOVER;
	rc = avb_control_send(cfg->ctrl_avdecc_control_h, AVB_MSG_ADP, &msg, sizeof(struct avb_adp_msg));

	return rc;
}


void app_avb_send_aecp_volume_control(struct app_avb_cfg *cfg, avb_u8 volume_percent)
{
	struct avb_control_handle *ctrl_h = cfg->ctrl_avdecc_control_h;
	int rc;

	rc = aecp_aem_send_set_control_single_u8_command(ctrl_h, &cfg->listener_entity_id, 0, &volume_percent, NULL, 0);
	if (rc != AVB_SUCCESS)
		printf("%s: aecp_aem_send_set_control_single_u8_command failed with error %d(%s)\n", __func__, rc, avb_strerror(rc));
}

void app_avb_send_aecp_playstop_control(struct app_avb_cfg *cfg, avb_u8 play_stop)
{
	struct avb_control_handle *ctrl_h = cfg->ctrl_avdecc_control_h;
	int rc;

	rc = aecp_aem_send_set_control_single_u8_command(ctrl_h, &cfg->talker_entity_id, 1, &play_stop, NULL, 0);
	if (rc != AVB_SUCCESS)
		printf("%s: aecp_aem_send_set_control_single_u8_command failed with error %d(%s)\n", __func__, rc, avb_strerror(rc));
}

void app_avb_send_avdecc_media_track_control(struct app_avb_cfg *cfg, avb_u8 media_track)
{
	struct avb_control_handle *ctrl_h = cfg->ctrl_avdecc_control_h;
	int rc;

	rc = aecp_aem_send_set_control_single_u8_command(ctrl_h,  &cfg->talker_entity_id, 2, &media_track, NULL, 0);
	if (rc != AVB_SUCCESS)
		printf("%s: aecp_aem_send_set_control_single_u8_command failed with error %d(%s)\n", __func__, rc, avb_strerror(rc));
}

void app_avb_send_avdecc_prevnext_control(struct app_avb_cfg *cfg, avb_u8 next)
{
	if (next) /* increment or decrement track number */
		cfg->media_track+=1;
	else
		cfg->media_track-=1;

	app_avb_send_avdecc_media_track_control(cfg, cfg->media_track);
}

void app_avb_display_entity_info(struct entity_info *info)
{
	printf("Entity ID = 0x%" PRIx64 "\n", ntohll(info->entity_id));
	printf("     caps = 0x%x, talker caps = 0x%x, listener caps = 0x%x controller caps = 0x%x association ID = 0x%" PRIx64 "\n",
			info->entity_capabilities, info->talker_capabilities, info->listener_capabilities, info->controller_capabilities, ntohll(info->association_id));

}


void app_avb_aecp_get_volume(struct app_avb_cfg *cfg)
{
	struct avb_control_handle *ctrl_h = cfg->ctrl_avdecc_control_h;
	int rc;

	rc = aecp_aem_send_get_control(ctrl_h, &cfg->listener_entity_id, 0, NULL, NULL, NULL, 0);
	if (rc != AVB_SUCCESS)
		printf("%s: aecp_aem_send_get_control failed with error %d(%s)\n", __func__, rc, avb_strerror(rc));

}

void app_avb_aecp_get_playstop(struct app_avb_cfg *cfg)
{
	struct avb_control_handle *ctrl_h = cfg->ctrl_avdecc_control_h;
	int rc;

	rc = aecp_aem_send_get_control(ctrl_h, &cfg->talker_entity_id, 1, NULL, NULL, NULL, 0);
	if (rc != AVB_SUCCESS)
		printf("%s: aecp_aem_send_get_control failed with error %d(%s)\n", __func__, rc, avb_strerror(rc));
}

void app_avb_aecp_get_media_track(struct app_avb_cfg *cfg)
{
	struct avb_control_handle *ctrl_h = cfg->ctrl_avdecc_control_h;
	int rc;

	rc = aecp_aem_send_get_control(ctrl_h, &cfg->talker_entity_id, 2, NULL, NULL, NULL, 0);
	if (rc != AVB_SUCCESS)
		printf("%s: aecp_aem_send_get_control failed with error %d(%s)\n", __func__, rc, avb_strerror(rc));
}


int app_avb_aecp_set_media_track(struct app_avb_cfg *cfg, avb_u8 track)
{
	struct avb_control_handle *ctrl_h = cfg->ctrl_avdecc_control_h;
	avb_u8 status;
	avb_u8 result = track;
	int rc;

	rc = aecp_aem_send_set_control_single_u8_command(ctrl_h, &cfg->talker_entity_id, 2, &result, &status, 1);

	if ((rc == AVB_SUCCESS) && (status == AECP_AEM_SUCCESS))
		rc = result;
	else
		rc = -1;

	return rc;
}

int app_avb_aecp_get_media_track_name(struct app_avb_cfg *cfg, char *track_name, avb_u16 input_len)
{
	struct avb_control_handle *ctrl_h = cfg->ctrl_avdecc_control_h;
	avb_u8 status;
	avb_u16 len = input_len;
	int rc;

	rc = aecp_aem_send_get_control(ctrl_h, &cfg->talker_entity_id, 3, track_name, &len, &status, 1);

	if ((rc == AVB_SUCCESS) && (status == AECP_AEM_SUCCESS))
		rc = len;
	else
		rc = -1;

	return rc;
}


#define DEFAULT_MEDIA_TRACK_COUNT 8
#define ACQUIRE_WAIT_DELAY	100000  // in us
void app_avb_build_media_track_list(struct app_avb_cfg *cfg)
{
	avb_u8 track = 1;
	int count = 0;
	int rc;

	// Acquire entity
	rc = aecp_aem_acquire_entity(cfg->ctrl_avdecc_control_h, &cfg->talker_entity_id, 1);
	while (rc == 0) {
		usleep(ACQUIRE_WAIT_DELAY);
		count++;
		rc = aecp_aem_acquire_entity(cfg->ctrl_avdecc_control_h, &cfg->talker_entity_id, 1);
	}
	if (count)
		printf("%s: Entity acquisition took %f seconds\n", __func__, count*(ACQUIRE_WAIT_DELAY/1000000.0));

	if (rc != 1) {
		printf("%s: Acquire entity failed\n", __func__);
		goto exit;
	}

	// Allocate array of strings
	if (!cfg->media_tracks) {
		cfg->media_tracks = malloc(DEFAULT_MEDIA_TRACK_COUNT*sizeof(char *));
		if (!cfg->media_tracks) {
			printf("%s: malloc failed while allocating array %d\n", __func__, track);
			goto exit;
		}
		memset(cfg->media_tracks, 0, DEFAULT_MEDIA_TRACK_COUNT*sizeof(char *));
	}


	// Fetch file names, one-by-one
	while (app_avb_aecp_set_media_track(cfg, track) > 0) {
		if (!cfg->media_tracks[track - 1]) {
			cfg->media_tracks[track - 1] = malloc(AEM_UTF8_MAX_LENGTH);
			if (!cfg->media_tracks[track - 1]) {
				printf("%s: malloc failed at track %d\n", __func__, track);
				goto exit;
			}
		}

		rc = app_avb_aecp_get_media_track_name(cfg, cfg->media_tracks[track - 1], AEM_UTF8_MAX_LENGTH);
		if (rc < 0) {
			printf("%s: get track name failed at track %d\n", __func__, track);
			goto exit;
		}

		track++;
		if ((track % DEFAULT_MEDIA_TRACK_COUNT) == 0) {
			printf("%s: reallocating array after %d tracks.\n", __func__, track);
			cfg->media_tracks = realloc(cfg->media_tracks, (track + DEFAULT_MEDIA_TRACK_COUNT)*(sizeof(char *)));
			if (!cfg->media_tracks) {
				printf("%s: realloc failed while allocating array %d\n", __func__, track);
				goto exit;
			}
		}
	}

exit:
	cfg->media_track_count = track - 1;
	// Release entity.
	rc = aecp_aem_acquire_entity(cfg->ctrl_avdecc_control_h, &cfg->talker_entity_id, 0);
	if (rc != 1)
		printf("%s: WARNING got return code %d while releasing talker entity\n", __func__, rc);
}

int avb_register_unsolicited(struct avb_control_handle *ctrl_h, avb_u64 *entity_id)
{
	struct avb_aecp_msg aecp_msg;
	struct aecp_aem_pdu *aecp_cmd = (struct aecp_aem_pdu *) aecp_msg.buf;
	unsigned int msg_type, msg_len;
	int rc;

	aecp_msg.msg_type =  AECP_AEM_COMMAND;
	aecp_msg.len = sizeof(struct aecp_aem_pdu);
	memset(aecp_cmd, 0, sizeof(struct aecp_aem_pdu));
	AECP_AEM_SET_U_CMD_TYPE(aecp_cmd, 0, AECP_AEM_CMD_REGISTER_UNSOLICITED_NOTIFICATION);
	memcpy(&aecp_cmd->entity_id, entity_id, 8);

	msg_type = AVB_MSG_AECP;
	msg_len = sizeof(struct avb_aecp_msg);
	rc = avb_control_send_sync(ctrl_h, &msg_type, &aecp_msg, sizeof(struct avb_aecp_msg), &aecp_msg, &msg_len, -1);
	if ((rc != AVB_SUCCESS) || (msg_type != AVB_MSG_AECP)) {
		printf("avb_control_send_sync() failed: AVB_MSG_AECP\n");
		return -1;
	}

	if (aecp_msg.status != AECP_AEM_SUCCESS) {
		printf("REGISTER_UNSOLICITED_NOTIFICATION failed with status = %d\n", aecp_msg.status);
		return -1;
	}

	return 0;
}


#define ADP_ENTITY_ASSOCIATION_ID_SUPPORTED		(1 << 5)
#define ADP_ENTITY_ASSOCIATION_ID_VALID			(1 << 6)

int avdecc_association_id_match(struct entity_info *info, avb_u64 association_id)
{
	return ( (info->entity_capabilities & ntohl(ADP_ENTITY_ASSOCIATION_ID_SUPPORTED | ADP_ENTITY_ASSOCIATION_ID_VALID))
			&& (info->association_id == ntohll(association_id)));
}


int app_avb_handle_adp(struct app_avb_cfg *cfg, struct avb_adp_msg *msg, avb_u16 *ctrl_index)
{
	int rc = AVB_ERR_CTRL_RX;  // Make sure the AvbController object ignores the ctrl value by default when handling ADP
	int i;

	switch (msg->msg_type) {
	case ADP_ENTITY_AVAILABLE:
		printf("New entity detected: ");
		app_avb_display_entity_info(&msg->info);

		if (msg->info.listener_capabilities && (!cfg->listener_discovered) && avdecc_association_id_match(&msg->info, cfg->audio_listener_association_id)) {  // For now, assume the first one we see is the one we are interested in
			cfg->listener_discovered = 1;
			memcpy(&cfg->listener_entity_id, &msg->info.entity_id, 8);

			avb_register_unsolicited(cfg->ctrl_avdecc_control_h, &msg->info.entity_id);
			//TODO fetch volume value from listener entity with GET_CONTROL command
			//cfg->volume = app_avb_aecp_get_volume(cfg);
		}

		if (msg->info.talker_capabilities && (!cfg->talker_discovered) && avdecc_association_id_match(&msg->info, cfg->talker_association_id)) {  // For now, assume the first one we see is the one we are interested in
			cfg->talker_discovered = 1;
			memcpy(&cfg->talker_entity_id, &msg->info.entity_id, 8);

			app_avb_build_media_track_list(cfg);
			*ctrl_index = 10;
			rc = AECP_AEM_SUCCESS;

			for (i = 0; i < cfg->media_track_count; i++)
				printf("%s: media track %d = %s\n", __func__, i, cfg->media_tracks[i]);

			avb_register_unsolicited(cfg->ctrl_avdecc_control_h, &msg->info.entity_id);
			//Fetch Play/Pause status and Media track ID from talker entity  with GET_CONTROL command
			app_avb_aecp_get_media_track(cfg);
			app_avb_aecp_get_playstop(cfg);
		}


		break;
	case ADP_ENTITY_DEPARTING:
		printf("Departing entity: ");
		app_avb_display_entity_info(&msg->info);

		if (memcmp(&cfg->listener_entity_id, &msg->info.entity_id, 8) == 0)
			cfg->listener_discovered = 0;

		if (memcmp(&cfg->talker_entity_id, &msg->info.entity_id, 8) == 0)
			cfg->talker_discovered = 0;

		break;
	default:
		printf("Unknown ADP message type %d\n", msg->msg_type);
		break;
	}

	return rc;
}

/*
 * Possible values for *ctrl_index:
 * 0 => volume control changed
 * 1 => play/stop
 * 2 => media track ID changed
 * 3 => media track name changed
 * 4 => media track list changed
 */
int app_avb_handle_aecp_aem(struct avb_aecp_msg *msg, avb_u16 *ctrl_index, void *ctrl_value)
{
	struct aecp_aem_pdu *aem = (struct aecp_aem_pdu *)msg->buf;
	avb_u16 cmd_type = AECP_AEM_GET_CMD_TYPE(aem);
	unsigned short status = AECP_AEM_SUCCESS;

	if (msg->msg_type != AECP_AEM_RESPONSE)
		printf("%s: WARNING Received message type %d but only AECP_AEM_RESPONSE(%d) expected here.\n", __func__, msg->msg_type, AECP_AEM_RESPONSE);

	if (msg->status != AECP_AEM_SUCCESS)
			printf("%s: WARNING Received response with status %d.\n", __func__, msg->status);


	switch (cmd_type) {
	case AECP_AEM_CMD_SET_CONTROL:
	case AECP_AEM_CMD_GET_CONTROL:
	{
		struct aecp_aem_set_get_control_pdu *set_control_cmd  = (struct aecp_aem_set_get_control_pdu *)(aem + 1);
		void *values_cmd = set_control_cmd + 1;
		*ctrl_index = ntohs(set_control_cmd->descriptor_index);

		/* Only descriptor index 0,1,2 has been mapped here */
		if (*ctrl_index > 3)
			return AECP_AEM_NO_SUCH_DESCRIPTOR;

		if (*ctrl_index == 3)
			memcpy(ctrl_value, values_cmd, msg->len - sizeof(struct aecp_aem_pdu) - sizeof(struct aecp_aem_set_get_control_pdu));
		else
			*(avb_u8 *)ctrl_value = *(avb_u8 *)values_cmd;

		break;
	}
	default:
		status = AECP_AEM_NOT_IMPLEMENTED;
		printf("%s: WARNING: received command(%d) which is not implemented\n", __func__, cmd_type);
		break;
	}
	return status;
}


int app_avb_handle_avdecc_controller(struct app_avb_cfg *cfg, avb_msg_type_t *msg_type, avb_u16 *ctrl_index, void *ctrl_value)
{
	struct avb_control_handle *ctrl_h = cfg->ctrl_avdecc_control_h;
	union avb_controller_msg avdecc_msg;
	unsigned int msg_len;
	int rc = AVB_SUCCESS;
	int ret;

	msg_len = sizeof(union avb_controller_msg);
	ret = avb_control_receive(ctrl_h, msg_type, &avdecc_msg, &msg_len);
	if (ret != AVB_SUCCESS) {
		printf("%s: WARNING: Got error message %d(%s) while trying to receive AECP command.\n", __func__, ret, avb_strerror(ret));
		rc = AVB_ERR_CTRL_RX;
		goto receive_error;
	}

	switch (*msg_type) {
	case AVB_MSG_ADP:
		printf("AVB_MSG_ADP\n");

		rc = app_avb_handle_adp(cfg, &avdecc_msg.adp, ctrl_index);

		break;
	case AVB_MSG_AECP:
		printf("AVB_MSG_AECP\n");

		ret = app_avb_handle_aecp_aem(&avdecc_msg.aecp, ctrl_index, ctrl_value);
		if (ret != AECP_AEM_SUCCESS)
			rc = AVB_ERR_CTRL_RX;

		break;
	default:
		printf("%s: WARNING: Received unsupported AVDECC message type %d.\n", __func__, *msg_type);
		rc = AVB_ERR_CTRL_RX;
		break;
	}

receive_error:
	return rc;
}




static void set_avb_config(unsigned int *avb_flags)
{
	*avb_flags = 0;
}


int app_avb_setup(struct app_avb_cfg *cfg)
{
	unsigned int avb_flags;
	int rc = 0;

	/*
	* setup the avb stack
	*/
	set_avb_config(&avb_flags);

	rc = avb_init(&cfg->avb_h, avb_flags);
	if (rc != AVB_SUCCESS) {
		printf("avb_init() failed: %s\n", avb_strerror(rc));
		rc = -1;
		goto error_avb_init;
	}

	/*
	 * open AVB_CTRL_AVDECC_CONTROLLER control type channel to send commands (volume control)
	 */
	rc = avb_control_open(cfg->avb_h, &cfg->ctrl_avdecc_control_h, AVB_CTRL_AVDECC_CONTROLLER);
	if (rc != AVB_SUCCESS) {
		printf("avb_control_open(AVB_CTRL_AVDECC_CONTROLLER) failed: %s\n", avb_strerror(rc));
		goto error_control_avdecc_controller;
	}

	cfg->ctrl_avdecc_control_fd = avb_control_rx_fd(cfg->ctrl_avdecc_control_h);
	cfg->media_tracks = NULL;
	cfg->media_track_count = 0;

	app_avb_adp_start_dump_entities(cfg);

	return 0;

error_control_avdecc_controller:
	avb_exit(cfg->avb_h);

error_avb_init:
	return rc;
}

#ifdef __cplusplus
}
#endif
