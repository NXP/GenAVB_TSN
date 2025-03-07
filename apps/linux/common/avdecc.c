/*
 * Copyright 2021-2024 NXP
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
#include <fcntl.h>
#include <libgen.h>

#include <genavb/genavb.h>
#include <genavb/helpers.h>
#include <genavb/aem.h>

#include "avdecc.h"

/* BINDING PARAMS ENTRY: [ListenerEntityID ListenerStreamIndex TalkerEntityID TalkerStreamIndex ControllerEntityID StreamStarted] */
#define NVRAM_BINDING_PARAMS_PER_ENTRY		6
#define MAX_LISTENER_STREAMS			16
#define TMP_BINDING_FILENAME			"/tmp_milan_binding_params"

/* AUDIO MAPPINGS ENTRY: [DescriptorType DescriptorIndex AudioMapIndex AudioMappingIndex StreamIndex StreamChannel ClusterOffset ClusterChannel] */
#define NVRAM_AUDIO_MAPPINGS_PER_ENTRY		8
#define NVRAM_AUDIO_MAPPINGS_MAX_ENTRIES	((MAX_STREAM_PORT_INPUT * MAX_AUDIO_MAPS * MAX_AUDIO_MAP_SIZE) + (MAX_STREAM_PORT_OUTPUT * MAX_AUDIO_MAPS * MAX_AUDIO_MAP_SIZE)) /* 128 */
#define TMP_AUDIO_MAPPINGS_FILENAME		"/tmp_audio_mappings"

/* PERSISTENT PARAMS ENTRY: [EntityID DescriptorType DescriptorIndex PersistentParamType PersistentParamValue] */
#define NVRAM_PERSISTENT_PARAMS_PER_ENTRY	5
#define NVRAM_PERSISTENT_PARAMS_MAX_ENTRIES	64
#define TMP_PERSISTENT_PARAMS_FILENAME		"/tmp_persistent_params"

#define NVRAM_ENTRY_MAX_LEN			256
#define STR_MAX_LEN				64

static int nvram_update_bindings_params(const char *binding_filename, avb_u64 entity_id, avb_u16 listener_stream_index, avb_u64 talker_entity_id, avb_u16 talker_stream_index, avb_u64 controller_entity_id, avb_u16 started)
{
	avb_u16 orig_listener_stream_index, orig_talker_stream_index, orig_started;
	avb_u64 orig_entity_id, orig_talker_entity_id, orig_controller_entity_id;
	char binding_filename_cpy[FILENAME_MAX_LEN];
	char tmp_filename[FILENAME_MAX_LEN];
	char new_entry[NVRAM_ENTRY_MAX_LEN];
	bool create_new_entry = false;
	char *parent_dirname = NULL;
	char *orig_entry = NULL;
	unsigned int read_size;
	int remaining_space;
	size_t len = 0;
	int rc = 0;
	FILE *fOrig;
	FILE *fTmp;
	int fParent;

	if (h_strncpy_strict(binding_filename_cpy, binding_filename, FILENAME_MAX_LEN) < 0) {
		printf("binding_filename (%s) copy failed, max allowed characters (%u)\n", binding_filename, FILENAME_MAX_LEN);
		goto err;
	}

	parent_dirname = dirname(binding_filename_cpy);

	/* Use the same parent directory as the binding file to keep the temporary file on the same filesystem, for the sake of rename() */
	if (h_strncpy_strict(tmp_filename, parent_dirname, FILENAME_MAX_LEN) < 0) {
		printf("copy of binding file dirname (%s) failed\n", parent_dirname);
		goto err;
	}

	remaining_space = FILENAME_MAX_LEN - (strlen(tmp_filename) + 1);

	if (remaining_space < strlen(TMP_BINDING_FILENAME)) {
		printf("tmp_filename %s %s is too long\n", parent_dirname, TMP_BINDING_FILENAME);
		goto err;
	}

	strncat(tmp_filename, TMP_BINDING_FILENAME, remaining_space);

	fParent = open(parent_dirname, O_RDONLY | O_DIRECTORY);
	if (fParent < 0){
		printf("open(%s) failed: %s\n", parent_dirname, strerror(errno));
		goto err;
	}

	fOrig = fopen(binding_filename, "r");
	if (fOrig == NULL) {
		printf("fopen(%s) failed: %s\n", binding_filename, strerror(errno));
		goto err_orig_file_open;
	}

	fTmp = fopen(tmp_filename, "w");
	if (fTmp == NULL) {
		printf("fopen(%s) failed: %s\n", tmp_filename, strerror(errno));
		goto err_tmp_file_open;
	}

	if (talker_entity_id) {
		create_new_entry = true;

		rc = snprintf(new_entry, NVRAM_ENTRY_MAX_LEN, "%016"PRIx64" %u %016"PRIx64" %u %016"PRIx64" %u\n",
			    entity_id , listener_stream_index, talker_entity_id,
			    talker_stream_index, controller_entity_id, started);

		if ((rc < 0) || (rc >= NVRAM_ENTRY_MAX_LEN)) {
			printf("Couldn't create the new entry: %s\n", strerror(errno));
			goto err_new_entry;
		}
	}

	while(((read_size = getline(&orig_entry, &len, fOrig)) != -1)) {
		if (read_size != strlen(orig_entry)) {
			printf("Unexpected embedded null byte(s)\n");
			goto err_read_entry;
		}

		if (sscanf(orig_entry, "%016"PRIx64" %hu %016"PRIx64" %hu %016"PRIx64" %hu", &orig_entity_id, &orig_listener_stream_index,
				&orig_talker_entity_id, &orig_talker_stream_index, &orig_controller_entity_id, &orig_started) == NVRAM_BINDING_PARAMS_PER_ENTRY) {

			if (orig_listener_stream_index == listener_stream_index && orig_entity_id == entity_id) {
				/* If matching listener entity ID and stream index, either remove or update it
				 * in place (no need for a new entry in file).
				 */
				create_new_entry = false;

				if (talker_entity_id) {
					/* If valid talker_entity_id, change the original entry with new one. */
					//printf("update new entry %s\n", new_entry);
					if (fputs(new_entry, fTmp) == EOF) {
						printf("Couldn't override the entry in the file: %s\n", strerror(errno));
						goto err_puts_entry;
					}
				} else {
					/* Zero talker_entity_id means remove original entry. */
					continue;
				}
			} else {
				//printf("keep original entry %s\n", orig_entry);
				if (fputs(orig_entry, fTmp) == EOF) {
					printf("Couldn't put the original entry in the file: %s\n", strerror(errno));
					goto err_puts_entry;
				}
			}
		}
	}

	/* Create new entry in file with valid binding params. */
	if (create_new_entry) {
		if (fputs(new_entry, fTmp) == EOF) {
			printf("Couldn't put the new entry in the file: %s\n", strerror(errno));
			goto err_puts_entry;
		}
	}

	rc = fflush(fTmp);
	if (rc < 0) {
		printf("fflush() failed, %s\n", strerror(errno));
		goto err_fflush;
	}

	fsync(fileno(fTmp));

	free(orig_entry);

	if (fclose(fTmp)) {
		printf("fclose(%s) failed\n", tmp_filename);
		goto err_tmp_file_close;
	}

	if (fclose(fOrig)) {
		printf("fclose(%s) failed\n", binding_filename);
		goto err_orig_file_close;
	}

	rc = rename(tmp_filename, binding_filename);
	if (rc < 0) {
		printf("rename() failed, %s\n", strerror(errno));
		goto err_rename;
	}

	/* fsync parent directory to make sure the rename went through to the disk */
	fsync(fParent);
	close(fParent);

	return 0;

err_fflush:
err_puts_entry:
err_read_entry:
	if (orig_entry)
		free(orig_entry);

err_new_entry:
	if (fclose(fTmp))
		printf("err_fclose(%s) failed\n", tmp_filename);

err_tmp_file_close:
err_tmp_file_open:
	if (fclose(fOrig))
		printf("err_fclose(%s) failed\n", binding_filename);

err_orig_file_close:
err_orig_file_open:
err_rename:
	close(fParent);
err:
	return -1;
}

