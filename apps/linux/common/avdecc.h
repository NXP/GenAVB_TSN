/*
 * Copyright 2021, 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _COMMON_AVDECC_H_
#define _COMMON_AVDECC_H_

#include <genavb/genavb.h>

#include "audio_mappings.h"

#define FILENAME_MAX_LEN	256

int avdecc_nvm_bindings_init(struct avb_control_handle *s_avdecc_handle, const char *binding_filename);
int avdecc_nvm_audio_mappings_init(struct avb_control_handle *s_avdecc_handle, const char *audio_mappings_file_name);
int avdecc_nvm_persistent_params_init(struct avb_control_handle *s_avdecc_handle, const char *persistent_params_file_name);
void avdecc_nvm_bindings_update(const char *binding_filename, struct genavb_msg_media_stack_bind *binding_params);
void avdecc_nvm_bindings_remove(const char *binding_filename, struct genavb_msg_media_stack_unbind *unbinding_params);
void avdecc_nvm_audio_mappings_update(const char *audio_mappings_file_name, avb_u16 desc_type, avb_u16 desc_index, avb_u16 audio_map_index, unsigned int new_entries_count, struct mapping_entry *new_entries, struct audio_mapping_entry *stream_port_audio_mappings);
void avdecc_nvm_audio_mappings_remove(const char *audio_mappings_file_name, avb_u16 desc_type, avb_u16 desc_index, avb_u16 audio_map_index, unsigned int entries_count, struct mapping_entry *removed_entries, struct audio_mapping_entry *stream_port_audio_mappings);
void avdecc_nvm_persistent_params_update(const char *persistent_params_file_name, struct genavb_msg_media_stack_persistent_param *persistent_param);
#endif /* _COMMON_AVDECC_H_ */
