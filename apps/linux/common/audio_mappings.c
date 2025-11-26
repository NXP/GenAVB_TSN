/*
 * Copyright 2024-2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

 #include <stdio.h>
 #include <string.h>
 #include <inttypes.h>

#include <genavb/aem.h>
#include <genavb/helpers.h>

#include "avdecc.h"

/* Make sure that maximum number of audio mappings does not exceed the IPC (and PDU) message size */
#if ((MAX_AUDIO_MAP_SIZE * 8 + 32) > AVB_AECP_MAX_MSG_SIZE)
#error Maximum audio mapping count exceeds size of AECP IPC and PDU
#endif

/* aecp_aem_get_audio_map_mappings_format's fields of audio_mapping_entry are stored in network order */
static struct audio_mapping_entry stream_port_input_audio_mappings[MAX_STREAM_PORT_INPUT][MAX_AUDIO_MAPS][MAX_AUDIO_MAP_SIZE];
static struct audio_mapping_entry stream_port_output_audio_mappings[MAX_STREAM_PORT_OUTPUT][MAX_AUDIO_MAPS][MAX_AUDIO_MAP_SIZE];

static char nvm_audio_mappings_file_name[FILENAME_MAX_LEN] = {'\0'};

static int get_free_audio_mapping_index(struct audio_mapping_entry *stream_port_audio_mappings)
{
	int index = -1;
	unsigned int i;

	for (i = 0; i < MAX_AUDIO_MAP_SIZE; i++) {
		if (!stream_port_audio_mappings[i].valid && !stream_port_audio_mappings[i].selected) {
			stream_port_audio_mappings[i].selected = true;
			index = i;
			goto exit;
		}
	}

exit:
	return index;
}

int audio_mappings_remove(avb_u16 desc_index, avb_u16 desc_type, avb_u16 mappings_count, void *audio_mappings)
{
	struct audio_mapping_entry (*stream_port_audio_mappings)[MAX_AUDIO_MAPS][MAX_AUDIO_MAP_SIZE];
	struct mapping_entry remove_entries[MAX_AUDIO_MAP_SIZE] = {{-1 , -1}};
	unsigned int remove_entries_count = 0;
	int rc = AECP_AEM_SUCCESS;
	bool found = false;
	unsigned int i, j;

	switch(desc_type) {
	case AEM_DESC_TYPE_STREAM_PORT_INPUT:
		stream_port_audio_mappings = stream_port_input_audio_mappings;

		if (desc_index >= MAX_STREAM_PORT_INPUT) {
			printf("%s: Error: Unsupported AEM_DESC_TYPE_STREAM_PORT_INPUT index(%u). Max supported(%u)\n", __func__, desc_index, MAX_STREAM_PORT_INPUT - 1);
			rc = AECP_AEM_NOT_SUPPORTED;
			goto exit;
		}

		break;

	case AEM_DESC_TYPE_STREAM_PORT_OUTPUT:
		stream_port_audio_mappings = stream_port_output_audio_mappings;

		if (desc_index >= MAX_STREAM_PORT_OUTPUT) {
			printf("%s: Error: Unsupported AEM_DESC_TYPE_STREAM_PORT_OUTPUT index(%u). Max supported(%u)\n", __func__, desc_index, MAX_STREAM_PORT_OUTPUT - 1);
			rc = AECP_AEM_NOT_SUPPORTED;
			goto exit;
		}

		break;

	default:
		printf("%s: Error: Unknown descriptor type(%u)\n", __func__, desc_type);
		rc = AECP_AEM_BAD_ARGUMENTS;
		goto exit;
	}

	for (i = 0; i < mappings_count; i++) {
		found = false;

		for (j = 0; j < MAX_AUDIO_MAP_SIZE; j++) {
			/* Only current valid audio mappings can possibly be removed */
			if (!stream_port_audio_mappings[desc_index][AUDIO_MAP_IDX][j].valid)
				continue;

			if (!memcmp(&((struct aecp_aem_get_audio_map_mappings_format *)audio_mappings)[i], &stream_port_audio_mappings[desc_index][AUDIO_MAP_IDX][j].mapping, sizeof(struct aecp_aem_get_audio_map_mappings_format))) {
				remove_entries[remove_entries_count++].registered_index = j;
				found = true;
				break;
			}
		}

		if (!found) {
			printf("%s: Error: AEM_DESC_TYPE_STREAM_PORT_%s index(%u) does not contain the mapping at index(%u) from the REMOVE_AUDIO_MAPPINGS command\n",
				__func__, (desc_type == AEM_DESC_TYPE_STREAM_PORT_INPUT) ? "INPUT" : "OUTPUT", desc_index, i);
			rc = AECP_AEM_BAD_ARGUMENTS;
			goto exit;
		}
	}

	for (i = 0; i < remove_entries_count; i++) {
		stream_port_audio_mappings[desc_index][AUDIO_MAP_IDX][remove_entries[i].registered_index].valid = false;
	}

	if (strlen(nvm_audio_mappings_file_name))
		avdecc_nvm_audio_mappings_remove(nvm_audio_mappings_file_name, desc_type, desc_index, AUDIO_MAP_IDX, remove_entries_count, remove_entries, stream_port_audio_mappings[desc_index][AUDIO_MAP_IDX]);

exit:
	return rc;
}

