/*
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
 @brief SJA specific FQTSS service implementation
 @details
*/
#ifndef _LINUX_FQTSS_SJA_H_
#define _LINUX_FQTSS_SJA_H_

#if defined(CONFIG_SJA1105)
int fqtss_sja_set_oper_idle_slope(unsigned int port_id, uint8_t traffic_class, unsigned int idle_slope);
int fqtss_sja_init(void);
void fqtss_sja_exit(void);
#else
static inline int fqtss_sja_set_oper_idle_slope(unsigned int port_id, uint8_t traffic_class, unsigned int idle_slope) { return -1; }
static inline int fqtss_sja_init(void) { return 0; }
static inline void fqtss_sja_exit(void) { }
#endif

#endif /* _LINUX_FQTSS_SJA_H_ */
