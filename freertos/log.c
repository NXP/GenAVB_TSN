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
 @brief FreeRTOS logging services
 @details Linux logging services implementation
*/

#include "common/log.h"
#include "sdk_printf.h"

void _os_log_raw(const char *format, ...)
{
	va_list ap;

	va_start(ap, format);

	sdk_vprintf(format, ap);

	va_end(ap);
}


void _os_log(const char *level, const char *func, const char *component, const char *format, ...)
{
	va_list ap;

	sdk_printf("%-4s %11u.%09u %-6s %-32.32s : ", level, (unsigned int)log_time_s, (unsigned int)log_time_ns, component, func);

	va_start(ap, format);

	sdk_vprintf(format, ap);

	va_end(ap);
}

