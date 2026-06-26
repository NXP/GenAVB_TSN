/*
 * Copyright 2021, 2026 NXP.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @brief Talker entity
 @details Talker AVDECC Milan entity definition with single output stream
*/

#include "genavb/adp.h"
#include "genavb/aem_helpers.h"

#include "talker_audio_single_milan.h"

AEM_ENTITY_STORAGE();

unsigned int talker_audio_single_milan_init(struct aem_desc_hdr *aem_desc)
{
	AEM_ENTITY_INIT(aem_desc);

	return 1;
}
