/*
* Copyright 2018, 2020 NXP
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
 @brief FreeRTOS specific standard library implementation
 @details
*/

#include "FreeRTOS.h"

#include "os/stdlib.h"

void *os_malloc(size_t size)
{
	return pvPortMalloc(size);
}

void os_free(void *p)
{
	vPortFree(p);
}

int os_abs(int j)
{
	if (j < 0)
		return -j;
	else
		return j;
}

double os_fabs(double j)
{
	if (j < 0.0)
		return -j;
	else
		return j;
}

long long int os_llabs(long long int j)
{
	if (j < 0)
		return -j;
	else
		return j;
}


long int os_random(void)
{
	return 1;       /* FIXME */
}
