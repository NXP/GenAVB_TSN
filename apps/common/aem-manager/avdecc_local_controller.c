/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @brief Local Controller entity
 @details Local Controller AVDECC entity definition
*/

#include "genavb/adp.h"
#include "genavb/aem_helpers.h"

#include "avdecc_local_controller.h"

AEM_ENTITY_STORAGE();

void local_controller_init(struct aem_desc_hdr *aem_desc)
{
	AEM_ENTITY_INIT(aem_desc);
}
