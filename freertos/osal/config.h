/*
* Copyright 2017, 2020, 2022-2023 NXP
*
* SPDX-License-Identifier: BSD-3-Clause
*/

/**
 @file
 @brief FreeRTOS static configuration
 @details Contains all compile time configuration options for FreeRTOS
*/

#ifndef _FREERTOS_OSAL_CFG_H_
#define _FREERTOS_OSAL_CFG_H_

#define GPTP_CFG_STACK_DEPTH			(configMINIMAL_STACK_SIZE + 430)
#define GPTP_CFG_EVENT_QUEUE_LENGTH		16
#define SRP_CFG_STACK_DEPTH			(configMINIMAL_STACK_SIZE + 430)
#define SRP_CFG_EVENT_QUEUE_LENGTH		16
#define AVDECC_CFG_STACK_DEPTH			(configMINIMAL_STACK_SIZE + 330)
#define AVDECC_CFG_EVENT_QUEUE_LENGTH		16
#define AVTP_CFG_STACK_DEPTH			(configMINIMAL_STACK_SIZE + 580)
#define AVTP_CFG_EVENT_QUEUE_LENGTH		16
#define STATS_CFG_STACK_DEPTH			(configMINIMAL_STACK_SIZE + 210)
#define STATS_CFG_EVENT_QUEUE_LENGTH		16
#define MANAGEMENT_CFG_STACK_DEPTH		(configMINIMAL_STACK_SIZE + 256)
#define MANAGEMENT_CFG_EVENT_QUEUE_LENGTH	16
#define MAAP_CFG_STACK_DEPTH			(configMINIMAL_STACK_SIZE + 200)
#define MAAP_CFG_EVENT_QUEUE_LENGTH		16
#define HSR_CFG_STACK_DEPTH			(configMINIMAL_STACK_SIZE + 580)
#define HSR_CFG_EVENT_QUEUE_LENGTH		16

#define GPTP_CFG_PRIORITY		(configMAX_PRIORITIES - 8) /* NET_RX_PRIORITY - 4 */
#define SRP_CFG_PRIORITY		(GPTP_CFG_PRIORITY)
#define AVDECC_CFG_PRIORITY		(GPTP_CFG_PRIORITY)
#define AVTP_CFG_PRIORITY		(configMAX_PRIORITIES - 6)
#define MANAGEMENT_CFG_PRIORITY		(GPTP_CFG_PRIORITY)
#define STATS_CFG_PRIORITY		1
#define MAAP_CFG_PRIORITY		(GPTP_CFG_PRIORITY)
#define HSR_CFG_PRIORITY		(GPTP_CFG_PRIORITY)

#endif /* _FREERTOS_OSAL_CFG_H_ */
