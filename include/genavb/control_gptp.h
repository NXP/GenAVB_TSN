/*
 * Copyright 2018, 2023 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 \file control_gptp.h
 \brief GenAVB public control API
 \details GPTP control API definition for the GenAVB library

 \copyright Copyright 2018, 2023 NXP
*/

#ifndef _GENAVB_PUBLIC_CONTROL_GPTP_API_H_
#define _GENAVB_PUBLIC_CONTROL_GPTP_API_H_

#include "control.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "config.h"
#include "types.h"

/**
 * \ingroup control
 * GPTP grand master get status command
 *
 */
struct genavb_msg_gm_get_status {
	uint8_t domain;
};

/**
 * \ingroup control
 *
 */
struct genavb_msg_gm_status {
	uint8_t domain;
	uint64_t gm_id;
};

/**
 * \ingroup control
 *
 */
union genavb_msg_gptp {
	struct genavb_msg_gm_get_status gm_get_status;
	struct genavb_msg_gm_status gm_status;
};

#ifdef __cplusplus
}
#endif

#endif /* _GENAVB_PUBLIC_CONTROL_GPTP_API_H_ */
