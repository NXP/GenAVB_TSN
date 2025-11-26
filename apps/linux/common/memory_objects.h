/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __MEMORY_OBJECTS_H__
#define __MEMORY_OBJECTS_H__

#include <genavb/genavb.h>

#define MAX_MEMORY_OBJECT_PATH_STRING	255

#define memory_object_common genavb_msg_memory_object_desc_attr

struct memory_object_entry {
	struct memory_object_common obj_attr;	/* memory object's common attributes */
	FILE *fp;		/* memory object's file pointer */
	void *addr;		/* memory object's memory mapped address */
};

int aecp_address_access_handler(avb_u16 tlv_count, void *tlv_data);
int memory_objects_init(const char *memory_objects_cfg_file_name, struct memory_object_common *stack_msg);
void memory_objects_exit(void);

#endif /* __MEMORY_OBJECTS_H__ */
