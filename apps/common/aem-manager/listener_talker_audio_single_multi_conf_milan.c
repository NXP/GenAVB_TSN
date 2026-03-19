/*
 * Copyright 2021, 2026 NXP.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @brief Listener + Talker entity
 @details Listener and Talker AVDECC Milan entity definition supporting two configurations each with single input/output streams
*/

#include "genavb/adp.h"
#include "genavb/aem_helpers.h"

#include "listener_talker_audio_single_multi_conf_milan.h"

AEM_ENTITY_STORAGE();

#if CFG_MAX_AEM_CONFIGS < 2
#error "Listener + Talker Milan multi configuration entity requires CFG_MAX_AEM_CONFIGS >= 2"
#endif

unsigned int listener_talker_audio_single_multi_conf_milan_init(struct aem_desc_hdr *aem_desc)
{
	struct aem_desc_hdr *aem_desc_cfg_0 = aem_desc;
	struct aem_desc_hdr *aem_desc_cfg_1 = aem_desc + AEM_NUM_DESC_TYPES;

	AEM_ENTITY_INIT(aem_desc_cfg_0, 0);
	AEM_ENTITY_INIT(aem_desc_cfg_1, 1);

	return 2;
}
