/*
 * Copyright 2018-2021 NXP
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
 \file types.h
 \brief OS specific GenAVB public API
 \details OS specific net types

 \copyright Copyright 2018-2021 NXP
*/

#ifndef _OS_GENAVB_PUBLIC_NET_TYPES_H_
#define _OS_GENAVB_PUBLIC_NET_TYPES_H_

#define NET_DATA_OFFSET 128 /* Hardcode the value to 128 as a common L1_CACHE alignment of
                               the max avb descriptor size for both i.MX6 and i.MX8*/

#define MAX_SOCKETS	256

struct genavb_xdp_key {
	uint16_t protocol;	/**< protocol type */
	uint16_t vlan_id;	/**< vlan id (network order), one of [VLAN_VID_MIN, VLAN_VID_MAX], VLAN_VID_NONE or VLAND_ID_DEFAULT */
	uint8_t dst_mac[6];	/**< destination MAC */
};

#endif /* _OS_GENAVB_PUBLIC_NET_TYPES_H_ */

