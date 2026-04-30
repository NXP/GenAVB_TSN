/*
 * Copyright 2015-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2018, 2021, 2023, 2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief AVTP compressed video format (CVF) handling functions
 @details
*/

#ifndef _CVF_H_
#define _CVF_H_

#ifdef CFG_AVTP_1722A
#include "common/net.h"
#include "genavb/cvf.h"
#include "genavb/avdecc.h"

#include "stream.h"

int listener_cvf_check(struct redundant_set_listener *set, u64 const *stream_id, struct ipc_avtp_connect *ipc);
int talker_cvf_check(struct redundant_set_talker *set, u64 const *stream_id, struct ipc_avtp_connect *ipc);

#endif

#endif /* _CVF_H_ */
