/*
 * Copyright 2020, 2022-2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 \file		helpers.c
 \brief		Helper functions
 \details	Various common helpers for Linux

 \copyright Copyright 2020, 2022-2023 NXP
 */

#include <stdio.h>

#include "genavb/helpers.h"

/** Copy string data from src to dst but won't copy more than the src length, checking the dst buffer is null terminated and without overrun
 * \param dst	pointer to the destination string buffer
 * \param src	pointer to the source string buffer
 * \param dst_len	desired length for the destination buffer
 */
void h_strncpy(char *dst, const char *src, int dst_len)
{
	int src_len;

	src_len = strlen(src) + 1;
	if (src_len > dst_len)
		src_len = dst_len;

	memcpy(dst, src, src_len);
	dst[dst_len - 1] = '\0';
}

/** Copy string data from src to dst, return 0 if successful, -1 otherwise
 * \param dst	pointer to the destination string buffer
 * \param src	pointer to the source string buffer
 * \param dst_len	desired length for the destination buffer
 */
int h_strncpy_strict(char *dst, const char *src, int dst_len)
{
	int rc;

	rc = snprintf(dst, dst_len, "%s", src);
	if (rc < 0 || rc >= dst_len)
		return -1;

	return 0;
}

/** Convert a string to an unsigned long integer with handling errno management
 * \param output	pointer to the output integer variable
 * \param nptr		pointer to the buffer containing the representation of an integral number
 * \param endptr	pointer to the string buffer that would contain the next character in nptr after the numerical value
 * \param base		number base
 * \return      	0 on success, or -1 on error.
 */
int h_strtoul(unsigned long *output, const char *nptr, char **endptr, int base)
{
	errno = 0;

	*output = strtoul(nptr, endptr, base);

	if (errno != 0)
		return -1;

	return 0;
}

/** Convert a string to an unsigned long long integer with handling errno management
 * \param output	pointer to the output integer variable
 * \param nptr		pointer to the buffer containing the representation of an integral number
 * \param endptr	pointer to the string buffer that would contain the next character in nptr after the numerical value
 * \param base		number base
 * \return      	0 on success, or -1 on error.
 */
int h_strtoull(unsigned long long *output, const char *nptr, char **endptr, int base)
{
	errno = 0;

	*output = strtoull(nptr, endptr, base);

	if (errno != 0)
		return -1;

	return 0;
}
