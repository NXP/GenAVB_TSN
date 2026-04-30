/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <inttypes.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include <genavb/aem.h>
#include <genavb/helpers.h>

#include "memory_objects.h"
#include "helpers.h"

static struct memory_object_entry memory_objects[AEM_NUM_MEMORY_OBJECTS_MAX];
static unsigned int num_memory_objects_configured = 0;

/* Search and point to the memory object corresponding to the address in the tlv_data,
 * if the address is valid and can actually be used to access the memory object.
 * \return		status AECP_ADDRESS_ACCESS_SUCCESS on success otherwise an error status.
 * \param tlv_data	pointer to the tlv_data of the ADDRESS_ACCESS command, containing the start address and range of the memory access.
 * \param obj		output parameter, pointer to the memory object entry matching the address range of the command.
 * \param offset	output parameter, offset between the start address of the memory object and the start address of the memory access.
 */
static int get_memory_object(struct aecp_addr_access_tlv *tlv_data, struct memory_object_entry **obj, avb_u64 *offset)
{
	avb_u16 length = AECP_ADDR_ACCESS_GET_LENGTH(tlv_data);
	avb_u64 addr = ntohll(tlv_data->address);
	int rc = AECP_ADDRESS_ACCESS_SUCCESS;
	avb_u16 type, cfg_idx, desc_idx;
	avb_u64 offset_addr;
	unsigned int i;

	desc_idx = AEM_MEMORY_OBJECT_ADDR_TO_DESC_IDX(addr);
	cfg_idx = AEM_MEMORY_OBJECT_ADDR_TO_CFG_IDX(addr);
	offset_addr = AEM_MEMORY_OBJECT_START_ADDR(addr);
	type = AEM_MEMORY_OBJECT_ADDR_TO_TYPE(addr);

	*obj = NULL;

	for (i = 0; i < num_memory_objects_configured; i++) {
		if ((desc_idx == memory_objects[i].obj_attr.desc_idx) &&
		    (cfg_idx == memory_objects[i].obj_attr.cfg_idx) &&
		    (type == memory_objects[i].obj_attr.object_type)) {

			if (offset_addr < memory_objects[i].obj_attr.len) {
				if ((offset_addr + length) > memory_objects[i].obj_attr.len) {
					printf("%s: Error: address(%016"PRIx64") matching memory object(%u) goes past the end on the memory object\n",
						__func__, addr, desc_idx);
					rc = AECP_ADDRESS_ACCESS_ADDRESS_INVALID;

				} else {
					/* Memory object found and matching the requested address range from the command's tlv_data */
					*obj = &memory_objects[i];
					*offset = offset_addr;
				}

			} else {
				printf("%s: Error: address(%016"PRIx64") matching memory object(%u) is out of bound (too high)\n",
					__func__, addr, desc_idx);
				rc = AECP_ADDRESS_ACCESS_ADDRESS_TOO_HIGH;
			}

			goto out;
		}
	}

	if (!(*obj)) {
		printf("%s: Error: no memory object mapped to address(%016"PRIx64")\n", __func__, addr);
		rc = AECP_ADDRESS_ACCESS_ADDRESS_INVALID;
	}

out:
	return rc;
}

int aecp_address_access_handler(avb_u16 tlv_count, void *tlv_data)
{
	struct aecp_addr_access_tlv *cur_tlv_data = (struct aecp_addr_access_tlv *)tlv_data;
	int rc = AECP_ADDRESS_ACCESS_SUCCESS;
	unsigned int i;
	avb_u16 length;
	avb_u8 mode;

	for (i = 0; i < tlv_count; i++) {
		length = AECP_ADDR_ACCESS_GET_LENGTH(cur_tlv_data);
		mode = cur_tlv_data->mode;

		switch (mode) {
		case AECP_ADDR_ACCESS_MODE_READ:
		{
			struct memory_object_entry *obj;
			avb_u64 offset = 0;

			rc = get_memory_object(cur_tlv_data, &obj, &offset);
			if (rc != AECP_ADDRESS_ACCESS_SUCCESS)
				goto err;

			memcpy((char *)(cur_tlv_data + 1), (char *)(obj->addr + offset), length);

			break;
		}
		default:
			/* WRITE and EXECUTE are NOT_IMPLEMENTED, so skip them */
			rc = AECP_ADDRESS_ACCESS_NOT_IMPLEMENTED;
			goto err;
		}

		cur_tlv_data = (struct aecp_addr_access_tlv *)((char *)cur_tlv_data + length + sizeof(struct aecp_addr_access_tlv));
	}

err:
	return rc;
}

