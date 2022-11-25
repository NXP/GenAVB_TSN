/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *    Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *    Neither the name of NXP Semiconductors nor the
 *    names of its contributors may be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 @brief AEM helper functions
 @details Helper functions to handle AEM structures

 Copyright 2014 Freescale Semiconductor, Inc.
 All Rights Reserved.
*/

#ifndef _GENAVB_PUBLIC_AEM_HELPERS_H_
#define _GENAVB_PUBLIC_AEM_HELPERS_H_

#include "aem.h"

struct aem_desc_hdr {
	void *ptr;
	avb_u16 size;
	avb_u16 total;
};

unsigned int aem_get_descriptor_max(struct aem_desc_hdr *aem_desc, avb_u16 type);
void *aem_get_descriptor(struct aem_desc_hdr *aem_desc, avb_u16 type, avb_u16 index, avb_u16 *len);
void aem_entity_desc_fixup(struct aem_desc_hdr *aem_desc);
void aem_configuration_desc_fixup(struct aem_desc_hdr *aem_desc);
void aem_video_cluster_desc_fixup(struct aem_desc_hdr *aem_desc);
unsigned int aem_get_listener_streams(struct aem_desc_hdr *aem_desc);
unsigned int aem_get_talker_streams(struct aem_desc_hdr *aem_desc);

struct aem_desc_hdr *aem_entity_load_from_file(const char *name);

#endif /* _GENAVB_PUBLIC_AEM_HELPERS_H_ */
