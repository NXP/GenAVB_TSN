/*
 * Copyright 2020, 2022 NXP
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
 @file		helpers.c
 @brief		Helper functions
 @details	Various common helpers for Linux
 */

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
