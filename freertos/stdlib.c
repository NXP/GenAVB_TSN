/*
* Copyright 2018, 2020, 2023 NXP
*
* SPDX-License-Identifier: BSD-3-Clause
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
