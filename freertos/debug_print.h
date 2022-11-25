/*
* Copyright 2018, 2020, 2022 NXP
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
 @brief Stats printing
*/

#ifndef _DEBUG_PRINT_H_
#define _DEBUG_PRINT_H_

#define SHOW_MCLOCK_STATS		1
#define SHOW_HR_TIMER_STATS		1
#define SHOW_QOS_STATS			0
#define SHOW_PORT_STATS			1

#if (defined(CONFIG_AVTP) && SHOW_MCLOCK_STATS)
void mclock_show_stats(void);
#else
static inline void mclock_show_stats(void) {};
#endif

#if SHOW_HR_TIMER_STATS
void hr_timer_show_stats(void);
#else
static inline void hr_timer_show_stats(void) {};
#endif

#if SHOW_QOS_STATS
void net_port_qos_show_stats(void);
#else
static inline void net_port_qos_show_stats(void){};
#endif

#if SHOW_PORT_STATS
void net_port_show_stats(void);
#else
static inline void net_port_show_stats(void){};
#endif

#endif /* _DEBUG_PRINT_H_ */

