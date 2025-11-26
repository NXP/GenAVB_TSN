/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 \file version.c
 \brief GenAVB public API
 \details API definition for the GenAVB library
*/

#include "genavb/init.h"
#include "common/version.h"

const char *genavb_version(void)
{
	return GENAVB_VERSION;
}
