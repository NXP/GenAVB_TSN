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
 \file clock.h
 \brief GenAVB API private includes
 \details private definitions for the GenAVB library

 \copyright Copyright 2019-2020 NXP
*/

#ifndef _PRIVATE_CLOCK_H_
#define _PRIVATE_CLOCK_H_

#include "genavb/clock.h"
#include "os/clock.h"

os_clock_id_t genavb_clock_to_os_clock(genavb_clock_id_t id);

#endif /* _PRIVATE_CLOCK_H_ */

