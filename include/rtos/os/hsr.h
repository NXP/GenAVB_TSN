/*
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 \file
 \brief GenAVB/TSN RTOS public API
 \details HSR definitions.

 \copyright Copyright 2023 NXP
*/
#ifndef _OS_GENAVB_PUBLIC_HSR_API_H_
#define _OS_GENAVB_PUBLIC_HSR_API_H_

/** Set the HSR operation mode
 * \ingroup HSR
 *
 * \return		::GENAVB_SUCCESS or negative error code.
 * \param genavb	genavb stream handle
 * \param mode		different node operation mode
 */
int genavb_hsr_operation_mode_set(struct genavb_handle *genavb, genavb_hsr_mode_t mode);

#endif /* _OS_GENAVB_PUBLIC_HSR_API_H_ */
