/*
 * Copyright 2018, 2020, 2022, 2024-2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _COMMON_HELPERS_H_
#define _COMMON_HELPERS_H_

#include <stddef.h>     /* for offsetof() */

#define container_of(entry, type, member) ((type *)((unsigned char *)(entry) - offsetof(type, member)))

#define VAL_TO_STR_HELPER(x) #x
#define VAL_TO_STR(x) VAL_TO_STR_HELPER(x)

#endif /* _COMMON_HELPERS_H_ */
