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
 @brief FreeRTOS static configuration
 @details Contains all compile time configuration options for FreeRTOS
*/

#ifndef _FREERTOS_OSAL_CFG_H_
#define _FREERTOS_OSAL_CFG_H_

#define GPTP_CFG_STACK_DEPTH			(configMINIMAL_STACK_SIZE + 430)
#define GPTP_CFG_EVENT_QUEUE_LENGTH		16
#define SRP_CFG_STACK_DEPTH			(configMINIMAL_STACK_SIZE + 310)
#define SRP_CFG_EVENT_QUEUE_LENGTH		16
#define AVDECC_CFG_STACK_DEPTH			(configMINIMAL_STACK_SIZE + 330)
#define AVDECC_CFG_EVENT_QUEUE_LENGTH		16
#define AVTP_CFG_STACK_DEPTH			(configMINIMAL_STACK_SIZE + 580)
#define AVTP_CFG_EVENT_QUEUE_LENGTH		16
#define STATS_CFG_STACK_DEPTH			(configMINIMAL_STACK_SIZE + 210)
#define STATS_CFG_EVENT_QUEUE_LENGTH		16
#define MANAGEMENT_CFG_STACK_DEPTH		(configMINIMAL_STACK_SIZE + 166)
#define MANAGEMENT_CFG_EVENT_QUEUE_LENGTH	16

#define GPTP_CFG_PRIORITY		(configMAX_PRIORITIES - 8) /* NET_RX_PRIORITY - 4 */
#define SRP_CFG_PRIORITY		(GPTP_CFG_PRIORITY)
#define AVDECC_CFG_PRIORITY		(GPTP_CFG_PRIORITY)
#define AVTP_CFG_PRIORITY		(configMAX_PRIORITIES - 6)
#define MANAGEMENT_CFG_PRIORITY		(GPTP_CFG_PRIORITY)
#define STATS_CFG_PRIORITY		1

#endif /* _FREERTOS_OSAL_CFG_H_ */
