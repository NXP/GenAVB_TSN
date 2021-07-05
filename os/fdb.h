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
 @brief FDB OS abstraction
 @details
*/

#ifndef _OS_FDB_H_
#define _OS_FDB_H_

#include "os/sys_types.h"

typedef enum {
	FDB_PORT_CONTROL_FORWARDING = 0,
	FDB_PORT_CONTROL_FILTERING
} fdb_port_control_t;

struct fdb_port_map {
	unsigned int port_id;
	fdb_port_control_t control;
};

/* 802.1Q-2018, section 8.8.5 */
int fdb_dynamic_vlan_create(u16 vid, struct fdb_port_map *map, unsigned int n);
int fdb_dynamic_vlan_delete(u16 vid);
int fdb_dynamic_vlan_read(u16 vid, struct fdb_port_map *map, unsigned int *n);

/* 802.1Q-2018, section 8.8.7 */
int fdb_dynamic_reservation_create(u8 *mac, u16 vid, struct fdb_port_map *map, unsigned int n);
int fdb_dynamic_reservation_delete(u8 *mac, u16 vid);
int fdb_dynamic_reservation_read(u8 *mac, u16 vid, struct fdb_port_map *map, unsigned int *n);

int fdb_init(void);
void fdb_exit(void);

#include "osal/fdb.h"

#endif /* _OS_FDB_H_ */