int audio_mappings_add(avb_u16 desc_index, avb_u16 desc_type, avb_u16 mappings_count, void *audio_mappings)
{
	struct audio_mapping_entry (*stream_port_audio_mappings)[MAX_AUDIO_MAPS][MAX_AUDIO_MAP_SIZE];
	avb_u16 mapping_stream_index, mapping_stream_channel, mapping_cluster_offset, mapping_cluster_channel;
	struct mapping_entry new_entries[MAX_AUDIO_MAP_SIZE] = {{-1 , -1}};
	unsigned int new_entries_count = 0;
	int rc = AECP_AEM_SUCCESS;
	unsigned int i, j;
	int index;

	switch(desc_type) {
	case AEM_DESC_TYPE_STREAM_PORT_INPUT:
		stream_port_audio_mappings = stream_port_input_audio_mappings;

		if (desc_index >= MAX_STREAM_PORT_INPUT) {
			printf("%s: Error: Unsupported AEM_DESC_TYPE_STREAM_PORT_INPUT index(%u). Max supported(%u)\n", __func__, desc_index, MAX_STREAM_PORT_INPUT - 1);
			rc = AECP_AEM_NOT_SUPPORTED;
			goto exit;
		}

		break;

	case AEM_DESC_TYPE_STREAM_PORT_OUTPUT:
		stream_port_audio_mappings = stream_port_output_audio_mappings;

		if (desc_index >= MAX_STREAM_PORT_OUTPUT) {
			printf("%s: Error: Unsupported AEM_DESC_TYPE_STREAM_PORT_OUTPUT index(%u). Max supported(%u)\n", __func__, desc_index, MAX_STREAM_PORT_OUTPUT - 1);
			rc = AECP_AEM_NOT_SUPPORTED;
			goto exit;
		}

		break;

	default:
		printf("%s: Error: Unknown descriptor type(%u)\n", __func__, desc_type);
		rc = AECP_AEM_BAD_ARGUMENTS;
		goto exit;
	}

	for (i = 0; i < mappings_count; i++) {
		/* Skip any duplicated mappings (in the command) that have already been processed */
		for (j = 0; j < i; j++) {
			if (!memcmp(&((struct aecp_aem_get_audio_map_mappings_format *)audio_mappings)[i],
				    &((struct aecp_aem_get_audio_map_mappings_format *)audio_mappings)[j],
				    sizeof(struct aecp_aem_get_audio_map_mappings_format)))
				goto next_mapping;
		}

		mapping_stream_index = ((struct aecp_aem_get_audio_map_mappings_format *)audio_mappings)[i].mapping_stream_index;
		mapping_stream_channel = ((struct aecp_aem_get_audio_map_mappings_format *)audio_mappings)[i].mapping_stream_channel;
		mapping_cluster_offset = ((struct aecp_aem_get_audio_map_mappings_format *)audio_mappings)[i].mapping_cluster_offset;
		mapping_cluster_channel = ((struct aecp_aem_get_audio_map_mappings_format *)audio_mappings)[i].mapping_cluster_channel;

		/* Skip mappings (in the command) for which there is already a valid and matching mapping */
		for (j = 0; j < MAX_AUDIO_MAP_SIZE; j++) {
			/* Only compare current valid audio mappings to the audio mappings of the command */
			if (!stream_port_audio_mappings[desc_index][AUDIO_MAP_IDX][j].valid)
				continue;

			if (!memcmp(&((struct aecp_aem_get_audio_map_mappings_format *)audio_mappings)[i],
				    &stream_port_audio_mappings[desc_index][AUDIO_MAP_IDX][j].mapping,
				    sizeof(struct aecp_aem_get_audio_map_mappings_format))) {
				/* The same audio mapping is already here and valid */
				goto next_mapping;
			}
		}

		/* At this point, the mapping is neither a duplicate from the command nor already matching an existing mappings.
		 * So, it will be either added as a new mapping or override (changes) an existing one.
		 */

		for (j = 0; j < MAX_AUDIO_MAP_SIZE; j++) {
			/* Only compare current valid audio mappings to the audio mappings of the command */
			if (!stream_port_audio_mappings[desc_index][AUDIO_MAP_IDX][j].valid)
				continue;

			/* The stack ensures (before passing the command to upper layers) that, for each audio mapping of a
			 * given redundant stream, the same mapping to all its redundant streams are present (even when the initial command omits that).
			 * That guarantees that all redundant mappings are sanitized and passed all at once in the same command and
			 * makes sure that all redundant mappings can directly be added/removed/overridden all at once here.
			 */

			if (desc_type == AEM_DESC_TYPE_STREAM_PORT_INPUT) {
				/* If the command, on a Stream Port Input, contains a mapping
				 * that references an existing mapping with the same cluster’s channel
				 * and two different stream’s channels that are not redundant,
				 * the app may accept the command and override the previous mapping.
				 * As per MILAN Specification v1.2 5.4.2.27
				 */
				if ((mapping_cluster_offset == stream_port_audio_mappings[desc_index][AUDIO_MAP_IDX][j].mapping.mapping_cluster_offset) &&
				    (mapping_cluster_channel == stream_port_audio_mappings[desc_index][AUDIO_MAP_IDX][j].mapping.mapping_cluster_channel) &&
				    ((mapping_stream_index != stream_port_audio_mappings[desc_index][AUDIO_MAP_IDX][j].mapping.mapping_stream_index) ||
				    (mapping_stream_channel != stream_port_audio_mappings[desc_index][AUDIO_MAP_IDX][j].mapping.mapping_stream_channel))) {
					new_entries[new_entries_count].registered_index = j;
					new_entries[new_entries_count].mapping_index = i;
					new_entries_count++;

					/* Temporarily invalidate overriden mapping, to be able to override
					 * all corresponding redundant mappings on the next loops.
					 * The command is sanitized before being sent to the apps,
					 * so there shouldn't more than one override per unique mapping anyway.
					 */
					stream_port_audio_mappings[desc_index][AUDIO_MAP_IDX][j].valid = false;

					goto next_mapping;
				}

			} else { /* AEM_DESC_TYPE_STREAM_PORT_OUTPUT */
				/* If the command, on a Stream Port Output, contains a mapping
				 * that references an existing mapping with the same stream’s channel
				 * and two different cluster’s channels that are not redundant,
				 * the app may accept the command and override the previous mapping.
				 * As per MILAN Specification v1.2 5.4.2.27
				 */
				if ((mapping_stream_index == stream_port_audio_mappings[desc_index][AUDIO_MAP_IDX][j].mapping.mapping_stream_index) &&
				    (mapping_stream_channel == stream_port_audio_mappings[desc_index][AUDIO_MAP_IDX][j].mapping.mapping_stream_channel) &&
				    ((mapping_cluster_offset != stream_port_audio_mappings[desc_index][AUDIO_MAP_IDX][j].mapping.mapping_cluster_offset) ||
				    (mapping_cluster_channel != stream_port_audio_mappings[desc_index][AUDIO_MAP_IDX][j].mapping.mapping_cluster_channel))) {
					new_entries[new_entries_count].registered_index = j;
					new_entries[new_entries_count].mapping_index = i;
					new_entries_count++;
					goto next_mapping;
				}
			}
		}

		index = get_free_audio_mapping_index(stream_port_audio_mappings[desc_index][AUDIO_MAP_IDX]);
		if (index < 0) {
			printf("%s: Error: AEM_DESC_TYPE_STREAM_PORT_%s index(%u)'s mappings are full. Max supported(%u)\n",
				__func__, (desc_type == AEM_DESC_TYPE_STREAM_PORT_INPUT) ? "INPUT" : "OUTPUT", desc_index, MAX_AUDIO_MAP_SIZE);
			rc = AECP_AEM_NOT_SUPPORTED;
			goto mappings_full; /* Unable to get a free entry from our audio mappings array */
		}

		/* Save the entry of the new audio mapping to add */
		new_entries[new_entries_count].registered_index = index;
		new_entries[new_entries_count].mapping_index = i;
		new_entries_count++;

next_mapping:
		continue;
	}

	/* Command is valid, update the audio mappings */
	for (i = 0; i < new_entries_count; i++) {
		memcpy(&stream_port_audio_mappings[desc_index][AUDIO_MAP_IDX][new_entries[i].registered_index].mapping,
			&((struct aecp_aem_get_audio_map_mappings_format *)audio_mappings)[new_entries[i].mapping_index],
			sizeof(struct aecp_aem_get_audio_map_mappings_format));

		stream_port_audio_mappings[desc_index][AUDIO_MAP_IDX][new_entries[i].registered_index].valid = true;
	}

	if (strlen(nvm_audio_mappings_file_name))
		avdecc_nvm_audio_mappings_update(nvm_audio_mappings_file_name, desc_type, desc_index, AUDIO_MAP_IDX, new_entries_count, new_entries, stream_port_audio_mappings[desc_index][AUDIO_MAP_IDX]);

mappings_full:
	/* Reset selected state of audio_mapping_entries, they either became valid or the status is AECP_AEM_NOT_SUPPORTED */
	for (i = 0; i < MAX_AUDIO_MAP_SIZE; i++) {
		stream_port_audio_mappings[desc_index][AUDIO_MAP_IDX][i].selected = false;
	}

exit:
	return rc;
}

