/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *    Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *    Neither the name of NXP Semiconductors nor the
 *    names of its contributors may be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 @file
 @brief AECP protocol helpers
 @details

 Copyright 2015 Freescale Semiconductor, Inc.
 All Rights Reserved.
*/

#ifndef _COMMON_AECP_H_
#define _COMMON_AECP_H_

#include <genavb/genavb.h>

int aecp_aem_send_set_control(struct avb_control_handle *ctrl_h, avb_u64 *entity_id, int unsolicited, avb_u16 descriptor_index, void *value, avb_u16 *len, avb_u8 *status, int sync);
int aecp_aem_send_set_control_single_u8(struct avb_control_handle *ctrl_h, avb_u64 *entity_id, avb_u16 descriptor_index, avb_u8 value);
int aecp_aem_send_set_control_single_u8_command(struct avb_control_handle *ctrl_h, avb_u64 *entity_id, avb_u16 descriptor_index, avb_u8 *value, avb_u8 *status, int sync);
int aecp_aem_send_set_control_single_u8_response(struct avb_control_handle *ctrl_h, avb_u16 descriptor_index, avb_u8 *value);
int aecp_aem_send_set_control_single_u8_unsolicited_response(struct avb_control_handle *ctrl_h, avb_u16 descriptor_index, avb_u8 *value);
int aecp_aem_send_set_control_utf8(struct avb_control_handle *ctrl_h, avb_u64 *entity_id, avb_u16 descriptor_index, const char *value);
int aecp_aem_send_set_control_utf8_command(struct avb_control_handle *ctrl_h, avb_u64 *entity_id, avb_u16 descriptor_index, const char *value, avb_u8 *status, int sync);
int aecp_aem_send_set_control_utf8_response(struct avb_control_handle *ctrl_h, avb_u16 descriptor_index, const char *value);
int aecp_aem_send_set_control_utf8_unsolicited_response(struct avb_control_handle *ctrl_h, avb_u16 descriptor_index, const char *value);
int aecp_aem_send_get_control(struct avb_control_handle *ctrl_h, avb_u64 *entity_id, avb_u16 descriptor_index, void *value, avb_u16 *len, avb_u8 *status, int sync);
int aecp_aem_acquire_entity(struct avb_control_handle *ctrl_h, avb_u64 *entity_id, int acquire);
int aecp_aem_send_read_descriptor(struct avb_control_handle *ctrl_h, avb_u64 *entity_id, avb_u16 configuration_index, avb_u16 descriptor_type, avb_u16 descriptor_index, void *buffer, avb_u16 *len, avb_u8 *status, int sync);
int aecp_aem_send_start_streaming(struct avb_control_handle *ctrl_h, avb_u64 *entity_id, avb_u16 descriptor_type, avb_u16 descriptor_index, avb_u8 *status, int sync);
int aecp_aem_send_stop_streaming(struct avb_control_handle *ctrl_h, avb_u64 *entity_id, avb_u16 descriptor_type, avb_u16 descriptor_index, avb_u8 *status, int sync);
int avdecc_fmt_pretty_printf(const struct avdecc_format *format, char *str, size_t size);

#endif /* _COMMON_AECP_H_ */
