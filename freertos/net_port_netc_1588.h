/*
* Copyright 2022 NXP
*
* NXP Confidential. This software is owned or controlled by NXP and may only
* be used strictly in accordance with the applicable license terms.  By expressly
* accepting such terms or by downloading, installing, activating and/or otherwise
* using the software, you are agreeing that you have read, and that you agree to
* comply with and are bound by, such license terms.  If you do not agree to be
* bound by the applicable license terms, then you may not retain, install, activate
* or otherwise use the software.
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
