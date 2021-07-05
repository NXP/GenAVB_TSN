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
 \file timer.h
 \brief GenAVB API private includes
 \details private definitions for the GenAVB library

 \copyright Copyright 2019-2020 NXP
*/

#ifndef _PRIVATE_TIMER_H_
#define _PRIVATE_TIMER_H_

#include "os/timer.h"

struct genavb_timer {
	struct os_timer os_t;
	void (*callback)(void *, int);
	void *data;
};

#endif /* _PRIVATE_TIMER_H_ */
