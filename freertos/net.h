/*
* Copyright 2019-2020, 2023 NXP
*
* SPDX-License-Identifier: BSD-3-Clause
*/

/**
 @file
 @brief FreeRTOS specific Network service implementation
 @details
*/
#ifndef _FREERTOS_NET_H_
#define _FREERTOS_NET_H_

void net_tx_desc_free(void *data, unsigned long entry);
void net_rx_desc_free(void *data, unsigned long entry);

#endif /* _FREERTOS_NET_H_ */
