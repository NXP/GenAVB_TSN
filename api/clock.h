/*
 * Copyright 2019-2021, 2023, 2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 \file clock.h
 \brief GenAVB API private includes
 \details private definitions for the GenAVB library
*/

#ifndef _PRIVATE_CLOCK_H_
#define _PRIVATE_CLOCK_H_

#include "genavb/clock.h"
#include "os/clock.h"

os_clock_id_t genavb_clock_to_os_clock(genavb_clock_id_t id);
genavb_clock_id_t os_clock_to_genavb_clock(os_clock_id_t id);

#endif /* _PRIVATE_CLOCK_H_ */