int audio_mappings_get(avb_u16 desc_index, avb_u16 desc_type, avb_u16 map_index, avb_u16 *number_of_maps, avb_u16 *mappings_count, void *audio_mappings)
{
	struct audio_mapping_entry (*stream_port_audio_mappings)[MAX_AUDIO_MAPS][MAX_AUDIO_MAP_SIZE];
	avb_u16 mapping_index = 0;
	int rc = AECP_AEM_SUCCESS;
	unsigned int i;

	if (map_index >= MAX_AUDIO_MAPS) {
		printf("%s: Error: Unsupported MAP INDEX index(%u). Max supported(%u)\n", __func__, map_index, MAX_AUDIO_MAPS - 1);
		rc = AECP_AEM_BAD_ARGUMENTS;
		goto exit;
	}

	switch(desc_type) {
	case AEM_DESC_TYPE_STREAM_PORT_INPUT:
		stream_port_audio_mappings = stream_port_input_audio_mappings;

		if (desc_index >= MAX_STREAM_PORT_INPUT) {
			printf("%s: Error: Unsupported AEM_DESC_TYPE_STREAM_PORT_INPUT index(%u). Max supported(%u)\n", __func__, desc_index, MAX_STREAM_PORT_INPUT - 1);
			rc = AECP_AEM_NOT_SUPPORTED;
			goto exit;
		}

		break;

	case AEM_DESC_TYPE_STREAM_PORT_OUTPUT:
		stream_port_audio_mappings = stream_port_output_audio_mappings;

		if (desc_index >= MAX_STREAM_PORT_OUTPUT) {
			printf("%s: Error: Unsupported AEM_DESC_TYPE_STREAM_PORT_OUTPUT index(%u). Max supported(%u)\n", __func__, desc_index, MAX_STREAM_PORT_OUTPUT - 1);
			rc = AECP_AEM_NOT_SUPPORTED;
			goto exit;
		}

		break;

	default:
		printf("%s: Error: Unknown descriptor type(%u)\n", __func__, desc_type);
		rc = AECP_AEM_BAD_ARGUMENTS;
		goto exit;
	}

	for (i = 0; i < MAX_AUDIO_MAP_SIZE; i++) {
		if (stream_port_audio_mappings[desc_index][AUDIO_MAP_IDX][i].valid) {
			memcpy(&((struct aecp_aem_get_audio_map_mappings_format *)audio_mappings)[mapping_index], &stream_port_audio_mappings[desc_index][AUDIO_MAP_IDX][i].mapping, sizeof(struct aecp_aem_get_audio_map_mappings_format));
			mapping_index++;
		}
	}

	*mappings_count = mapping_index;
	*number_of_maps = MAX_AUDIO_MAPS;

exit:
	return rc;
}

