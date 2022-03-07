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


#define _GNU_SOURCE

#include <stdlib.h>
#include <math.h>
#include <time.h>

#include "os/stdlib.h"

void *os_malloc(size_t size)
{
	return malloc(size);
}

void os_free(void *p)
{
	free(p);
}

int os_abs(int j)
{
	return abs(j);
}

double os_fabs(double j)
{
	return fabs(j);
}

long long int os_llabs(long long int j)
{
	return llabs(j);
}

void os_random_init(void)
{
	struct timespec ts;

	if (clock_gettime(CLOCK_REALTIME, &ts) < 0) {
		/* Handle error */
		srandom(time(NULL));
	} else {
		srandom(ts.tv_nsec ^ ts.tv_sec);
	}
}

long int os_random(void)
{
	return random();
}