static int nvram_update_audio_mappings(const char *audio_mappings_file_name, avb_u16 desc_type, avb_u16 desc_index, avb_u16 audio_map_index, unsigned int new_entries_count, struct mapping_entry *new_entries, struct audio_mapping_entry *stream_port_audio_mappings)
{
	avb_u16 orig_stream_index, orig_stream_channel, orig_cluster_offset, orig_cluster_channel;
	avb_u16 orig_desc_type, orig_desc_index, orig_audio_map_index, orig_mapping_index;
	avb_u16 stream_index, stream_channel, cluster_offset, cluster_channel;
	char audio_mappings_file_name_cpy[FILENAME_MAX_LEN];
	bool entries_found[MAX_AUDIO_MAP_SIZE] = {false};
	char tmp_filename[FILENAME_MAX_LEN];
	char new_entry[NVRAM_ENTRY_MAX_LEN];
	bool keep_orig_entry = false;
	unsigned int num_entries = 0;
	char *parent_dirname = NULL;
	char *orig_entry = NULL;
	unsigned int read_size;
	int remaining_space;
	unsigned int i;
	size_t len = 0;
	int fParent;
	FILE *fOrig;
	FILE *fTmp;
	int rc = 0;

	if (!audio_mappings_file_name) {
		printf("Invalid audio mappings file name\n");
		goto err;
	}

	if (!new_entries || !stream_port_audio_mappings) {
		printf("audio_mappings_file_name (%s) invalid audio mappings\n", audio_mappings_file_name);
		goto err;
	}

	if ((desc_type != AEM_DESC_TYPE_STREAM_PORT_INPUT) && (desc_type != AEM_DESC_TYPE_STREAM_PORT_OUTPUT)) {
		printf("audio_mappings_file_name (%s) unknown descriptor type(%u)\n", audio_mappings_file_name, desc_type);
		goto err;
	}

	if (new_entries_count > MAX_AUDIO_MAP_SIZE) {
		printf("audio_mappings_file_name (%s) new entries count(%u) exceeding max(%u) allowed\n", audio_mappings_file_name, new_entries_count, MAX_AUDIO_MAP_SIZE);
		goto err;
	}

	if (h_strncpy_strict(audio_mappings_file_name_cpy, audio_mappings_file_name, FILENAME_MAX_LEN) < 0) {
		printf("audio_mappings_file_name (%s) copy failed, max allowed characters (%u)\n", audio_mappings_file_name, FILENAME_MAX_LEN);
		goto err;
	}

	parent_dirname = dirname(audio_mappings_file_name_cpy);

	/* Use the same parent directory as the binding file to keep the temporary file on the same filesystem, for the sake of rename() */
	if (h_strncpy_strict(tmp_filename, parent_dirname, FILENAME_MAX_LEN) < 0) {
		printf("copy of audio_mappings file dirname (%s) failed\n", parent_dirname);
		goto err;
	}

	remaining_space = FILENAME_MAX_LEN - (strlen(tmp_filename) + 1);

	if (remaining_space < strlen(TMP_AUDIO_MAPPINGS_FILENAME)) {
		printf("tmp_filename %s %s is too long\n", parent_dirname, TMP_AUDIO_MAPPINGS_FILENAME);
		goto err;
	}

	strncat(tmp_filename, TMP_AUDIO_MAPPINGS_FILENAME, remaining_space);

	fParent = open(parent_dirname, O_RDONLY | O_DIRECTORY);
	if (fParent < 0){
		printf("open(%s) failed: %s\n", parent_dirname, strerror(errno));
		goto err;
	}

	fOrig = fopen(audio_mappings_file_name, "r");
	if (fOrig == NULL) {
		printf("fopen(%s) failed: %s\n", audio_mappings_file_name, strerror(errno));
		goto err_orig_file_open;
	}

	fTmp = fopen(tmp_filename, "w");
	if (fTmp == NULL) {
		printf("fopen(%s) failed: %s\n", tmp_filename, strerror(errno));
		goto err_tmp_file_open;
	}

	/* The audio mappings are saved in non-volatile memory when they've been modified.
	 * And only modified audio mappings are saved, the rest either have default values or have been removed.
	 */
	while(((read_size = getline(&orig_entry, &len, fOrig)) != -1)) {
		if (read_size != strlen(orig_entry)) {
			printf("Unexpected embedded null byte(s)\n");
			goto err_read_entry;
		}

		/* Read the current original audio mapping for a given stream_port and audio map */
		if (sscanf(orig_entry, "%hu %hu %hu %hu %hu %hu %hu %hu",
			    &orig_desc_type, &orig_desc_index, &orig_audio_map_index, &orig_mapping_index,
			    &orig_stream_index, &orig_stream_channel, &orig_cluster_offset, &orig_cluster_channel) == NVRAM_AUDIO_MAPPINGS_PER_ENTRY) {

			keep_orig_entry = true;

			/* Compare the current original audio mapping to the new entries of audio mapping.
			 * If the line matches one of the new entries, keep the new entry and put it in the file,
			 * otherwise don't override the original entry/line and keep it.
			 */
			for (i = 0; i < new_entries_count; i++) {
				if (entries_found[i])
					continue;

				if ((new_entries[i].registered_index >= MAX_AUDIO_MAP_SIZE) || (new_entries[i].registered_index < 0))
					continue;

				stream_index = ntohs(stream_port_audio_mappings[new_entries[i].registered_index].mapping.mapping_stream_index);
				stream_channel = ntohs(stream_port_audio_mappings[new_entries[i].registered_index].mapping.mapping_stream_channel);
				cluster_offset = ntohs(stream_port_audio_mappings[new_entries[i].registered_index].mapping.mapping_cluster_offset);
				cluster_channel = ntohs(stream_port_audio_mappings[new_entries[i].registered_index].mapping.mapping_cluster_channel);

				switch(desc_type) {
				case AEM_DESC_TYPE_STREAM_PORT_INPUT:
					if ((orig_desc_type == desc_type) && (orig_desc_index == desc_index) &&
					    (orig_audio_map_index == audio_map_index) && (orig_mapping_index == new_entries[i].registered_index) &&
					    (orig_cluster_offset == cluster_offset) && (orig_cluster_channel == cluster_channel))
						entries_found[i] = true;

					break;

				case AEM_DESC_TYPE_STREAM_PORT_OUTPUT:
					if ((orig_desc_type == desc_type) && (orig_desc_index == desc_index) &&
					    (orig_audio_map_index == audio_map_index) && (orig_mapping_index == new_entries[i].registered_index) &&
					    (orig_stream_index == stream_index) && (orig_stream_channel == stream_channel))
						entries_found[i] = true;

					break;

				default:
					/* Should not happen, desc_type is sanity tested at the start */
					goto err_param_type;
				}

				/* Keep the new entry and put it in the file instead of the original entry */
				if (entries_found[i]) {
					rc = snprintf(new_entry, NVRAM_ENTRY_MAX_LEN, "%hu %hu %hu %hu %hu %hu %hu %hu\n",
						    desc_type, desc_index, audio_map_index, new_entries[i].registered_index,
						    stream_index, stream_channel, cluster_offset, cluster_channel);

					if ((rc < 0) || (rc >= NVRAM_ENTRY_MAX_LEN)) {
						printf("Couldn't create the new entry %u: %s\n", num_entries, strerror(errno));
						goto err_new_entry;
					}

					if (fputs(new_entry, fTmp) == EOF) {
						printf("Couldn't override the entry %u in the file: %s\n", num_entries, strerror(errno));
						goto err_new_entry;
					}

					num_entries++;

					keep_orig_entry = false;

					break;
				}
			}

			/* Keep the original entry and put it in the file as it didn't match any new entry */
			if (keep_orig_entry) {
				if (fputs(orig_entry, fTmp) == EOF) {
					printf("Couldn't put the original entry %u in the file: %s\n", num_entries, strerror(errno));
					goto err_new_entry;
				}

				num_entries++;
			}
		}
	}

	/* Add the remaining new audio mappings that didn't override an already saved audio mapping */
	for (i = 0; i < new_entries_count; i++) {
		if (num_entries >= NVRAM_AUDIO_MAPPINGS_MAX_ENTRIES) {
			printf("audio_mappings_file_name (%s) reached max(%u) allowed of saved audio mappings\n", audio_mappings_file_name, NVRAM_AUDIO_MAPPINGS_MAX_ENTRIES);
			break;
		}

		if ((new_entries[i].registered_index >= MAX_AUDIO_MAP_SIZE) || (new_entries[i].registered_index < 0))
			continue;

		if (!entries_found[i]) {
			stream_index = ntohs(stream_port_audio_mappings[new_entries[i].registered_index].mapping.mapping_stream_index);
			stream_channel = ntohs(stream_port_audio_mappings[new_entries[i].registered_index].mapping.mapping_stream_channel);
			cluster_offset = ntohs(stream_port_audio_mappings[new_entries[i].registered_index].mapping.mapping_cluster_offset);
			cluster_channel = ntohs(stream_port_audio_mappings[new_entries[i].registered_index].mapping.mapping_cluster_channel);

			rc = snprintf(new_entry, NVRAM_ENTRY_MAX_LEN, "%hu %hu %hu %hu %hu %hu %hu %hu\n",
				    desc_type, desc_index, audio_map_index, new_entries[i].registered_index,
				    stream_index, stream_channel, cluster_offset, cluster_channel);

			if ((rc < 0) || (rc >= NVRAM_ENTRY_MAX_LEN)) {
				printf("Couldn't create the new entry %u: %s\n", num_entries, strerror(errno));
				goto err_new_entry;
			}

			if (fputs(new_entry, fTmp) == EOF) {
				printf("Couldn't put the new entry %u in the file: %s\n", num_entries, strerror(errno));
				goto err_new_entry;
			}

			num_entries++;
		}
	}

	rc = fflush(fTmp);
	if (rc < 0) {
		printf("fflush() failed, %s\n", strerror(errno));
		goto err_fflush;
	}

	fsync(fileno(fTmp));

	free(orig_entry);

	if (fclose(fTmp)) {
		printf("fclose(%s) failed\n", tmp_filename);
		goto err_tmp_file_close;
	}

	if (fclose(fOrig)) {
		printf("fclose(%s) failed\n", audio_mappings_file_name);
		goto err_orig_file_close;
	}

	rc = rename(tmp_filename, audio_mappings_file_name);
	if (rc < 0) {
		printf("rename() failed, %s\n", strerror(errno));
		goto err_rename;
	}

	/* fsync parent directory to make sure the rename went through to the disk */
	fsync(fParent);
	close(fParent);

	return 0;

err_fflush:
err_new_entry:
err_param_type:
err_read_entry:
	if (orig_entry)
		free(orig_entry);

	if (fclose(fTmp))
		printf("err_fclose(%s) failed rc = %d\n", tmp_filename, rc);

err_tmp_file_close:
err_tmp_file_open:
	if (fclose(fOrig))
		printf("err_fclose(%s) failed rc = %d\n", audio_mappings_file_name, rc);

err_orig_file_close:
err_orig_file_open:
err_rename:
	close(fParent);
err:
	return -1;
}

