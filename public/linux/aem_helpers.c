/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *    Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *    Neither the name of NXP Semiconductors nor the
 *    names of its contributors may be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 @brief AEM helper functions
 @details Helper functions to handle AEM structures

 Copyright 2014 Freescale Semiconductor, Inc.
 All Rights Reserved.
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

#include "genavb/aem_helpers.h"



struct aem_desc_hdr *aem_entity_load_from_file(const char *name)
{
	struct aem_desc_hdr *aem_desc;
	int fd, i, rc;
	off_t size;
	unsigned int offset = 0, len;

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

	aem_desc = malloc(AEM_NUM_DESC_TYPES * sizeof(struct aem_desc_hdr) + size);
	if (!aem_desc) {
		printf("entity(%s) malloc() failed\n", name);
		goto err_malloc;
	}

	memset(aem_desc, 0, AEM_NUM_DESC_TYPES * sizeof(struct aem_desc_hdr) + size);

	for (i = 0; i < AEM_NUM_DESC_TYPES; i++) {
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
		aem_desc[i].ptr = (char *)aem_desc + offset;

		len = aem_desc[i].size * aem_desc[i].total;

		rc = read(fd, aem_desc[i].ptr, len);
		if (rc != len) {
			printf("entity(%s) read, desc data(%d) failed: %s %d\n", name, i, strerror(errno), rc);
			goto err_read;
		}

		offset += len;
	}

	printf("Loaded AVDECC entity(%s)\n", name);

	close(fd);

	return aem_desc;

err_read:
	free(aem_desc);

err_malloc:
err_size:
	close(fd);

err_open:
	return NULL;
}
