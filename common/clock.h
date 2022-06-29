/*
* Copyright 2019-2020 NXP.
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
 @file	clock.h
 @brief Generic clock functions
 @details
*/

#ifndef _COMMON_CLOCK_H_
#define _COMMON_CLOCK_H_

#include "os/clock.h"


int clock_set_time64(os_clock_id_t clk_id, u64 ns);

#endif /* _COMMON_CLOCK_H_ */
