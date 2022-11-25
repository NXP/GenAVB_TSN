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
 \file adp.h
 \brief GenAVB public API
 \details 1722.1 ADP helper definitions.

 \copyright Copyright 2015 Freescale Semiconductor, Inc.
*/
#ifndef _GENAVB_PUBLIC_ADP_H_
#define _GENAVB_PUBLIC_ADP_H_

#include "types.h"

/* ADP message types */
#define ADP_ENTITY_AVAILABLE 0
#define ADP_ENTITY_DEPARTING 1
#define ADP_ENTITY_DISCOVER  2
#define ADP_ENTITY_NOTFOUND  3

/* Entity capabilities bits */
#define ADP_ENTITY_EFU_MODE				(1 << 0)
#define ADP_ENTITY_ADDRESS_ACCESS_SUPPORTED		(1 << 1)
#define ADP_ENTITY_GATEWAY_ENTITY			(1 << 2)
#define ADP_ENTITY_AEM_SUPPORTED			(1 << 3)
#define ADP_ENTITY_LEGACY_AVC				(1 << 4)
#define ADP_ENTITY_ASSOCIATION_ID_SUPPORTED		(1 << 5)
#define ADP_ENTITY_ASSOCIATION_ID_VALID			(1 << 6)
#define ADP_ENTITY_VENDOR_UNIQUE_SUPPORTED		(1 << 7)
#define ADP_ENTITY_CLASS_A_SUPPORTED			(1 << 8)
#define ADP_ENTITY_CLASS_B_SUPPORTED			(1 << 9)
#define ADP_ENTITY_GPTP_SUPPORTED			(1 << 10)
#define ADP_ENTITY_AEM_AUTHENTICATION_SUPPORTED		(1 << 11)
#define ADP_ENTITY_AEM_AUTHENTICATION_REQUIRED		(1 << 12)
#define ADP_ENTITY_AEM_PERSISTENT_ACQUIRE_SUPPORTED	(1 << 13)
#define ADP_ENTITY_AEM_IDENTIFY_CONTROL_INDEX_VALID	(1 << 14)
#define ADP_ENTITY_AEM_INTERFACE_INDEX_VALID		(1 << 15)
#define ADP_ENTITY_GENERAL_CONTROLLER_IGNORE		(1 << 16)
#define ADP_ENTITY_ENTITY_NOT_READY			(1 << 17)

/* Talker capabilities bits*/
#define ADP_TALKER_IMPLEMENTED		(1 << 0)
#define ADP_TALKER_OTHER_SOURCE		(1 << 9)
#define ADP_TALKER_CONTROL_SOURCE	(1 << 10)
#define ADP_TALKER_MEDIA_CLOCK_SOURCE	(1 << 11)
#define ADP_TALKER_SMPTE_SOURCE		(1 << 12)
#define ADP_TALKER_MIDI_SOURCE		(1 << 13)
#define ADP_TALKER_AUDIO_SOURCE		(1 << 14)
#define ADP_TALKER_VIDEO_SOURCE		(1 << 15)

/* Listener capabilities bits*/
#define ADP_LISTENER_IMPLEMENTED	(1 << 0)
#define ADP_LISTENER_OTHER_SINK		(1 << 9)
#define ADP_LISTENER_CONTROL_SINK	(1 << 10)
#define ADP_LISTENER_MEDIA_CLOCK_SINK	(1 << 11)
#define ADP_LISTENER_SMPTE_SINK		(1 << 12)
#define ADP_LISTENER_MIDI_SINK		(1 << 13)
#define ADP_LISTENER_AUDIO_SINK		(1 << 14)
#define ADP_LISTENER_VIDEO_SINK		(1 << 15)

/* Controller capabilities bits */
#define ADP_CONTROLLER_IMPLEMENTED	(1 << 0)


/**
 * \ingroup control
 * Discovered entity information, based on IEEE 1722.1-2013 section 6.2.1 and 6.2.6.1.1.
 */
struct entity_info {
	avb_u64 entity_id;
	avb_u64 entity_model_id;
	avb_u32 entity_capabilities;
	avb_u16 talker_stream_sources;
	avb_u16 talker_capabilities;
	avb_u16 listener_stream_sinks;
	avb_u16 listener_capabilities;
	avb_u32 controller_capabilities;
	avb_u64 gptp_grandmaster_id;
	avb_u8  gptp_domain_number;
	avb_u16 identity_control_index;
	avb_u16 interface_index;
	avb_u32 available_index;
	avb_u64 association_id;
	avb_u8 mac_addr[6];		/* 6.2.6.1.1: source MAC address of the received ADPDU */
	avb_u8 local_mac_addr[6];	/* 6.2.6.1.1: MAC address of the local network port that received the ADPDU */
};

#endif /* _GENAVB_PUBLIC_ADP_H_ */

