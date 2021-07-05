/*
* Copyright 2014 Freescale Semiconductor, Inc.
* Copyright 2020 NXP
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
 @brief Linux specific standard library implementation
 @details
*/

#include <string.h>

#include "os/string.h"

void *os_memset(void *s, int c, size_t n)
{
	return memset(s, c, n);
}

void *os_memcpy(void *d, const void *s, size_t n)
{
	return memcpy(d, s, n);
}

void *os_memmove(void *d, const void *s, size_t n)
{
	return memmove(d, s, n);
}

int os_memcmp(const void *s1, const void *s2, size_t n)
{
	return memcmp(s1, s2, n);
}

int os_strnlen(const char *s, size_t maxlen)
{
	int i = 0;
	const char *c = s;

	while (i < maxlen) {
		if (*c == '\0')
			break;

		c++;
		i++;
	}

	return i;
}
