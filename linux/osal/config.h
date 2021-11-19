/*
* Copyright 2014 Freescale Semiconductor, Inc.
* Copyright 2020 NXP
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
 @brief Linux static configuration
 @details Contains all compile time configuration options for linux
*/

#ifndef _LINUX_OSAL_CFG_H_
#define _LINUX_OSAL_CFG_H_

#define GPTP_CFG_PRIORITY		58
#define AVTP_CFG_PRIORITY		60
#define SRP_CFG_PRIORITY		58
#define MAAP_CFG_PRIORITY		58
#define AVDECC_CFG_PRIORITY		58
#define MANAGEMENT_CFG_PRIORITY		58
#define STATS_CFG_PRIORITY		49

#ifndef CONFIG_AVB_DEFAULT_NET
#define CONFIG_AVB_DEFAULT_NET		NET_AVB
#endif

#ifndef CONFIG_LIB_DEFAULT_NET
#define CONFIG_LIB_DEFAULT_NET		NET_AVB
#endif


#ifndef CONFIG_FGPTP_DEFAULT_NET
#define CONFIG_FGPTP_DEFAULT_NET	NET_AVB
#endif

#ifndef CONFIG_1733_DEFAULT_NET
#define CONFIG_1733_DEFAULT_NET		NET_AVB
#endif

#endif /* _LINUX_OSAL_CFG_H_ */
