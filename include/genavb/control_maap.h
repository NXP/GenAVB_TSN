/*
 * Copyright 2021 NXP
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
 \file control_maap.h
 \brief GenAVB public control API
 \details MAAP control API definition for the GenAVB library

 \copyright Copyright 2021 NXP
*/

#ifndef _GENAVB_PUBLIC_CONTROL_MAAP_API_H_
#define _GENAVB_PUBLIC_CONTROL_MAAP_API_H_

#include "control.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "config.h"
#include "types.h"

/**
 * \ingroup control
 * MAAP IPC status
 */
typedef enum {
	MAAP_STATUS_SUCCESS,
	MAAP_STATUS_FREE,
	MAAP_STATUS_CONFLICT,
	MAAP_STATUS_ERROR,
} genavb_maap_status_t;

/**
 * \ingroup control
 * MAAP IPC response status
 */
typedef enum {
	MAAP_RESPONSE_SUCCESS,
	MAAP_RESPONSE_ERROR,
} genavb_maap_response_status_t;

/**
 * \ingroup control
 * MAAP generic command
 */
struct genavb_msg_maap_create {
	avb_u8 flag;		/**< Prefered range flag. 1 to allocate our range to a port, 0 to let the stack choose the range */
	avb_u16 port_id;
	avb_u32 range_id;
	avb_u16 count;		/**< Number of consecutive addresses in the range. max = 65024 */
	avb_u8 base_address[6];		/**< First address of the range */
};

/**
 * \ingroup control
 * MAAP generic command
 */
struct genavb_msg_maap_delete {
	avb_u16 port_id;
	avb_u32 range_id;
};

/**
 * \ingroup control
 * MAAP generic response
 */
struct genavb_msg_maap_create_response {
	avb_u16 status;		/**< ::genavb_maap_response_status_t */
	avb_u16 port_id;
	avb_u32 range_id;
	avb_u16 count;		/**< Number of consecutive addresses in the range. max = 65024 */
	avb_u8 base_address[6];		/**< First address of the range */
};

/**
 * \ingroup control
 * MAAP generic response
 */
struct genavb_msg_maap_delete_response {
	avb_u16 status;		/**< ::genavb_maap_response_status_t */
	avb_u16 port_id;
	avb_u32 range_id;
};

/**
 * \ingroup control
 * MAAP generic indication
 */
struct genavb_maap_status {
	avb_u16 status;		/**< ::genavb_maap_status_t */
	avb_u16 port_id;
	avb_u32 range_id;
	avb_u16 count;		/**< Number of consecutive addresses in the range. max = 65024 */
	avb_u8 base_address[6];		/**< First address of the range */
};

#ifdef __cplusplus
}
#endif

#endif /* _GENAVB_PUBLIC_CONTROL_MAAP_API_H_ */
