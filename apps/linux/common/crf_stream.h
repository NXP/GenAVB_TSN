/*
 * Copyright 2017 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _CRF_STREAM_H_
#define _CRF_STREAM_H_

#include "clock_domain.h"


#define MAX_CRF_STREAMS 2

typedef struct {
	struct avb_stream_params stream_params;
	struct avb_stream_handle *stream_handle;

	unsigned int batch_size_ns;
	unsigned int cur_batch_size;
} aar_crf_stream_t;

extern aar_crf_stream_t g_crf_streams[];

aar_crf_stream_t * crf_stream_get(media_clock_role_t role);
int crf_stream_create(media_clock_role_t role);
int crf_stream_destroy(media_clock_role_t role);

#endif /* _CLOCK_DOMAIN_H_ */

