/*
 * Copyright 2021 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <inttypes.h>

#include <genavb/genavb.h>

#define NVRAM_BINDING_PARAMS_PER_ENTRY	6
#define NVRAM_ENTRY_MAX_LEN		256
#define TEMPORARY_FILENAME		"/etc/genavb/tmp_milan_params_binding" //Keep it on the same filesystem as the orginal file for the sake of rename()

#define MAX_LISTENER_STREAMS	16

static int nvram_update_entry(const char *binding_filename, avb_u64 entity_id, avb_u16 listener_stream_index, avb_u64 talker_entity_id, avb_u16 talker_stream_index, avb_u64 controller_entity_id, avb_u16 started)
{
	FILE *fOrig;
	FILE *fTmp;
	char new_entry[NVRAM_ENTRY_MAX_LEN];
	char *orig_entry = NULL;
	unsigned int read_size;
	size_t len = 0;
	avb_u64 orig_entity_id, orig_talker_entity_id, orig_controller_entity_id;
	avb_u16 orig_listener_stream_index, orig_talker_stream_index, orig_started;
	bool matching_entry = false;
	int rc = 0;

	fOrig = fopen(binding_filename, "r");
	if (fOrig == NULL) {
		printf("fopen(%s) failed: %s\n", binding_filename, strerror(errno));
		goto err_orig_file_open;
	}

	fTmp = fopen(TEMPORARY_FILENAME, "w");
	if (fTmp == NULL) {
		printf("fopen(%s) failed: %s\n", TEMPORARY_FILENAME, strerror(errno));
		goto err_tmp_file_open;
	}

	if (talker_entity_id)
		sprintf(new_entry, "%016"PRIx64" %u %016"PRIx64" %u %016"PRIx64" %u\n",
			entity_id , listener_stream_index,
			talker_entity_id, talker_stream_index,
			controller_entity_id, started);

	while(((read_size = getline(&orig_entry, &len, fOrig)) != -1)) {
		if (read_size != strlen(orig_entry)) {
			printf("Unexpected embedded null byte(s)\n");
			goto err_read_entry;
		}

		if (sscanf(orig_entry, "%016"PRIx64" %hu %016"PRIx64" %hu %016"PRIx64" %hu", &orig_entity_id, &orig_listener_stream_index,
				&orig_talker_entity_id, &orig_talker_stream_index, &orig_controller_entity_id, &orig_started) == NVRAM_BINDING_PARAMS_PER_ENTRY) {

			matching_entry = true;

			if (orig_listener_stream_index != listener_stream_index || orig_entity_id != entity_id) {
				//printf("keep original entry %s\n", orig_entry);
				fputs(orig_entry, fTmp);
			} else if (talker_entity_id) { /* Update the line if valid talker_entity_id, otherwise just remove it. */
				//printf("update new entry %s\n", new_entry);
				fputs(new_entry, fTmp);
			}
		}
	}

	if (!matching_entry)
		fputs(new_entry, fTmp);

	free(orig_entry);
	fclose(fOrig);
	fsync(fileno(fTmp));
	fclose(fTmp);

	rc = remove(binding_filename);
	if (rc < 0) {
		printf("remove() failed, %s\n", strerror(errno));
		goto err;
	}

	rc = rename(TEMPORARY_FILENAME, binding_filename);
	if (rc < 0) {
		printf("rename() failed, %s\n", strerror(errno));
		goto err;
	}

	return 0;

err_read_entry:
	fclose(fTmp);
err_tmp_file_open:
	fclose(fOrig);

err_orig_file_open:
err:
	return -1;
}

static int parse_binding_params_file(const char *binding_filename, struct genavb_msg_media_stack_bind *binding_params, unsigned int size)
{
	FILE *fBind;
	char *nvram_entry = NULL;
	unsigned int read_char;
	avb_u64 entity_id, talker_entity_id, controller_entity_id;
	avb_u16 listener_stream_index, talker_stream_index, started;
	size_t len = 0;
	int rc = 0;

	if (!binding_filename || !binding_params || !size) {
		rc = -1;
		goto err;
	}

	fBind = fopen(binding_filename, "a+");
	if (fBind == NULL) {
		printf("fopen(%s) failed: %s\n", binding_filename, strerror(errno));
		rc = -1;
		goto err;
	}

	printf("binding params file name: %s\n", binding_filename);

	/* read all entries in the nvram file one by one. if stream index matches read it */
	while (((read_char = getline(&nvram_entry, &len, fBind)) != -1)) {
		if (read_char != strlen(nvram_entry)) {
			printf("Unexpected embedded null byte(s)\n");
			rc = -1;
			goto err_read_nvram;
		}

		if (sscanf(nvram_entry, "%016"PRIx64" %hu %016"PRIx64" %hu %016"PRIx64" %hu", &entity_id, &listener_stream_index,
				&talker_entity_id, &talker_stream_index, &controller_entity_id, &started) == NVRAM_BINDING_PARAMS_PER_ENTRY) {

			printf("Read EntityID %016"PRIx64" Listener unique ID %u Talker entity ID %016"PRIx64" Talker unique ID %u Controller entity ID%016"PRIx64" Started %u\n",
				entity_id , listener_stream_index, talker_entity_id, talker_stream_index,
				controller_entity_id, started);

			if (listener_stream_index < size) {
				binding_params[listener_stream_index].entity_id = entity_id;
				binding_params[listener_stream_index].listener_stream_index = listener_stream_index;
				binding_params[listener_stream_index].talker_entity_id = talker_entity_id;
				binding_params[listener_stream_index].talker_stream_index = talker_stream_index;
				binding_params[listener_stream_index].controller_entity_id = controller_entity_id;
				binding_params[listener_stream_index].started = started;
			} else
				printf("error on nvram entry %s, out of bound stream index %u\n", nvram_entry, listener_stream_index);
		} else
			printf("error: can not read data from nvram\n");
	}

err_read_nvram:
	free(nvram_entry);
	fclose(fBind);

err:
	return rc;
}