int audio_mappings_set(avb_u16 desc_index, avb_u16 desc_type, avb_u16 audio_map_index, avb_u16 mapping_index, struct audio_mapping_entry *audio_mapping_entry)
{
	struct audio_mapping_entry (*stream_port_audio_mappings)[MAX_AUDIO_MAPS][MAX_AUDIO_MAP_SIZE];

	if (!audio_mapping_entry) {
		printf("%s: Error: Can not set an audio mapping to NULL\n", __func__);
		goto err;
	}

	switch(desc_type) {
	case AEM_DESC_TYPE_STREAM_PORT_INPUT:
		stream_port_audio_mappings = stream_port_input_audio_mappings;

		if (desc_index >= MAX_STREAM_PORT_INPUT) {
			printf("%s: Error: Unsupported AEM_DESC_TYPE_STREAM_PORT_INPUT index(%u). Max supported(%u)\n", __func__, desc_index, MAX_STREAM_PORT_INPUT - 1);
			goto err;
		}

		break;

	case AEM_DESC_TYPE_STREAM_PORT_OUTPUT:
		stream_port_audio_mappings = stream_port_output_audio_mappings;

		if (desc_index >= MAX_STREAM_PORT_OUTPUT) {
			printf("%s: Error: Unsupported AEM_DESC_TYPE_STREAM_PORT_OUTPUT index(%u). Max supported(%u)\n", __func__, desc_index, MAX_STREAM_PORT_OUTPUT - 1);
			goto err;
		}

		break;

	default:
		printf("%s: Error: Unknown descriptor type(%u)\n", __func__, desc_type);
		goto err;
	}

	if (audio_map_index >= MAX_AUDIO_MAPS) {
		printf("%s: Error: Unsupported audio map index(%u). Max supported(%u)\n", __func__, audio_map_index, MAX_AUDIO_MAPS - 1);
		goto err;
	}

	if (mapping_index >= MAX_AUDIO_MAP_SIZE) {
		printf("%s: Error: Unsupported audio mapping index(%u). Max supported(%u)\n", __func__, mapping_index, MAX_AUDIO_MAP_SIZE - 1);
		goto err;
	}

	memcpy(&stream_port_audio_mappings[desc_index][audio_map_index][mapping_index], audio_mapping_entry, sizeof(struct audio_mapping_entry));

	stream_port_audio_mappings[desc_index][audio_map_index][mapping_index].valid = true;
	stream_port_audio_mappings[desc_index][audio_map_index][mapping_index].selected = false;
	stream_port_audio_mappings[desc_index][audio_map_index][mapping_index].reserved = 0;

	return 0;

err:
	return -1;
}

