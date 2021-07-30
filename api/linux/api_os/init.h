/*
* Copyright 2018, 2020 NXP
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
 \file init.h
 \brief GenAVB API private includes
 \details private definitions for the GenAVB library

 \copyright Copyright 2018, 2020 NXP
*/

#ifndef _LINUX_PRIVATE_INIT_H_
#define _LINUX_PRIVATE_INIT_H_

#include "common/ipc.h"
#include "common/list.h"

struct genavb_handle {
	int flags;
	struct list_head streams;
	struct ipc_tx avtp_tx;
	struct ipc_rx avtp_rx;
};

#endif /* _LINUX_PRIVATE_INIT_H_ */