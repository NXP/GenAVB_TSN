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
 @brief basic queue implementation
 @details
*/

#ifndef _QUEUE_H_
#define _QUEUE_H_

#include "os/sys_types.h"
#include "atomic.h"

#define QUEUE_ENTRIES_MAX	32

typedef unsigned int atomic_t;
static inline void smp_wmb(void) {}

#include "common/os/queue_common.h"

#endif /* _QUEUE_H_ */

