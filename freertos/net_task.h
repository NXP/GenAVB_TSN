/*
* Copyright 2017, 2020, 2023 NXP
*
* SPDX-License-Identifier: BSD-3-Clause
*/

/**
 @file
 @brief FreeRTOS specific Network service implementation
 @details
*/

#ifndef _FREERTOS_NET_TASK_H_
#define _FREERTOS_NET_TASK_H_

int net_task_init(void);
void net_task_exit(void);

#endif /* _FREERTOS_NET_TASK_H_ */
