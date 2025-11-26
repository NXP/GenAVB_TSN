/*
 * Copyright 2025 NXP.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @brief Redundant Listener + Talker entity
 @details Redundant Listener and Talker AVDECC Milan entity definition with AAF/CRF input/output streams
*/

#include "genavb/adp.h"
#include "genavb/aem_helpers.h"

#include "listener_talker_audio_redundant_milan.h"

AEM_ENTITY_STORAGE();

void listener_talker_audio_redundant_milan_init(struct aem_desc_hdr *aem_desc)
{
	AEM_ENTITY_INIT(aem_desc);
}
