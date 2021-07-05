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
 @brief Logging services
 @details Logging services implementation
*/

#ifndef _OS_LOG_H_
#define _OS_LOG_H_

/** Print log messages to the standard output
 *
 * \return none
 * \param level	string of the current logging level ("CRIT", "ERR", "INIT", "INFO", "DEBUG")
 * \param func	string of the calling function
 * \param component	string of the calling component
 * \param format	string of the print format
 */
void _os_log(const char *level, const char *func, const char *component, const char *format, ...);

#include "osal/log.h"

#endif /* _OS_LOG_H_ */
