/*
* Copyright 2020, 2022-2023 NXP
*
* SPDX-License-Identifier: BSD-3-Clause
*/

/**
 @file
 @brief QOS OS abstraction
 @details
*/

#ifndef _OS_QOS_H_
#define _OS_QOS_H_

#include "os/clock.h"
#include "genavb/qos.h"

/** Sets Scheduled Traffic admin configuration for a given logical port
 *
 * \return 0 on success, -1 on error
 * \param port_id	logical port id to configure
 * \param clk_id		clock id
 * \param config	EST configuration
 */
int qos_st_set_admin_config(unsigned int port_id, os_clock_id_t clk_id,
			    struct genavb_st_config *config);

/** Gets Scheduled Traffic admin configuration for a given logical port
 *
 * \return 0 on success, -1 on error
 * \param port_id	logical port id to configure
 * \param type		config type to read (administrative or operational)
 * \param config	EST configuration that will be written
 * \param list_length   the maximum length of the list provided in the config
 */
int qos_st_get_config(unsigned int port_id, genavb_st_config_type_t type,
		      struct genavb_st_config *config, unsigned int list_length);


int qos_clock_discontinuity(os_clock_id_t clk_id);

int qos_fp_set(unsigned int port_id, unsigned int type, struct genavb_fp_config *config);

int qos_fp_get(unsigned int port_id, unsigned int type, struct genavb_fp_config *config);


#endif /* _OS_QOS_H_ */
