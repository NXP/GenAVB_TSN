/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2018, 2023, 2025-2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 \file aem_helpers.c
 \brief AEM helper functions
 \details Helper functions to handle AEM structures
*/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "genavb/aem.h"
#include "genavb/config.h"

#include "genavb/aem_helpers.h"



/* cfg_size is an array of the size of each configuration in bytes:
 * The returned pointer refers to the start of memory region of all configuration arrays:
 * start of config0 array = returned aem_desc
 * start of config1 array = start of config0 array + cfg_size[0] ...
 */
struct aem_desc_hdr *aem_entity_load_from_file(const char *name, unsigned int *cfg_size, unsigned int *cfg_count)
{
	struct aem_desc_hdr *aem_desc, *aem_desc_start;
	unsigned int offset, len, total_offset = 0;
	int fd, i, rc;
	off_t size;
	avb_u32 magic;

	fd = open(name, O_RDONLY);
	if (fd < 0) {
		printf("entity(%s) open() failed: %s\n", name, strerror(errno));
		goto err_open;
	}

	size = lseek(fd, 0, SEEK_END);
	lseek(fd, 0, SEEK_SET);

	if (size > (1024 * 1024)) {
		printf("entity(%s) too big (%llu)\n", name, (unsigned long long)size);
		goto err_size;
	}

	aem_desc = malloc(CFG_MAX_AEM_CONFIGS * AEM_NUM_DESC_TYPES * sizeof(struct aem_desc_hdr) + size);
	if (!aem_desc) {
		printf("entity(%s) malloc() failed\n", name);
		goto err_malloc;
	}

	memset(aem_desc, 0, CFG_MAX_AEM_CONFIGS * AEM_NUM_DESC_TYPES * sizeof(struct aem_desc_hdr) + size);

	*cfg_count = 0;

	aem_desc_start = aem_desc;

start:
	for (i = 0; i < AEM_NUM_DESC_TYPES; i++) {

		/* ENTITY and CONFIGURATION descriptors are shared accross all configurations
		 * and only the first configurations holds them. Point all following configurations
		 * to the first one for these descriptors.
		 */
		if (*cfg_count != 0 && (i == AEM_DESC_TYPE_ENTITY || i == AEM_DESC_TYPE_CONFIGURATION)) {
			aem_desc[i].total = aem_desc_start[i].total;
			aem_desc[i].size = aem_desc_start[i].size;
			continue;
		}

		rc = read(fd, &aem_desc[i].total, sizeof(avb_u16));
		if (rc != sizeof(avb_u16)) {
			printf("entity(%s) read() desc header(%d) failed: %s %d\n", name, i, strerror(errno), rc);
			goto err_read;
		}

		if (aem_desc[i].total > AEM_DESC_MAX_NUM) {
			printf("entity(%s) desc header(%d): invalid total desc number: %d\n", name, i, aem_desc[i].total);
			goto err_read;
		}

		rc = read(fd, &aem_desc[i].size, sizeof(avb_u16));
		if (rc != sizeof(avb_u16)) {
			printf("entity(%s) read() desc header(%d) failed: %s %d\n", name, i, strerror(errno), rc);
			goto err_read;
		}

		if (aem_desc[i].size > AEM_DESC_MAX_LENGTH) {
			printf("entity(%s) desc header(%d): invalid length: %d\n", name, i, aem_desc[i].size);
			goto err_read;
		}
	}

	offset = AEM_NUM_DESC_TYPES * sizeof(struct aem_desc_hdr);

	for (i = 0; i < AEM_NUM_DESC_TYPES; i++) {
		/* ENTITY and CONFIGURATION descriptors are shared accross all configurations
		 * and only the first configurations holds them. Point all following configurations
		 * to the first one for these descriptors.
		 */
		if (*cfg_count != 0 && (i == AEM_DESC_TYPE_ENTITY || i == AEM_DESC_TYPE_CONFIGURATION)) {
			aem_desc[i].ptr = aem_desc_start[i].ptr;
			continue;
		}

		aem_desc[i].ptr = (char *)aem_desc + offset;

		len = aem_desc[i].size * aem_desc[i].total;

		rc = read(fd, aem_desc[i].ptr, len);
		if (rc != len) {
			printf("entity(%s) read, desc data(%d) failed: %s %d\n", name, i, strerror(errno), rc);
			goto err_read;
		}

		offset += len;
	}

	cfg_size[*cfg_count] = offset;
	aem_desc = (void *)((char *)aem_desc + offset);

	total_offset += offset;
	(*cfg_count)++;

	/* Check if there's enough data left in the file (For multi configuration AEMs,
	 * next expected value is a magic number).
	 */
	if (total_offset + sizeof(avb_u32) > size) {
		/* Nothing more to read ... exit. */
		goto exit;
	}

	rc = read(fd, &magic, sizeof(avb_u32));
	if (rc != sizeof(avb_u32)) {
		printf("entity(%s) read() magic number for config(%u) failed: %s %d\n", name, *cfg_count, strerror(errno), rc);
		goto err_read;
	}

	/* Expect a magic number with the index of the following configuration. */
	if (IS_AEM_CONFIG_MAGIC(magic) && magic == AEM_CONFIG_MAGIC(*cfg_count))
		goto start;

exit:
	printf("Loaded AVDECC entity(%s) with (%u) configurations\n", name, *cfg_count);

	close(fd);

	return aem_desc_start;

err_read:
	free(aem_desc_start);

err_malloc:
err_size:
	close(fd);

err_open:
	return NULL;
}