static int memory_object_memory_access_init(struct memory_object_entry *memory_object, const char *object_file_name)
{
	void *mem_obj_map;
	struct stat stat;
	avb_u64 fsize;
	FILE *fMemObj;
	int fd;

	fMemObj = fopen(object_file_name, "r");
	if (fMemObj == NULL) {
		printf("%s: Error: fopen(%s) failed: %s\n", __func__, object_file_name, strerror(errno));
		goto err;
	}

	fd = fileno(fMemObj);
	if (fd < 0) {
		printf("%s: Error: fileno(%s) failed: %s\n", __func__, object_file_name, strerror(errno));
		goto err_file_process;
	}


	if (fstat(fd, &stat) == -1) {
		printf("%s: Error: fstat(%s) failed: %s\n", __func__, object_file_name, strerror(errno));
		goto err_file_process;
	}

	fsize = stat.st_size;

	if (fsize == 0) {
		printf("%s: Error: file(%s) is empty and can not be memory mapped\n", __func__, object_file_name);
		goto err_file_process;
	}

	if (fsize > memory_object->obj_attr.max_len) {
		printf("%s: Error: file(%s)'s size(%" PRIu64 ") > configured max_len(%" PRIu64 ") for this memory object\n",
			__func__, object_file_name, fsize, memory_object->obj_attr.max_len);
		goto err_file_process;
	}

	mem_obj_map = mmap(NULL, fsize, PROT_READ, MAP_PRIVATE, fd, 0);
	if (mem_obj_map == MAP_FAILED) {
		printf("%s: Error: file(%s) mmap() failed: %s\n", __func__, object_file_name, strerror(errno));
		goto err_file_process;
	}

	memory_object->obj_attr.len = fsize;
	memory_object->fp = fMemObj;
	memory_object->addr = mem_obj_map;

	return 0;

err_file_process:
	if (fclose(fMemObj))
		printf("%s: Error: fclose(%s) failed: %s\n", __func__, object_file_name, strerror(errno));
err:
	return -1;
}

static void memory_object_memory_access_exit(unsigned int obj_idx)
{
	if (fclose(memory_objects[obj_idx].fp))
		printf("%s: Error: memory_object(%u) fclose() failed: %s\n", __func__, obj_idx, strerror(errno));
}

