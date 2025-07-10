/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __AUDIO_MAPPINGS_H__
#define __AUDIO_MAPPINGS_H__

#include <genavb/genavb.h>

#define MAX_STREAM_PORT_INPUT	4
#define MAX_STREAM_PORT_OUTPUT	4
#define MAX_AUDIO_MAPS		1 /* Use a single audio map for now to store all audio mappings for a single STREAM_PORT_INPUT/OUTPUT */
#define MAX_AUDIO_MAP_SIZE	16 /* Maximum number of audio mappings in a single audio map */
#define AUDIO_MAP_IDX		0

struct audio_mapping_entry {
	struct aecp_aem_get_audio_map_mappings_format mapping; /* mapping is in network order */
	avb_u8 valid:1;
	avb_u8 selected:1;
	avb_u8 reserved:6;
};

struct mapping_entry {
	int registered_index; /* index in the registered array */
	int mapping_index;   /* index in the received mappings array */
};

int audio_mappings_init(void);
int audio_mappings_remove(avb_u16 desc_index, avb_u16 desc_type, avb_u16 mappings_count, void *audio_mappings);
int audio_mappings_add(avb_u16 desc_index, avb_u16 desc_type, avb_u16 mappings_count, void *audio_mappings);
int audio_mappings_get(avb_u16 desc_index, avb_u16 desc_type, avb_u16 map_index, avb_u16 *number_of_maps, avb_u16 *mappings_count, void *audio_mappings);
int audio_mappings_set(avb_u16 desc_index, avb_u16 desc_type, avb_u16 audio_map_index, avb_u16 mapping_index, struct audio_mapping_entry *audio_mapping_entry);
int audio_mappings_set_nvm_file_name(const char *audio_mappings_file_name);

#endif /* __AUDIO_MAPPINGS_H__ */
