/*
 * Copyright 2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2018, 2021, 2023, 2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief AVTP Control Format (ACF) handling functions
 @details
*/

#ifndef _ACF_H_
#define _ACF_H_

#ifdef CFG_AVTP_1722A
#include "common/net.h"
#include "genavb/acf.h"
#include "genavb/avdecc.h"

#include "stream.h"

int listener_acf_tscf_check(struct redundant_set_listener *set, u64 const *stream_id, struct ipc_avtp_connect *ipc);
int listener_acf_ntscf_check(struct redundant_set_listener *set, u64 const *stream_id, struct ipc_avtp_connect *ipc);
int talker_acf_tscf_check(struct redundant_set_talker *set, u64 const *stream_id, struct ipc_avtp_connect *ipc);
int talker_acf_ntscf_check(struct redundant_set_talker *set, u64 const *stream_id, struct ipc_avtp_connect *ipc);

#endif

#endif /* _ACF_H_ */
