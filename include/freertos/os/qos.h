/*
 * Copyright 2020, 2023 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 \file
 \brief GenAVB public API
 \details 802.1Q QoS definitions.

 \copyright Copyright 2020, 2023 NXP
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
