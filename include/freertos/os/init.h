/*
 * Copyright 2018, 2022 NXP
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
 \file init.h
 \brief OS specific GenAVB public initialization API
 \details OS specific initialization API definition for the GenAVB library

 \copyright Copyright 2018, 2022 NXP
*/

#ifndef _OS_GENAVB_PUBLIC_INIT_API_H_
#define _OS_GENAVB_PUBLIC_INIT_API_H_

/**
 * \ingroup init
 * GenAVB global configuration.
 */
struct genavb_config {
	struct management_config management_config;
	struct fgptp_config fgptp_config;
	struct avtp_config avtp_config;
	struct srp_config srp_config;
	struct avdecc_config avdecc_config;
	struct maap_config maap_config;
};

/** Gets a copy of the GenAVB default configuration.
 * \ingroup init
 * \param[out] config	pointer to the caller configuration storage.
 */
void genavb_get_default_config(struct genavb_config *config);

/** Sets the GenAVB configuration (used at initialization).
 * \ingroup init
 * \param[in] config	pointer to a GenAVB configuration. The configuration is only used when genavb_init() is called.
 */
void genavb_set_config(struct genavb_config *config);


#endif /* _OS_GENAVB_PUBLIC_INIT_API_H_ */
