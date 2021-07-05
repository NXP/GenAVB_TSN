/*
* Copyright 2020-2021 NXP
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
 @brief Linux specific osal entry points
 @details
*/

#include "net.h"
#include "os/fdb.h"
#include "shmem.h"
#include "init.h"
#include "clock.h"
#include "os_config.h"
#include "net_logical_port.h"
#include "rtnetlink.h"

__attribute__((weak)) int shmem_init(void) { return 0; };
__attribute__((weak)) void shmem_exit(void) { };
__attribute__((weak)) int net_init(void) { return 0; };
__attribute__((weak)) void net_exit(void) { };
__attribute__((weak)) int fdb_init(void) { return 0; };
__attribute__((weak)) void fdb_exit(void) { };
__attribute__((weak)) int fqtss_init(void) { return 0; };
__attribute__((weak)) void fqtss_exit(void) { };
__attribute__((weak)) int rtnetlink_socket_init(void) { return 0; };
__attribute__((weak)) void rtnetlink_socket_exit(void) { };

/** Initialize random seed
 * \return	none
 */
extern void os_random_init(void);

int os_init(void)
{
	struct os_config config = {0, };

	if (os_config_get(&config) < 0)
		goto err_config;

	/*
	* Application / Driver shared memory initialization
	*/
	if (shmem_init() < 0)
		goto err_shmem;

	logical_port_init(&config.logical_port_config);

	/*
	* Clock layer global init.
	*/
	if (os_clock_init(&config.clock_config) < 0)
		goto err_clock;

	/*
	* Network layer global init.
	*/
	if (net_init() < 0)
		goto err_net;

	/*
	* Network layer global init.
	*/
	if (rtnetlink_socket_init() < 0)
		goto err_rtnetlink;

	/*
	* FDB layer global init.
	*/
	if (fdb_init() < 0)
		goto err_fdb;

	/*
	* FQTSS layer global init.
	*/
	if (fqtss_init() < 0)
		goto err_fqtss;

	/*
	* random global init.
	*/
	os_random_init();

	return 0;

err_fqtss:
	fdb_exit();

err_fdb:
	rtnetlink_socket_exit();

err_rtnetlink:
	net_exit();

err_net:
	os_clock_exit();

err_clock:
	shmem_exit();

err_shmem:
err_config:
	return -1;
}

void os_exit(void)
{
	fqtss_exit();
	fdb_exit();
	rtnetlink_socket_exit();
	net_exit();
	os_clock_exit();
	shmem_exit();
}