static int nvram_remove_audio_mappings(const char *audio_mappings_file_name, avb_u16 desc_type, avb_u16 desc_index, avb_u16 audio_map_index, unsigned int entries_count, struct mapping_entry *removed_entries, struct audio_mapping_entry *stream_port_audio_mappings)
{
	avb_u16 orig_stream_index, orig_stream_channel, orig_cluster_offset, orig_cluster_channel;
	avb_u16 orig_desc_type, orig_desc_index, orig_audio_map_index, orig_mapping_index;
	avb_u16 stream_index, stream_channel, cluster_offset, cluster_channel;
	char audio_mappings_file_name_cpy[FILENAME_MAX_LEN];
	char tmp_filename[FILENAME_MAX_LEN];
	bool keep_orig_entry = false;
	char *parent_dirname = NULL;
	char *orig_entry = NULL;
	unsigned int read_size;
	int remaining_space;
	unsigned int i;
	size_t len = 0;
	int fParent;
	FILE *fOrig;
	FILE *fTmp;
	int rc = 0;

	if (!audio_mappings_file_name) {
		printf("Invalid audio mappings file name\n");
		goto err;
	}

	if (!removed_entries || !stream_port_audio_mappings) {
		printf("audio_mappings_file_name (%s) invalid audio mappings\n", audio_mappings_file_name);
		goto err;
	}

	if ((desc_type != AEM_DESC_TYPE_STREAM_PORT_INPUT) && (desc_type != AEM_DESC_TYPE_STREAM_PORT_OUTPUT)) {
		printf("audio_mappings_file_name (%s) unknown descriptor type(%u)\n", audio_mappings_file_name, desc_type);
		goto err;
	}

	if (entries_count > MAX_AUDIO_MAP_SIZE) {
		printf("audio_mappings_file_name (%s) trying to remove more entries(%u) than the max(%u) number of entries allowed\n", audio_mappings_file_name, entries_count, MAX_AUDIO_MAP_SIZE);
		goto err;
	}

	if (h_strncpy_strict(audio_mappings_file_name_cpy, audio_mappings_file_name, FILENAME_MAX_LEN) < 0) {
		printf("audio_mappings_file_name (%s) copy failed, max allowed characters (%u)\n", audio_mappings_file_name, FILENAME_MAX_LEN);
		goto err;
	}

	parent_dirname = dirname(audio_mappings_file_name_cpy);

	/* Use the same parent directory as the binding file to keep the temporary file on the same filesystem, for the sake of rename() */
	if (h_strncpy_strict(tmp_filename, parent_dirname, FILENAME_MAX_LEN) < 0) {
		printf("copy of audio_mappings file dirname (%s) failed\n", parent_dirname);
		goto err;
	}

	remaining_space = FILENAME_MAX_LEN - (strlen(tmp_filename) + 1);

	if (remaining_space < strlen(TMP_AUDIO_MAPPINGS_FILENAME)) {
		printf("tmp_filename %s %s is too long\n", parent_dirname, TMP_AUDIO_MAPPINGS_FILENAME);
		goto err;
	}

	strncat(tmp_filename, TMP_AUDIO_MAPPINGS_FILENAME, remaining_space);

	fParent = open(parent_dirname, O_RDONLY | O_DIRECTORY);
	if (fParent < 0){
		printf("open(%s) failed: %s\n", parent_dirname, strerror(errno));
		goto err;
	}

	fOrig = fopen(audio_mappings_file_name, "r");
	if (fOrig == NULL) {
		printf("fopen(%s) failed: %s\n", audio_mappings_file_name, strerror(errno));
		goto err_orig_file_open;
	}

	fTmp = fopen(tmp_filename, "w");
	if (fTmp == NULL) {
		printf("fopen(%s) failed: %s\n", tmp_filename, strerror(errno));
		goto err_tmp_file_open;
	}

	while(((read_size = getline(&orig_entry, &len, fOrig)) != -1)) {
		if (read_size != strlen(orig_entry)) {
			printf("Unexpected embedded null byte(s)\n");
			goto err_read_entry;
		}

		/* Read the current original audio mapping for a given stream_port and audio map */
		if (sscanf(orig_entry, "%hu %hu %hu %hu %hu %hu %hu %hu",
			    &orig_desc_type, &orig_desc_index, &orig_audio_map_index, &orig_mapping_index,
			    &orig_stream_index, &orig_stream_channel, &orig_cluster_offset, &orig_cluster_channel) == NVRAM_AUDIO_MAPPINGS_PER_ENTRY) {

			keep_orig_entry = true;

			/* Compare the current original audio mapping to the entries of audio mapping.
			 * If the line matches exactly one of the entries, remove it.
			 */
			for (i = 0; i < entries_count; i++) {
				if ((removed_entries[i].registered_index >= MAX_AUDIO_MAP_SIZE) || (removed_entries[i].registered_index < 0))
					continue;

				stream_index = ntohs(stream_port_audio_mappings[removed_entries[i].registered_index].mapping.mapping_stream_index);
				stream_channel = ntohs(stream_port_audio_mappings[removed_entries[i].registered_index].mapping.mapping_stream_channel);
				cluster_offset = ntohs(stream_port_audio_mappings[removed_entries[i].registered_index].mapping.mapping_cluster_offset);
				cluster_channel = ntohs(stream_port_audio_mappings[removed_entries[i].registered_index].mapping.mapping_cluster_channel);

				if ((orig_desc_type == desc_type) && (orig_desc_index == desc_index) &&
				    (orig_audio_map_index == audio_map_index) && (orig_mapping_index == removed_entries[i].registered_index) &&
				    (orig_stream_index == stream_index) && (orig_stream_channel == stream_channel) &&
				    (orig_cluster_offset == cluster_offset) && (orig_cluster_channel == cluster_channel)) {
					keep_orig_entry = false;

					break;
				}
			}

			/* Keep the original entry and put it in the file as it didn't match any entry that has been removed */
			if (keep_orig_entry) {
				if (fputs(orig_entry, fTmp) == EOF) {
					printf("Couldn't put the original entry in the file: %s\n", strerror(errno));
					goto err_orig_entry;
				}
			}
		}
	}

	rc = fflush(fTmp);
	if (rc < 0) {
		printf("fflush() failed, %s\n", strerror(errno));
		goto err_fflush;
	}

	fsync(fileno(fTmp));

	free(orig_entry);

	if (fclose(fTmp)) {
		printf("fclose(%s) failed\n", tmp_filename);
		goto err_tmp_file_close;
	}

	if (fclose(fOrig)) {
		printf("fclose(%s) failed\n", audio_mappings_file_name);
		goto err_orig_file_close;
	}

	rc = rename(tmp_filename, audio_mappings_file_name);
	if (rc < 0) {
		printf("rename() failed, %s\n", strerror(errno));
		goto err_rename;
	}

	/* fsync parent directory to make sure the rename went through to the disk */
	fsync(fParent);
	close(fParent);

	return 0;

err_fflush:
err_orig_entry:
err_read_entry:
	if (orig_entry)
		free(orig_entry);

	if (fclose(fTmp))
		printf("err_fclose(%s) failed rc = %d\n", tmp_filename, rc);

err_tmp_file_close:
err_tmp_file_open:
	if (fclose(fOrig))
		printf("err_fclose(%s) failed rc = %d\n", audio_mappings_file_name, rc);

err_orig_file_close:
err_orig_file_open:
err_rename:
	close(fParent);
err:
	return -1;
}

