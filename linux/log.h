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
 @brief Linux specific monotonic time logging implementation
 @details
*/


#ifndef _LINUX_LOG_H_
#define _LINUX_LOG_H_


/** Enable the monotonic time ouput in log messages.
 * \return none
 */
void log_enable_monotonic(void);


/** Update the monotonic time that will be displayed in log messages.
 * May be called at any time to provide the desired log timestamping accuracy.
 * \return none
 */
int log_update_monotonic(void);

#endif /* _LINUX_LOG_H_ */
