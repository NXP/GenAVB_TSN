/*
 * Copyright 2018 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdlib.h>
#include <errno.h>

#include "helpers.h"

int h_strtoul(unsigned long *output, const char *nptr, char **endptr, int base)
{
	errno = 0;

	*output = strtoul(nptr, endptr, base);

	if (errno != 0)
		return -1;

	return 0;
}


int h_strtoull(unsigned long long *output, const char *nptr, char **endptr, int base)
{
	errno = 0;

	*output = strtoull(nptr, endptr, base);

	if (errno != 0)
		return -1;

	return 0;
}
