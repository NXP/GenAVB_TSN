/*
* Copyright 2019-2020 NXP
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
#ifndef _FREERTOS_NET_H_
#define _FREERTOS_NET_H_

void net_tx_desc_free(void *data, unsigned long entry);
void net_rx_desc_free(void *data, unsigned long entry);

#endif /* _FREERTOS_NET_H_ */
