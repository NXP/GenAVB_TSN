/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _AEM_MANAGER_H_
#define _AEM_MANAGER_H_

#include "genavb/aem_helpers.h"
#include "genavb/types.h"
#include "genavb/net_types.h"


struct aem_desc_handler {
	void (*print)(struct aem_desc_hdr *aem_desc, int level, int min, int max);
	void (*fixup)(struct aem_desc_hdr *aem_desc);
	int (*check)(struct aem_desc_hdr *aem_desc);
	void (*update_name)(struct aem_desc_hdr *aem_desc, char *name);
};

#endif /* _AEM_MANAGER_H_ */