static int nvram_update_persistent_params(const char *persistent_params_file_name, struct genavb_msg_media_stack_persistent_param *persistent_param)
{
	avb_u16 orig_desc_type, orig_desc_index;
	avb_u8 entry_value[STR_MAX_LEN];
	avb_u32 orig_param_type;
	avb_u64 orig_entity_id;
	char persistent_params_file_name_cpy[FILENAME_MAX_LEN];
	char tmp_filename[FILENAME_MAX_LEN];
	char new_entry[NVRAM_ENTRY_MAX_LEN];
	bool create_new_entry = false;
	char *parent_dirname = NULL;
	char *orig_entry = NULL;
	unsigned int read_size;
	int remaining_space;
	size_t len = 0;
	int fParent;
	FILE *fOrig;
	FILE *fTmp;
	int rc = 0;

	if (!persistent_params_file_name || !persistent_param) {
		printf("Invalid persistent_params_file_name or persistent_params\n");
		goto err;
	}

	if (h_strncpy_strict(persistent_params_file_name_cpy, persistent_params_file_name, FILENAME_MAX_LEN) < 0) {
		printf("persistent_params_file_name (%s) copy failed, max allowed characters (%u)\n", persistent_params_file_name, FILENAME_MAX_LEN);
		goto err;
	}

	parent_dirname = dirname(persistent_params_file_name_cpy);

	/* Use the same parent directory as the binding file to keep the temporary file on the same filesystem, for the sake of rename() */
	if (h_strncpy_strict(tmp_filename, parent_dirname, FILENAME_MAX_LEN) < 0) {
		printf("copy of persistent_params file dirname (%s) failed\n", parent_dirname);
		goto err;
	}

	remaining_space = FILENAME_MAX_LEN - (strlen(tmp_filename) + 1);

	if (remaining_space < strlen(TMP_PERSISTENT_PARAMS_FILENAME)) {
		printf("tmp_filename %s %s is too long\n", parent_dirname, TMP_PERSISTENT_PARAMS_FILENAME);
		goto err;
	}

	strncat(tmp_filename, TMP_PERSISTENT_PARAMS_FILENAME, remaining_space);

	fParent = open(parent_dirname, O_RDONLY | O_DIRECTORY);
	if (fParent < 0){
		printf("open(%s) failed: %s\n", parent_dirname, strerror(errno));
		goto err;
	}

	fOrig = fopen(persistent_params_file_name, "r");
	if (fOrig == NULL) {
		printf("fopen(%s) failed: %s\n", persistent_params_file_name, strerror(errno));
		goto err_orig_file_open;
	}

	fTmp = fopen(tmp_filename, "w");
	if (fTmp == NULL) {
		printf("fopen(%s) failed: %s\n", tmp_filename, strerror(errno));
		goto err_tmp_file_open;
	}

	create_new_entry = true;

	switch (persistent_param->param_type) {
	case AVDECC_PERSISTENT_PARAM_OBJECT_NAME:
	case AVDECC_PERSISTENT_PARAM_GROUP_NAME:
	case AVDECC_PERSISTENT_PARAM_ENTITY_NAME:
	{
		rc = snprintf(new_entry, NVRAM_ENTRY_MAX_LEN, "%016"PRIx64" %hu %hu %u %s\n",
			    persistent_param->entity_id, persistent_param->desc_type, persistent_param->desc_index,
			    persistent_param->param_type, persistent_param->u.name);

		if ((rc < 0) || (rc >= NVRAM_ENTRY_MAX_LEN)) {
			printf("Couldn't create the new entry for parameter type NAME: %s\n", strerror(errno));
			goto err_new_entry;
		}

		break;
	}
	case AVDECC_PERSISTENT_PARAM_CLOCK_SOURCE_INDEX:
	{
		rc = snprintf(new_entry, NVRAM_ENTRY_MAX_LEN, "%016"PRIx64" %hu %hu %u %hu\n",
			    persistent_param->entity_id, persistent_param->desc_type, persistent_param->desc_index,
			    persistent_param->param_type, persistent_param->u.clock_source_index);

		if ((rc < 0) || (rc >= NVRAM_ENTRY_MAX_LEN)) {
			printf("Couldn't create the new entry for parameter type CLOCK_SOURCE_INDEX: %s\n", strerror(errno));
			goto err_new_entry;
		}

		break;
	}
	case AVDECC_PERSISTENT_PARAM_FORMAT:
	{
		rc = snprintf(new_entry, NVRAM_ENTRY_MAX_LEN, "%016"PRIx64" %hu %hu %u %016"PRIx64"\n",
			    persistent_param->entity_id, persistent_param->desc_type, persistent_param->desc_index,
			    persistent_param->param_type, persistent_param->u.format);

		if ((rc < 0) || (rc >= NVRAM_ENTRY_MAX_LEN)) {
			printf("Couldn't create the new entry for parameter type FORMAT: %s\n", strerror(errno));
			goto err_new_entry;
		}

		break;
	}
	case AVDECC_PERSISTENT_PARAM_PRESENTATION_TIME_OFFSET:
	{
		rc = snprintf(new_entry, NVRAM_ENTRY_MAX_LEN, "%016"PRIx64" %hu %hu %u %u\n",
			    persistent_param->entity_id, persistent_param->desc_type, persistent_param->desc_index,
			    persistent_param->param_type, persistent_param->u.presentation_time_offset);

		if ((rc < 0) || (rc >= NVRAM_ENTRY_MAX_LEN)) {
			printf("Couldn't create the new entry for parameter type PRESENTATION_TIME_OFFSET: %s\n", strerror(errno));
			goto err_new_entry;
		}

		break;
	}
	case AVDECC_PERSISTENT_PARAM_SAMPLING_RATE:
	{
		rc = snprintf(new_entry, NVRAM_ENTRY_MAX_LEN, "%016"PRIx64" %hu %hu %u %u\n",
			    persistent_param->entity_id, persistent_param->desc_type, persistent_param->desc_index,
			    persistent_param->param_type, persistent_param->u.sampling_rate);

		if ((rc < 0) || (rc >= NVRAM_ENTRY_MAX_LEN)) {
			printf("Couldn't create the new entry for parameter type SAMPLING_RATE: %s\n", strerror(errno));
			goto err_new_entry;
		}

		break;
	}
	default:
		goto err_param_type;
	}

	while(((read_size = getline(&orig_entry, &len, fOrig)) != -1)) {
		if (read_size != strlen(orig_entry)) {
			printf("Unexpected embedded null byte(s)\n");
			goto err_read_entry;
		}

		if (sscanf(orig_entry, "%016"PRIx64" %hu %hu %u %s",
			    &orig_entity_id, &orig_desc_type, &orig_desc_index,
			    &orig_param_type, entry_value) == NVRAM_PERSISTENT_PARAMS_PER_ENTRY) {

			if ((orig_entity_id == persistent_param->entity_id) &&
			    (orig_desc_type == persistent_param->desc_type) &&
			    (orig_desc_index == persistent_param->desc_index) &&
			    (orig_param_type == persistent_param->param_type)) {
				/* Matching persistent parameter for a saved descriptor belonging to the same entity */
				create_new_entry = false;

				if (fputs(new_entry, fTmp) == EOF) {
					printf("Couldn't override the entry in the file: %s\n", strerror(errno));
					goto err_puts_entry;
				}
			} else {
				//printf("keep original entry %s\n", orig_entry);
				if (fputs(orig_entry, fTmp) == EOF) {
					printf("Couldn't put the original entry in the file: %s\n", strerror(errno));
					goto err_puts_entry;
				}
			}
		}
	}

	/* Create new entry in file with valid binding params. */
	if (create_new_entry) {
		if (fputs(new_entry, fTmp) == EOF) {
			printf("Couldn't put the new entry in the file: %s\n", strerror(errno));
			goto err_puts_entry;
		}
	}

	rc = fflush(fTmp);
	if (rc < 0) {
		printf("fflush() failed, %s\n", strerror(errno));
		goto err_fflush;
	}

	fsync(fileno(fTmp));

	free(orig_entry);

	if (fclose(fTmp)) {
		printf("fclose(%s) failed\n", tmp_filename);
		goto err_tmp_file_close;
	}

	if (fclose(fOrig)) {
		printf("fclose(%s) failed\n", persistent_params_file_name);
		goto err_orig_file_close;
	}

	rc = rename(tmp_filename, persistent_params_file_name);
	if (rc < 0) {
		printf("rename() failed, %s\n", strerror(errno));
		goto err_rename;
	}

	/* fsync parent directory to make sure the rename went through to the disk */
	fsync(fParent);
	close(fParent);

	return 0;

err_fflush:
err_puts_entry:
err_read_entry:
	if (orig_entry)
		free(orig_entry);

err_param_type:
err_new_entry:
	if (fclose(fTmp))
		printf("err_fclose(%s) failed\n", tmp_filename);

err_tmp_file_close:
err_tmp_file_open:
	if (fclose(fOrig))
		printf("err_fclose(%s) failed\n", persistent_params_file_name);

err_orig_file_close:
err_orig_file_open:
err_rename:
	close(fParent);
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
		printf("Invalid parameters\n");
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

	if (fclose(fBind)) {
		printf("fclose(%s) failed\n", binding_filename);
		rc = -1;
	}

err:
	return rc;
}

