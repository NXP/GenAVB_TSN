/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @brief Controller entity
 @details Controller AVDECC entity definition
*/

#include "genavb/adp.h"
#include "genavb/aem_helpers.h"

#include "controller.h"

AEM_ENTITY_STORAGE();

void controller_init(struct aem_desc_hdr *aem_desc)
{
	AEM_ENTITY_INIT(aem_desc);
}
