/*
* Copyright 2018, 2020, 2023 NXP
*
* SPDX-License-Identifier: BSD-3-Clause
*/

/**
 @file
 @brief FreeRTOS specific IPC service implementation
 @details
*/

#ifndef _FREERTOS_IPC_H_
#define _FREERTOS_IPC_H_

int ipc_init(void);
void ipc_exit(void);

#endif /* _FREERTOS_IPC_H_ */
