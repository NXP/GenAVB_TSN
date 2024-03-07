/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @brief Talker entity
 @details Talker AVDECC entity definition with a single output stream
*/

#include "genavb/adp.h"
#include "genavb/aem_helpers.h"

#include "talker_video_single.h"

AEM_ENTITY_STORAGE();

void talker_video_single_init(struct aem_desc_hdr *aem_desc)
{
	AEM_ENTITY_INIT(aem_desc);
}
