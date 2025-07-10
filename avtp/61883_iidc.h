/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016, 2018, 2021, 2023, 2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief IEC 61883/IIDC protocol handling functions
 @details
*/

#ifndef _61883_IIDC_H_
#define _61883_IIDC_H_

#include "common/net.h"

#include "stream.h"

int listener_61883_iidc_check(struct redundant_set_listener *set, u64 const *stream_id, struct ipc_avtp_connect *ipc);
int talker_61883_iidc_check(struct redundant_set_talker *set, u64 const *stream_id, struct ipc_avtp_connect *ipc);

void avtp_61883_2_net_rx(struct stream_listener *, struct avtp_rx_desc **, unsigned int n);
void avtp_61883_3_net_rx(struct stream_listener *, struct avtp_rx_desc **, unsigned int n);
void avtp_61883_4_net_rx(struct stream_listener *, struct avtp_rx_desc **, unsigned int n);
void avtp_61883_5_net_rx(struct stream_listener *, struct avtp_rx_desc **, unsigned int n);
void avtp_61883_6_net_rx(struct stream_listener *, struct avtp_rx_desc **, unsigned int n);
void avtp_61883_7_net_rx(struct stream_listener *, struct avtp_rx_desc **, unsigned int n);
void avtp_iidc_net_rx(struct stream_listener *, struct avtp_rx_desc **, unsigned int n);

void avtp_61883_2_net_tx(struct stream_talker *);
void avtp_61883_3_net_tx(struct stream_talker *);
void avtp_61883_4_net_tx(struct stream_talker *);
void avtp_61883_5_net_tx(struct stream_talker *);
void avtp_61883_6_net_tx(struct stream_talker *);
void avtp_61883_7_net_tx(struct stream_talker *);
void avtp_iidc_net_tx(struct stream_talker *);

#endif /* _61883_IIDC_H_ */
