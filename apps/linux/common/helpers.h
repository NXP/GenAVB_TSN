/*
 * Copyright 2018 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _COMMON_HELPERS_H_
#define _COMMON_HELPERS_H_

int h_strtoul(unsigned long int *output, const char *nptr, char **endptr, int base);
int h_strtoull(unsigned long long *output, const char *nptr, char **endptr, int base);

#define offset_of(type, member) ((unsigned long)&(((type *)0)->member))
#define container_of(entry, type, member) ((type *)((unsigned char *)(entry) - offset_of(type, member)))

#endif /* _COMMON_HELPERS_H_ */
