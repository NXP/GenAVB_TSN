/*
 * Copyright 2019 NXP
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
 \file clock.h
 \brief GenAVB public API
 \details clock definitions.

 \copyright Copyright 2019 NXP
*/
#ifndef _GENAVB_PUBLIC_CLOCK_H_
#define _GENAVB_PUBLIC_CLOCK_H_

#include "types.h"

/**
 * \ingroup clock
 * GenAVB clock type.
 */
typedef enum {
	GENAVB_CLOCK_MONOTONIC,		/**< Monotonic system clock */
	GENAVB_CLOCK_GPTP_0_0,		/**< gPTP clock interface 0 domain 0 */
	GENAVB_CLOCK_GPTP_0_1,		/**< gPTP clock interface 0 domain 1 */
	GENAVB_CLOCK_GPTP_1_0,		/**< gPTP clock interface 1 domain 0 */
	GENAVB_CLOCK_GPTP_1_1,		/**< gPTP clock interface 1 domain 1 */
	GENAVB_CLOCK_BR_0_0,		/**< gPTP clock bridge 0 domain 0 */
	GENAVB_CLOCK_BR_0_1,		/**< gPTP clock bridge 0 domain 1 */
	GENAVB_CLOCK_MAX
} genavb_clock_id_t;

/**
 * Get time in nanoseconds.
 * \ingroup clock
 * \return	::GENAVB_SUCCESS or negative error code.
 * \param id	clock id.
 * \param ns	pointer to uint64_t variable that will hold the result.
 */
int genavb_clock_gettime64(genavb_clock_id_t id, uint64_t *ns);

#endif /* _GENAVB_PUBLIC_CLOCK_H_ */

