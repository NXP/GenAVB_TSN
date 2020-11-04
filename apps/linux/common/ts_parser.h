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

#ifndef _TS_PARSER_H_
#define _TS_PARSER_H_

#include <sys/time.h>
#include <genavb/genavb.h>

#include "file_buffer.h"

#define PES_SIZE	188

#ifdef __cplusplus
extern "C" {
#endif

struct ts_parser {
	uint64_t t0_ns;
	unsigned long long pcr0;

	unsigned int pcr_count;
	unsigned long long buffer_pts0;

	unsigned int transport_rate;

	unsigned long long byte_count;

	unsigned long long pcr[2];
	unsigned long long count[2];
};

void ts_parser_init(struct ts_parser *p);
int ts_parser_timestamp_range(struct file_buffer *b, struct ts_parser *p, struct avb_event *event, unsigned int *event_len, unsigned int data_start, unsigned int data_len, unsigned int presentation_offset);
int ts_parser_is_pcr(void *buf, unsigned long long *pcr);

#ifdef __cplusplus
}
#endif

#endif /* _TS_PARSER_H_ */