#define MEMORY_OBJECT_PARAMS_PER_ENTRY	6
static int memory_objects_process_config(const char *memory_objects_cfg_file_name)
{
	char file_path[MAX_MEMORY_OBJECT_PATH_STRING + 1];
	char *mem_obj_entry = NULL;
	unsigned int read_char;
	unsigned int i = 0;
	size_t len = 0;
	FILE *fMemObj;
	int rc = 0;

	if (!memory_objects_cfg_file_name) {
		printf("%s: Warning: No memory objects config filename provided\n", __func__);
		goto exit;
	}

	fMemObj = fopen(memory_objects_cfg_file_name, "r");
	if (fMemObj == NULL) {
		printf("%s: Error: fopen(%s) failed: %s\n", __func__, memory_objects_cfg_file_name, strerror(errno));
		goto err;
	}

	while (((read_char = getline(&mem_obj_entry, &len, fMemObj)) != -1)) {
		if (read_char != strlen(mem_obj_entry)) {
			printf("%s: Error: Unexpected embedded null byte(s)\n", __func__);
			goto err_parse;
		}

		if (*mem_obj_entry == '#')	/* discard commented lines */
			continue;

		if (i >= AEM_NUM_MEMORY_OBJECTS_MAX) {
			printf("%s: Error: Reached maximum memory objects %u, skip any remaining object\n", __func__, AEM_NUM_MEMORY_OBJECTS_MAX);
			break;
		}

		if (sscanf(mem_obj_entry, "%hu %hu %hu %"SCNu64" %"SCNu64" %" VAL_TO_STR(MAX_MEMORY_OBJECT_PATH_STRING) "[^\n]",
			    &memory_objects[i].obj_attr.cfg_idx, &memory_objects[i].obj_attr.desc_idx, &memory_objects[i].obj_attr.object_type,
			    &memory_objects[i].obj_attr.max_len, &memory_objects[i].obj_attr.max_segment_len, file_path) != MEMORY_OBJECT_PARAMS_PER_ENTRY) {
			printf("%s: Error: Can not read memory object(%u) from file %s\n", __func__, i, memory_objects_cfg_file_name);
			goto err_parse;
		}

		file_path[MAX_MEMORY_OBJECT_PATH_STRING] = '\0';

		if (memory_objects[i].obj_attr.object_type >= AEM_MEMORY_OBJECT_TYPE_MAX) {
			printf("%s: Error: Invalid memory object(%u) type(%u) >= max(%u)\n",
				__func__, i, memory_objects[i].obj_attr.object_type, AEM_MEMORY_OBJECT_TYPE_MAX);
			goto err_parse;
		}

		if (memory_objects[i].obj_attr.desc_idx >= AEM_NUM_MEMORY_OBJECTS_MAX) {
			printf("%s: Error: Invalid memory object(%u) desc_id(%u) >= max(%u)\n",
				__func__, i, memory_objects[i].obj_attr.desc_idx, AEM_NUM_MEMORY_OBJECTS_MAX);
			goto err_parse;
		}

		if ((memory_objects[i].obj_attr.max_len > AEM_MEMORY_OBJECT_START_ADDR_MASK) || (memory_objects[i].obj_attr.max_segment_len > AEM_MEMORY_OBJECT_START_ADDR_MASK)) {
			printf("%s: Error: Unsupported memory object(%u) max_len(%"PRIu64") and/or segment max_len(%"PRIu64") > max(%"PRIu64")\n",
				__func__, i, memory_objects[i].obj_attr.max_len, memory_objects[i].obj_attr.max_segment_len, AEM_MEMORY_OBJECT_START_ADDR_MASK);
			goto err_parse;
		}

		if (memory_object_memory_access_init(&memory_objects[i], file_path) < 0) {
			printf("%s: Error: Invalid memory object(%u) file(%s)\n", __func__, i, file_path);
			goto err_parse;
		}

		i++;
	}

	rc = i;

	free(mem_obj_entry);

	if (fclose(fMemObj)) {
		printf("%s: Error: fclose(%s) failed\n", __func__, memory_objects_cfg_file_name);
		rc = -1;
	}
exit:
	return rc;

err_parse:
	while (i > 0) {
		i--;

		munmap(memory_objects[i].addr, memory_objects[i].obj_attr.len);

		memory_object_memory_access_exit(i);
	}

	free(mem_obj_entry);

	if (fclose(fMemObj))
		printf("%s: Error: fclose(%s) failed\n", __func__, memory_objects_cfg_file_name);
err:
	return -1;
}

int memory_objects_init(const char *memory_objects_cfg_file_name, struct memory_object_common *stack_msg)
{
	unsigned int i;
	int rc = 0;

	if ((rc = memory_objects_process_config(memory_objects_cfg_file_name)) < 0) {
		printf("%s: Error: Invalid memory objects config(%s)\n", __func__, memory_objects_cfg_file_name);
		goto err;
	}

	num_memory_objects_configured = rc;

	for (i = 0; i < rc; i++)
		memcpy(&stack_msg[i], &memory_objects[i].obj_attr, sizeof(struct memory_object_common));

	return rc;

err:
	return -1;
}

void memory_objects_exit(void)
{
	unsigned int i;

	for (i = 0; i < num_memory_objects_configured; i++) {
		munmap(memory_objects[i].addr, memory_objects[i].obj_attr.len);

		memory_object_memory_access_exit(i);
	}

	num_memory_objects_configured = 0;
}