static int parse_audio_mappings_file(const char *audio_mappings_file_name)
{
	avb_u16 desc_type, desc_index, audio_map_index, mapping_index;
	struct audio_mapping_entry audio_mapping_entry;
	char *nvram_entry = NULL;
	unsigned int read_char;
	size_t len = 0;
	FILE *fAudMap;
	int rc = 0;

	if (!audio_mappings_file_name) {
		printf("Invalid parameters\n");
		rc = -1;
		goto err;
	}

	fAudMap = fopen(audio_mappings_file_name, "a+");
	if (fAudMap == NULL) {
		printf("fopen(%s) failed: %s\n", audio_mappings_file_name, strerror(errno));
		rc = -1;
		goto err;
	}

	printf("audio mappings file name: %s\n", audio_mappings_file_name);

	/* read all entries in the nvram file one by one. if stream index matches read it */
	while (((read_char = getline(&nvram_entry, &len, fAudMap)) != -1)) {
		if (read_char != strlen(nvram_entry)) {
			printf("Unexpected embedded null byte(s)\n");
			rc = -1;
			goto err_read_nvram;
		}

		if (sscanf(nvram_entry, "%hu %hu %hu %hu %hu %hu %hu %hu",
			    &desc_type, &desc_index, &audio_map_index, &mapping_index,
			    &audio_mapping_entry.mapping.mapping_stream_index, &audio_mapping_entry.mapping.mapping_stream_channel,
			    &audio_mapping_entry.mapping.mapping_cluster_offset, &audio_mapping_entry.mapping.mapping_cluster_channel) == NVRAM_AUDIO_MAPPINGS_PER_ENTRY) {
			audio_mapping_entry.valid = false;
			audio_mapping_entry.selected = false;
			audio_mapping_entry.reserved = 0;

			audio_mapping_entry.mapping.mapping_stream_index = htons(audio_mapping_entry.mapping.mapping_stream_index);
			audio_mapping_entry.mapping.mapping_stream_channel = htons(audio_mapping_entry.mapping.mapping_stream_channel);
			audio_mapping_entry.mapping.mapping_cluster_offset = htons(audio_mapping_entry.mapping.mapping_cluster_offset);
			audio_mapping_entry.mapping.mapping_cluster_channel = htons(audio_mapping_entry.mapping.mapping_cluster_channel);

			audio_mappings_set(desc_index, desc_type, audio_map_index, mapping_index, &audio_mapping_entry);
		} else
			printf("error: can not read data from nvram\n");
	}

err_read_nvram:
	free(nvram_entry);

	if (fclose(fAudMap)) {
		printf("fclose(%s) failed\n", audio_mappings_file_name);
		rc = -1;
	}

err:
	return rc;
}

