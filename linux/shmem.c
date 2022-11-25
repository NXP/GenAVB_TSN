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
 @brief Linux specific shared memory allocator
 @details
*/

#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include "shmem.h"
#include "common/log.h"
#include "modules/avbdrv.h"

#define SHMEM_DEV	"/dev/avb"

void *shmem_baseaddr;
int shmem_fd = -1;
static unsigned long shmem_size = 0;

int shmem_init(struct os_net_config *config)
{
	if (config->net_mode != NET_AVB)
		goto exit;

	if (!(shmem_fd < 0))
		goto err;

	shmem_fd = open(SHMEM_DEV, O_RDWR | O_CLOEXEC);
	if (shmem_fd < 0) {
		os_log(LOG_CRIT, "open(%s) %s\n", SHMEM_DEV, strerror(errno));
		goto err_open;
	}

	if (ioctl(shmem_fd, AVBDRV_IOC_SHMEM_SIZE, &shmem_size) < 0) {
		os_log(LOG_ERR, "ioctl() %s\n", strerror(errno));
		goto err_ioctl;
	}

	shmem_baseaddr = mmap(NULL, shmem_size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_LOCKED, shmem_fd, 0);
	if (shmem_baseaddr == MAP_FAILED) {
		os_log(LOG_ERR, "mmap() %s\n", strerror(errno));
		goto err_mmap;
	}

	os_log(LOG_INIT, "%d base (%p), size %lu done\n", shmem_fd, shmem_baseaddr, shmem_size);

exit:
	return 0;

err_ioctl:
err_mmap:
	close(shmem_fd);
	shmem_fd = -1;

err_open:
err:
	return -1;
}

void shmem_exit(void)
{
	if (shmem_fd < 0)
		return;

	munmap(shmem_baseaddr, shmem_size);

	close(shmem_fd);

	shmem_fd = -1;

	os_log(LOG_INIT, "done\n");
}

void *shmem_alloc(void)
{
	unsigned long addr;
	int rc;

	rc = read(shmem_fd, &addr, sizeof(unsigned long));
	if (rc < (int)sizeof(unsigned long)) {
		if (rc < 0)
			os_log(LOG_ERR, "read() %s\n", strerror(errno));
		else
			os_log(LOG_ERR, "read() incomplete\n");

		return NULL;
	}

	return shmem_to_virt(addr);
}


void shmem_free(void *buf)
{
	unsigned long addr = virt_to_shmem(buf);
	int rc;

	rc = write(shmem_fd, &addr, sizeof(unsigned long));
	if (rc < (int)sizeof(unsigned long)) {
		if (rc < 0)
			os_log(LOG_ERR, "write() %s\n", strerror(errno));
		else
			os_log(LOG_ERR, "write() incomplete\n");
	}
}
