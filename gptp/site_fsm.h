/*
* Copyright 2015 Freescale Semiconductor, Inc.
* Copyright 2020, 2023 NXP
*
* SPDX-License-Identifier: BSD-3-Clause
*/

/**
 @file
 @brief 802.1AS State Machine header file
 @details Definition of 802.1AS Site entity state machines functions and data structures.
*/

#ifndef _SITE_FSM_H_
#define _SITE_FSM_H_

#include "common/ptp.h"

#include "config.h"
#include "gptp.h"

void port_state_selection_sm(struct gptp_instance *instance);
int site_sync_sync_sm(struct gptp_instance *instance, ptp_site_sync_sync_sm_event_t event);


#endif /* _SITE_FSM_H_ */