static int parse_persistent_params_file(const char *persistent_params_file_name, struct genavb_msg_media_stack_persistent_param *persistent_params, unsigned int size)
{
	avb_u16 desc_type, desc_index;
	avb_u32 param_type;
	avb_u64 entity_id;
	unsigned long long entry_value_ull;
	char entry_value[STR_MAX_LEN];
	unsigned long entry_value_ul;
	char *nvram_entry = NULL;
	unsigned int read_char;
	unsigned int i = 0;
	size_t len = 0;
	FILE *fPersist;
	int rc = 0;

	if (!persistent_params_file_name || !persistent_params || !size) {
		printf("Invalid parameters\n");
		rc = -1;
		goto err;
	}

	fPersist = fopen(persistent_params_file_name, "a+");
	if (fPersist == NULL) {
		printf("fopen(%s) failed: %s\n", persistent_params_file_name, strerror(errno));
		rc = -1;
		goto err;
	}

	printf("Persistent parameters file name: %s\n", persistent_params_file_name);

	/* read all entries in the nvram file one by one */
	while (((read_char = getline(&nvram_entry, &len, fPersist)) != -1)) {
		if (read_char != strlen(nvram_entry)) {
			printf("Unexpected embedded null byte(s)\n");
			rc = -1;
			goto err_read_nvram;
		}

		if (sscanf(nvram_entry, "%016"PRIx64" %hu %hu %u %s",
			    &entity_id, &desc_type, &desc_index, &param_type, entry_value) == NVRAM_PERSISTENT_PARAMS_PER_ENTRY) {

			printf("Read EntityID %016"PRIx64" descType %hu descID %hu paramType %u value %s\n",
				entity_id, desc_type, desc_index, param_type, entry_value);

			if (i < size) {
				switch (param_type) {
				case AVDECC_PERSISTENT_PARAM_OBJECT_NAME:
				case AVDECC_PERSISTENT_PARAM_GROUP_NAME:
				case AVDECC_PERSISTENT_PARAM_ENTITY_NAME:
				{
					memcpy(persistent_params[i].u.name, entry_value, STR_MAX_LEN);
					break;
				}
				case AVDECC_PERSISTENT_PARAM_CLOCK_SOURCE_INDEX:
				{
					if (h_strtoul(&entry_value_ul, entry_value, NULL, 0) < 0) {
						rc = -1;
						goto err_param_val;
					}

					persistent_params[i].u.clock_source_index = (avb_u16)entry_value_ul;
					break;
				}
				case AVDECC_PERSISTENT_PARAM_FORMAT:
				{
					if (h_strtoull(&entry_value_ull, entry_value, NULL, 16) < 0) {
						rc = -1;
						goto err_param_val;
					}

					persistent_params[i].u.format = (avb_u64)entry_value_ull;
					break;
				}
				case AVDECC_PERSISTENT_PARAM_PRESENTATION_TIME_OFFSET:
				{
					if (h_strtoul(&entry_value_ul, entry_value, NULL, 0) < 0) {
						rc = -1;
						goto err_param_val;
					}

					persistent_params[i].u.presentation_time_offset = (avb_u32)entry_value_ul;
					break;
				}
				case AVDECC_PERSISTENT_PARAM_SAMPLING_RATE:
				{
					if (h_strtoul(&entry_value_ul, entry_value, NULL, 0) < 0) {
						rc = -1;
						goto err_param_val;
					}

					persistent_params[i].u.sampling_rate = (avb_u32)entry_value_ul;
					break;
				}
				default:
					printf("Unknown persistent parameters type %u\n", param_type);
					rc = -1;
					goto err_param_type;
				}

				persistent_params[i].entity_id = entity_id;
				persistent_params[i].desc_type = desc_type;
				persistent_params[i].desc_index = desc_index;
				persistent_params[i].param_type = param_type;

				i++;
			} else {
				printf("Reached maximum persistent parameters %u, skip any remaining parameters\n", size);
				goto exit;
			}

		} else
			printf("error: can not read data from nvram\n");
	}

exit:
err_param_type:
err_param_val:
err_read_nvram:
	free(nvram_entry);

	if (fclose(fPersist)) {
		printf("fclose(%s) failed\n", persistent_params_file_name);
		rc = -1;
	}

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

static int avdecc_persistent_params_send_ipc(struct avb_control_handle *s_avdecc_handle, struct genavb_msg_media_stack_persistent_param *persistent_param)
{
	struct genavb_msg_media_stack_persistent_param media_stack_persistent_param;
	unsigned int msg_len = sizeof(media_stack_persistent_param);
	genavb_msg_type_t msg_type = GENAVB_MSG_MEDIA_STACK_PERSISTENT_PARAM;
	int rc = AVB_SUCCESS;

	if (!persistent_param || !s_avdecc_handle) {
		rc = AVB_ERR_INVALID_PARAMS;
		goto exit;
	}

	switch (persistent_param->param_type) {
	case AVDECC_PERSISTENT_PARAM_OBJECT_NAME:
	case AVDECC_PERSISTENT_PARAM_GROUP_NAME:
	case AVDECC_PERSISTENT_PARAM_ENTITY_NAME:
	{
		memcpy(media_stack_persistent_param.u.name, persistent_param->u.name, STR_MAX_LEN);
		break;
	}
	case AVDECC_PERSISTENT_PARAM_CLOCK_SOURCE_INDEX:
	{
		media_stack_persistent_param.u.clock_source_index = persistent_param->u.clock_source_index;
		break;
	}
	case AVDECC_PERSISTENT_PARAM_FORMAT:
	{
		media_stack_persistent_param.u.format = persistent_param->u.format;
		break;
	}
	case AVDECC_PERSISTENT_PARAM_PRESENTATION_TIME_OFFSET:
	{
		media_stack_persistent_param.u.presentation_time_offset = persistent_param->u.presentation_time_offset;
		break;
	}
	case AVDECC_PERSISTENT_PARAM_SAMPLING_RATE:
	{
		media_stack_persistent_param.u.sampling_rate = persistent_param->u.sampling_rate;
		break;
	}
	default:
		printf("Unknown persistent parameters type %u\n", persistent_param->param_type);
		rc = AVB_ERR_INVALID_PARAMS;
		goto exit;
	}

	media_stack_persistent_param.entity_id = persistent_param->entity_id;
	media_stack_persistent_param.desc_type = persistent_param->desc_type;
	media_stack_persistent_param.desc_index = persistent_param->desc_index;
	media_stack_persistent_param.param_type = persistent_param->param_type;

	rc = avb_control_send(s_avdecc_handle, msg_type, &media_stack_persistent_param, msg_len);
	if (rc != AVB_SUCCESS) {
		printf("avb_control_send (GENAVB_CTRL_AVDECC_MEDIA_STACK) failed: %s\n", avb_strerror(rc));
		goto exit;
	}

	printf("Sent persistent parameter: EntityID %016"PRIx64" descType %hu descID %hu paramType %u\n",
		persistent_param->entity_id, persistent_param->desc_type, persistent_param->desc_index, persistent_param->param_type);

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

/** Parses the file containing the saved audio mappings and init the default audio mappings with them.
 * An audio mapping is saved if it has been modified at least once and is still valid,
 * otherwise initializing the audio mapping with its default value is enough
 *
 * \return 0 on success, negative otherwise.
 * \param	ctrl_h				Handle to an avb_control_handle, opened for an AVB_CTRL_AVDECC_MEDIA_STACK channel.
 * \param	audio_mappings_file_name	path to the file used for saved audio mappings.
 */
int avdecc_nvm_audio_mappings_init(struct avb_control_handle *s_avdecc_handle, const char *audio_mappings_file_name)
{
	if (!s_avdecc_handle) {
		printf("Invalid avb control handle\n");
		goto err;
	}

	if (!audio_mappings_file_name) {
		printf("Invalid audio mappings filename\n");
		goto err;
	}

	if (audio_mappings_set_nvm_file_name(audio_mappings_file_name) < 0) {
		printf("failed to save audio mappings filename %s\n", audio_mappings_file_name);
		goto err;
	}

	/* Get saved audio mappings from non-volatile memory. */
	if (parse_audio_mappings_file(audio_mappings_file_name) < 0) {
		printf("failed to parse audio mappings file %s\n", audio_mappings_file_name);
		goto err;
	}

	return 0;

err:
	return -1;
}

/** Parses the file containing the saved persistent params and init the avdecc stack with it
 *
 * \return 0 on success, negative otherwise.
 * \param	ctrl_h				Handle to an avb_control_handle, opened for an AVB_CTRL_AVDECC_MEDIA_STACK channel.
 * \param	persistent_params_file_name	path to the file used for saved persistent parameters.
 */
int avdecc_nvm_persistent_params_init(struct avb_control_handle *s_avdecc_handle, const char *persistent_params_file_name)
{
	struct genavb_msg_media_stack_persistent_param persistent_params[NVRAM_PERSISTENT_PARAMS_MAX_ENTRIES] = {0};
	int i;

	if (!s_avdecc_handle) {
		printf("Invalid avb control handle\n");
		goto err;
	}

	if (!persistent_params_file_name) {
		printf("Invalid persistent parameters filename\n");
		goto err;
	}

	/* Get saved persistent parameters from non-volatile memory. */
	if (parse_persistent_params_file(persistent_params_file_name, persistent_params, NVRAM_PERSISTENT_PARAMS_MAX_ENTRIES) < 0) {
		printf("failed to parse persistent parameters file %s\n", persistent_params_file_name);
		goto err;
	}

	/* Send persistent parameters to the AVDECC stack. */
	for (i = 0; i < NVRAM_PERSISTENT_PARAMS_MAX_ENTRIES; i++) {
		if (persistent_params[i].entity_id) {
			if (avdecc_persistent_params_send_ipc(s_avdecc_handle, &persistent_params[i]) < 0) {
				printf("failed to send persistent params %u\n", i);
				goto err;
			}
		} else {
			/* No more persistent parameters to send */
			break;
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

	nvram_update_bindings_params(binding_filename, binding_params->entity_id, binding_params->listener_stream_index,
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

	nvram_update_bindings_params(binding_filename, unbinding_params->entity_id, unbinding_params->listener_stream_index, 0, 0, 0, 0);

	printf("remove entry: EntityID %016"PRIx64" Listener unique ID %u from file %s\n",
		unbinding_params->entity_id , unbinding_params->listener_stream_index, binding_filename);
}

/** Updates the file containing the saved audio mappings with new ones.
 * If entry existed it will be updated, otherwise a new entry is created.
 * \return 	none
 * \param	audio_mappings_file_name	path to the file used for saved audio mappings.
 * \param	desc_type			descriptor's type concerned by the audio mappings. Either a stream_port_input or a stream_port_output.
 * \param	desc_index			descriptor's index concerned by the audio mappings.
 * \param	audio_map_index			index of the audio map holding the audio mappings.
 * \param	new_entries_count		number of updated/new audio mapping entries.
 * \param	new_entries			new audio mapping's indexes.
 * \param	stream_port_audio_mappings	audio mappings of the stream_port descriptor for the current (audio_map_index) audio map.
 */
void avdecc_nvm_audio_mappings_update(const char *audio_mappings_file_name, avb_u16 desc_type, avb_u16 desc_index, avb_u16 audio_map_index, unsigned int new_entries_count, struct mapping_entry *new_entries, struct audio_mapping_entry *stream_port_audio_mappings)
{
	if (!audio_mappings_file_name || !new_entries || !stream_port_audio_mappings)
		return;

	nvram_update_audio_mappings(audio_mappings_file_name, desc_type, desc_index,
				    audio_map_index, new_entries_count, new_entries,
				    stream_port_audio_mappings);

	printf("update %s with %u entries for descType %hu descID %hu audioMapID %hu\n",
		audio_mappings_file_name, new_entries_count, desc_type, desc_index, audio_map_index);
}

/** Remove audio mapping entries from the file containing the saved audio mappings.
 *
 * \return 	none
 * \param	audio_mappings_file_name	path to the file used for saved audio mappings.
 * \param	desc_type			descriptor's type concerned by the audio mappings. Either a stream_port_input or a stream_port_output.
 * \param	desc_index			descriptor's index concerned by the audio mappings.
 * \param	audio_map_index			index of the audio map holding the audio mappings.
 * \param	entries_count			number of audio mappings that were removed.
 * \param	removed_entries			removed audio mapping's indexes.
 * \param	stream_port_audio_mappings	audio mappings of the stream_port descriptor for the current (audio_map_index) audio map.
 */
void avdecc_nvm_audio_mappings_remove(const char *audio_mappings_file_name, avb_u16 desc_type, avb_u16 desc_index, avb_u16 audio_map_index, unsigned int entries_count, struct mapping_entry *removed_entries, struct audio_mapping_entry *stream_port_audio_mappings)
{
	if (!audio_mappings_file_name || !removed_entries || !stream_port_audio_mappings)
		return;

	nvram_remove_audio_mappings(audio_mappings_file_name, desc_type, desc_index,
				    audio_map_index, entries_count, removed_entries,
				    stream_port_audio_mappings);

	printf("update %s, removed %u entries for descType %hu descID %hu audioMapID %hu\n",
		audio_mappings_file_name, entries_count, desc_type, desc_index, audio_map_index);
}

/** Updates the file containing the saved persistent params with new ones. If entry existed it will be updated
 *  otherwise a new entry is created.
 * \return 	none
 * \param	persistent_params_file_name	path to the file used for saved persistent parameters.
 * \param	persistent_param		pointer to the genavb_msg_media_stack_persistent_param struct received upon one of a descriptor's persistent param update.
 */
void avdecc_nvm_persistent_params_update(const char *persistent_params_file_name, struct genavb_msg_media_stack_persistent_param *persistent_param)
{
	if (!persistent_params_file_name || !persistent_param)
		return;

	nvram_update_persistent_params(persistent_params_file_name, persistent_param);

	printf("update %s with entry: EntityID %016"PRIx64" Descriptor (%u, %u)\n",
		persistent_params_file_name, persistent_param->entity_id, persistent_param->desc_type, persistent_param->desc_index);
}
