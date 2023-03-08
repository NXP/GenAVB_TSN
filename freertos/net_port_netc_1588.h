/*
* Copyright 2022-2023 NXP
*
* SPDX-License-Identifier: BSD-3-Clause
*/

/**
 @file
 @brief FreeRTOS specific Network service implementation
 @details
*/

#ifndef _FREERTOS_NET_PORT_NETC_1588_H_
#define _FREERTOS_NET_PORT_NETC_1588_H_

void *netc_1588_init(void);
void netc_1588_exit(void);
uint64_t netc_1588_hwts_to_u64(void *handle, uint32_t hwts_ns);
uint64_t netc_1588_read_counter(void *priv);
int netc_1588_clock_adj_freq(void *priv, int32_t ppb);

#endif /* _FREERTOS_NET_PORT_NETC_1588_H_ */
