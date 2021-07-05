/*
* Copyright 2021 NXP
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
 @brief SJA specific FDB service implementation
 @details
*/
#ifndef _LINUX_FDB_SJA_H_
#define _LINUX_FDB_SJA_H_

#if defined(CONFIG_SJA1105)
int bridge_rtnetlink(u8 *mac_addr, u16 vid, unsigned int port_id, bool add);
#else
static inline int bridge_rtnetlink(u8 *mac_addr, u16 vid, unsigned int port_id, bool add) { return -1; }
#endif

#endif /* _LINUX_FDB_SJA_H_ */
