/*
 * Copyright 2020 NXP
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
 \file qos.h
 \brief GenAVB public API
 \details 802.1Q QoS definitions.

 \copyright Copyright 2020 NXP
*/

#ifndef _OS_GENAVB_PUBLIC_QOS_API_H_
#define _OS_GENAVB_PUBLIC_QOS_API_H_

/** Sets Scheduled Traffic admin configuration for a given logical port
 * \ingroup qos
 *
 * \return		::GENAVB_SUCCESS or negative error code.
 * \param port_id	logical port id to configure
 * \param clk_id	clock id
 * \param config	EST configuration
 */
int genavb_st_set_admin_config(unsigned int port_id, genavb_clock_id_t clk_id,
			       struct genavb_st_config *config);

/** Gets Scheduled Traffic configuration for a given logical port
 * \ingroup qos
 *
 * \return		::GENAVB_SUCCESS or negative error code.
 * \param port_id	logical port id to configure
 * \param type		config type to read (administrative or operational)
 * \param config	EST configuration that will be written
 * \param list_length   the maximum length of the list provided in the config
 */
int genavb_st_get_config(unsigned int port_id, genavb_st_config_type_t type,
			 struct genavb_st_config *config, unsigned int list_length);


/** Sets Frame Preemption configuration for a given logical port
 * \ingroup qos
 *
 * \return		::GENAVB_SUCCESS or negative error code.
 * \param port_id	logical port id to configure
 * \param type		type of configuration being set
 * \param config	configuration to set
 */
int genavb_fp_set(unsigned int port_id, genavb_fp_config_type_t type, struct genavb_fp_config *config);

/** Gets Frame Preemption configuration for a given logical port
 * \ingroup qos
 *
 * \return		::GENAVB_SUCCESS or negative error code.
 * \param port_id	logical port id to configure
 * \param type		type of configuration being retrieved
 * \param config	configuration retrieved
*/
int genavb_fp_get(unsigned int port_id, genavb_fp_config_type_t type, struct genavb_fp_config *config);

#endif /* _OS_GENAVB_PUBLIC_QOS_API_H_ */