static int avdecc_listener_stream_bind_send_ipc(struct avb_control_handle *s_avdecc_handle, struct genavb_msg_media_stack_bind *binding_params)
{
	struct genavb_msg_media_stack_bind media_stack_bind;
	unsigned int msg_len = sizeof(media_stack_bind);
	genavb_msg_type_t msg_type = GENAVB_MSG_MEDIA_STACK_BIND;
	int rc = AVB_SUCCESS;

	if (!binding_params || !s_avdecc_handle) {
		rc = AVB_ERR_INVALID_PARAMS;
		goto exit;
	}

	media_stack_bind.entity_id = binding_params->entity_id;
	media_stack_bind.listener_stream_index = binding_params->listener_stream_index;
	media_stack_bind.talker_entity_id = binding_params->talker_entity_id;
	media_stack_bind.talker_stream_index = binding_params->talker_stream_index;
	media_stack_bind.controller_entity_id = binding_params->controller_entity_id;
	media_stack_bind.started = binding_params->started;

	rc = avb_control_send(s_avdecc_handle, msg_type, &media_stack_bind, msg_len);
	if (rc != AVB_SUCCESS) {
		printf("avb_control_send (GENAVB_CTRL_AVDECC_MEDIA_STACK) failed: %s\n", avb_strerror(rc));
		goto exit;
	}

	printf("Sent binding parameters: EntityID %016"PRIx64" Listener unique ID %u Talker entity ID %016"PRIx64" Talker unique ID %u Controller entity ID%016"PRIx64" Started %u\n",
		binding_params->entity_id , binding_params->listener_stream_index,
		binding_params->talker_entity_id, binding_params->talker_stream_index,
		binding_params->controller_entity_id, binding_params->started);
exit:
	return rc;
}

/** Parses the file containing the saved binding params and init the avdecc stack with it
 *
 * \return 0 on success, negative otherwise.
 * \param	ctrl_h			Handle to an avb_control_handle, opened for an AVB_CTRL_AVDECC_MEDIA_STACK channel.
 * \param	binding_filename	path to the file used for saved binding parameters.
 */
int avdecc_nvm_bindings_init(struct avb_control_handle *s_avdecc_handle, const char *binding_filename)
{
	int i;
	struct genavb_msg_media_stack_bind listener_streams_binding_params[MAX_LISTENER_STREAMS] = {0};

	if (!s_avdecc_handle) {
		printf("Invalid avb control handle\n");
		goto err;
	}

	if (!binding_filename) {
		printf("Invalid binding filename\n");
		goto err;
	}

	/* Get saved binding parameters from non-volatile memory. */
	if (parse_binding_params_file(binding_filename, listener_streams_binding_params, MAX_LISTENER_STREAMS) < 0) {
			printf("failed to parse binding file %s\n", binding_filename);
			goto err;
	}

	/* Send them to the AVDECC stack. */
	for (i = 0; i < MAX_LISTENER_STREAMS; i++) {
		if (listener_streams_binding_params[i].talker_entity_id) {
			if (avdecc_listener_stream_bind_send_ipc(s_avdecc_handle, &listener_streams_binding_params[i]) < 0) {
				printf("failed to send binding params for stream %u\n", i);
				goto err;
			}
		}
	}

	return 0;

err:
	return -1;
}

/** Updates the file containing the saved binding params with new binding params. If entry existed it will be updated
 *  otherwise a new entry is created.
 * \return 	none
 * \param	binding_filename	path to the file used for saved binding parameters.
 * \param	binding_params		pointer to the genavb_msg_media_stack_bind struct received on bind event.
 */
void avdecc_nvm_bindings_update(const char *binding_filename, struct genavb_msg_media_stack_bind *binding_params)
{
	if (!binding_filename || !binding_params)
		return;

	nvram_update_entry(binding_filename, binding_params->entity_id, binding_params->listener_stream_index,
				binding_params->talker_entity_id, binding_params->talker_stream_index,
				binding_params->controller_entity_id, binding_params->started);

	printf("update %s with entry: EntityID %016"PRIx64" Listener unique ID %u Talker entity ID %016"PRIx64" Talker unique ID %u Controller entity ID%016"PRIx64" Started %u\n",
		binding_filename, binding_params->entity_id , binding_params->listener_stream_index,
		binding_params->talker_entity_id, binding_params->talker_stream_index,
		binding_params->controller_entity_id, binding_params->started);
}

/** Remove binding entry from the file containing the saved binding params.
 *
 * \return 	none
 * \param	binding_filename	path to the file used for saved binding parameters.
 * \param	unbinding_params	pointer to the genavb_msg_media_stack_unbind struct received on unbind event.
 */
void avdecc_nvm_bindings_remove(const char *binding_filename, struct genavb_msg_media_stack_unbind *unbinding_params)
{
	if (!binding_filename || !unbinding_params)
		return;

	nvram_update_entry(binding_filename, unbinding_params->entity_id, unbinding_params->listener_stream_index, 0, 0, 0, 0);

	printf("remove entry: EntityID %016"PRIx64" Listener unique ID %u from file %s\n",
		unbinding_params->entity_id , unbinding_params->listener_stream_index, binding_filename);
}
