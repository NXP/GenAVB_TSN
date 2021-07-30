/*
* Copyright 2017, 2020 NXP
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
 @brief FreeRTOS specific IPC service implementation
 @details
*/

#ifndef _FREERTOS_OSAL_IPC_H_
#define _FREERTOS_OSAL_IPC_H_

#define DEFAULT_IPC_DATA_SIZE	1024

struct ipc_rx {
	void *ipc_slot;		/* must match struct ipc_tx */
	void (*func)(struct ipc_rx const *, struct ipc_desc *);
	unsigned long priv;
};

struct ipc_tx {
	void *ipc_slot;
};

int ipc_rx_set_callback(struct ipc_rx *rx, int (*callback)(void *), void *data);
int ipc_rx_enable_callback(struct ipc_rx *rx);

#endif /* _FREERTOS_OSAL_IPC_H_ */