int audio_mappings_set_nvm_file_name(const char *audio_mappings_file_name)
{
	if (!audio_mappings_file_name) {
		printf("%s: Error: Invalid audio mappings filename\n", __func__);
		goto err;
	}

	if (h_strncpy_strict(nvm_audio_mappings_file_name, audio_mappings_file_name, FILENAME_MAX_LEN) < 0) {
		printf("%s: Error: nvm_audio_mappings_file_name (%s) copy failed, max allowed characters (%u)\n", __func__, audio_mappings_file_name, FILENAME_MAX_LEN);
		goto err;
	}

	return 0;

err:
	return -1;
}

int audio_mappings_init(void)
{
	unsigned int i, j;
	int rc = AECP_AEM_SUCCESS;

	for (i = 0; i < MAX_STREAM_PORT_INPUT; i++) {
		for (j = 0; j < MAX_AUDIO_MAP_SIZE; j++) {
			stream_port_input_audio_mappings[i][AUDIO_MAP_IDX][j].valid = false;
			stream_port_input_audio_mappings[i][AUDIO_MAP_IDX][j].selected = false;
		}
	}

	for (i = 0; i < MAX_STREAM_PORT_OUTPUT; i++) {
		for (j = 0; j < MAX_AUDIO_MAP_SIZE; j++) {
			stream_port_output_audio_mappings[i][AUDIO_MAP_IDX][j].valid = false;
			stream_port_output_audio_mappings[i][AUDIO_MAP_IDX][j].selected = false;
		}
	}

	return rc;
}
