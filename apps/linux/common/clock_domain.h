/*
 * Copyright 2016 Freescale Semiconductor, Inc.
 * Copyright 2017 NXP
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

#ifndef _CLOCK_DOMAIN_H_
#define _CLOCK_DOMAIN_H_

typedef enum {
	MEDIA_CLOCK_MASTER,
	MEDIA_CLOCK_SLAVE
}media_clock_role_t;

const char* get_genavb_clock_domain_t_name(genavb_clock_domain_t type);

const char* get_genavb_clock_source_type_t_name(genavb_clock_source_type_t type);

const char* get_genavb_clock_source_local_id_t_name(genavb_clock_source_local_id_t type);

const char* get_genavb_clock_domain_status_t_name(genavb_clock_domain_status_t type);

int get_clk_domain_validity(avb_clock_domain_t clk_domain);

void set_clk_domain_validity(avb_clock_domain_t clk_domain, int validity);

int handle_clock_domain_event(void);

void clock_domain_get_status(avb_clock_domain_t domain);

int get_audio_clk_sync(avb_clock_domain_t clk_domain);

void set_audio_clk_sync(avb_clock_domain_t clk_domain, int clk_sync);

int clock_domain_set_source_stream(avb_clock_domain_t domain, struct avb_stream_params *stream_params);

int clock_domain_set_source_internal(avb_clock_domain_t domain, avb_clock_source_local_id_t local_id);

int clock_domain_set_role(media_clock_role_t role, avb_clock_domain_t domain, struct avb_stream_params *stream_params);

int clock_domain_init(struct avb_handle *s_avb_handle, int *clk_fd);

int clock_domain_exit(void);

#endif /* _CLOCK_DOMAIN_H_ */
