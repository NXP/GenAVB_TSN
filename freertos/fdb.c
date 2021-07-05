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
 @brief FreeRTOS specific FDB service implementation
 @details
*/

#include "os/fdb.h"

int fdb_dynamic_reservation_create(u8 *mac, u16 vid, struct fdb_port_map *map, unsigned int n)
{
	return -1;
}

int fdb_dynamic_reservation_delete(u8 *mac, u16 vid)
{
	return -1;
}

int fdb_init(void)
{
	return -1;
}

void fdb_exit(void)
{
	return;
}
