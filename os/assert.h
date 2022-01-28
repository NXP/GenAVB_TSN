/*
* Copyright 2017, 2020 NXP
*
* NXP Confidential. This software is owned or controlled by NXP and may only
* be used strictly in accordance with the applicable license terms.  By expressly
* accepting such terms or by downloading, installing, activating and/or otherwise
* using the software, you are agreeing that you have read, and that you agree to
* comply with and are bound by, such license terms.  If you do not agree to be
* bound by the applicable license terms, then you may not retain, install, activate
* or otherwise use the software.
*/

/**
 @file
 @brief Standard library OS abstraction
 @details
*/

#ifndef _OS_ASSERT_H_
#define _OS_ASSERT_H_

/** abort the program if assertion is false
 * \return	no returned value
 * \param expr	expression to assert
 *
 * os_assert(expr)
 *
 */

void os_assert(int expr);

#include "osal/assert.h"

#endif /* _OS_ASSERT_H_ */
