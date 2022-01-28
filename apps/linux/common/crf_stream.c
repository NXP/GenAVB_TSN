/*
 * Copyright 2017 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <genavb/genavb.h>

#include "log.h"
#include "avb_stream.h"
#include "crf_stream.h"
#include "clock_domain.h"
#include "msrp.h"

aar_crf_stream_t * crf_stream_get(media_clock_role_t role)
{
	int i;

	for (i = 0; i < MAX_CRF_STREAMS; i++) {
		if (role == MEDIA_CLOCK_MASTER) {
			if (g_crf_streams[i].stream_params.direction == AVTP_DIRECTION_TALKER)
				return &g_crf_streams[i];
		}
		else {
			if (g_crf_streams[i].stream_params.direction == AVTP_DIRECTION_LISTENER)
				return &g_crf_streams[i];
		}
	}
	return NULL;
}

int crf_stream_create(media_clock_role_t role)
{
	aar_crf_stream_t *crf;
	int rc;

	struct avb_handle *avb_handle = avbstream_get_avb_handle();

	crf = crf_stream_get(role);
	if (!crf) {
		ERR("cannot get CRF stream");
		goto err_crf;
	}

	INF("stream_id: " STREAM_STR_FMT, STREAM_STR(crf->stream_params.stream_id));
	INF("dst_mac: " MAC_STR_FMT, MAC_STR(crf->stream_params.dst_mac));

	crf->cur_batch_size = avbstream_batch_size(crf->batch_size_ns, &crf->stream_params);

	/* The app is not aware of which SR classes are enabled, so different values are tried */
	rc = avb_stream_create(avb_handle, &crf->stream_handle, &crf->stream_params, &crf->cur_batch_size, 0);
	if (rc != AVB_SUCCESS) {
		crf->stream_params.stream_class = SR_CLASS_C;

		rc = avb_stream_create(avb_handle, &crf->stream_handle, &crf->stream_params, &crf->cur_batch_size, 0);
		if (rc != AVB_SUCCESS) {
			crf->stream_params.stream_class = SR_CLASS_E;

			rc = avb_stream_create(avb_handle, &crf->stream_handle, &crf->stream_params, &crf->cur_batch_size, 0);
			if (rc != AVB_SUCCESS) {
				crf->stream_params.stream_class = SR_CLASS_A;

				rc = avb_stream_create(avb_handle, &crf->stream_handle, &crf->stream_params, &crf->cur_batch_size, 0);
				if (rc != AVB_SUCCESS) {
					crf->stream_params.stream_class = SR_CLASS_D;

					rc = avb_stream_create(avb_handle, &crf->stream_handle, &crf->stream_params, &crf->cur_batch_size, 0);
					if (rc != AVB_SUCCESS) {
						ERR("create CRF stream failed, err %d", rc);
						goto err_avb;
					}
				}
			}
		}
	}

	if (role == MEDIA_CLOCK_MASTER) {
		rc = msrp_talker_register(&crf->stream_params);
		if (rc != AVB_SUCCESS) {
			ERR("msrp_talker_register error, rc = %d", rc);
			goto err_msrp;
		}
	}
	else {
		rc = msrp_listener_register(&crf->stream_params);
		if (rc != AVB_SUCCESS) {
			ERR("msrp_talker_register error, rc = %d", rc);
			goto err_msrp;
		}
	}

	return 0;

err_msrp:
	avb_stream_destroy(crf->stream_handle);

err_avb:
err_crf:
	return -1;
}

int crf_stream_destroy(media_clock_role_t role)
{
	aar_crf_stream_t *crf;
	int rc;

	crf = crf_stream_get(role);
	if (!crf) {
		ERR("cannot get CRF stream");
		return -1;
	}

	rc = avb_stream_destroy(crf->stream_handle);
	if (rc != AVB_SUCCESS)
		ERR("avb_stream_destroy error, rc = %d", rc);

	if (role == MEDIA_CLOCK_MASTER)
		msrp_talker_deregister(&crf->stream_params);
	else
		msrp_listener_deregister(&crf->stream_params);

	return rc;
}

