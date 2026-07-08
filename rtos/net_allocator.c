/*
 * Copyright 2025-2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief RTOS specific Network service implementation
 @details
*/

#include "rtos_abstraction_layer.h"

#include "net_allocator.h"
#include "net_port.h"

#define NET_ALLOC_BLOCK_SHIFT	7U
#define NET_ALLOC_BLOCK_SIZE	(1U << NET_ALLOC_BLOCK_SHIFT) /*< Minimum block size, in bytes */

#define NET_ALLOC_MAX_SIZES		(((NET_DATA_SIZE + NET_ALLOC_BLOCK_SIZE - 1) >> NET_ALLOC_BLOCK_SHIFT) + 1) /*< Size of cache linked list array */

#define NET_ALLOC_BLOCK_MAX		((4 * 1024) >> NET_ALLOC_BLOCK_SHIFT) /*< Maximum number of blocks in the cache */

struct net_allocator {
	void *head[NET_ALLOC_MAX_SIZES];	/*< Simply linked list of buffers in the cache, each entry
	                                    corresponds to buffers with a fixed number of blocks */
	unsigned int blocks;				/*< Total number of blocks present in the cache */
};

struct net_allocator_hdr {
	void *next;				/*< next available buffer in the linked list */
	uint16_t index;			/*< index of this buffer (i.e, number of blocks) */
	uint16_t offset;		/*< offset of the allocator header in the buffer */
};

#define NET_HEADROOM			(sizeof(struct net_allocator_hdr) + NET_DATA_OFFSET)

static struct net_allocator alloc;

/** Network allocator
 *
 * Implement a cache of network descriptors to avoid invoking RTOS heap allocator.
 * The cache is an array of simply linked lists of buffers.
 * All allocation sizes are rounded up to a multiple (N) of ::NET_ALLOC_BLOCK_SIZE.
 * Each list contains buffers of a fixed number of blocks.
 * The cache is locked by a global spinlock.
 * When allocating:
 * - If linked list is empty, allocate from RTOS heap
 * - Otherwise, remove buffer from the list and decrease block count.
 *
 * When freeing:
 * - If amount of blocks in the cache is above @NET_ALLOC_BLOCK_MAX, free to RTOS heap.
 * - Otherwise, add buffer to the list and increase block count.
 *
 * The state of each buffer is tracked in a struct ::net_allocator_hdr, embedded in the buffer.
 * The payload of the buffer (after the network descriptor) is aligned to ::NET_ALLOC_ALIGNMENT.
 * The layout of the buffer is:
 *
 * | offset | struct net_allocator_hdr | struct net_rx/tx_desc | payload |
 *
 * offset is of variable length, to make sure payload is aligned for each buffer.
 */
void *__net_alloc(unsigned int size)
{
	unsigned int index = (size + NET_ALLOC_BLOCK_SIZE - 1) >> NET_ALLOC_BLOCK_SHIFT;
	struct net_allocator_hdr *hdr;
	unsigned int offset;
	uint8_t *addr;

	if (unlikely(index >= NET_ALLOC_MAX_SIZES))
		return NULL;

	rtos_spin_lock(&rtos_global_spinlock, &rtos_global_key);

	if (likely(alloc.head[index] != NULL)) {

		addr = alloc.head[index];
		hdr = (struct net_allocator_hdr *)addr - 1;
		alloc.head[index] = hdr->next;
		alloc.blocks -= index;

		rtos_spin_unlock(&rtos_global_spinlock, rtos_global_key);

	} else {

		rtos_spin_unlock(&rtos_global_spinlock, rtos_global_key);

		size = NET_ALLOC_BLOCK_SIZE * index;
		size += NET_HEADROOM; 		/* reserve space for descriptor and allocator data */
		size += NET_ALLOC_ALIGNMENT; /* reserve space for alignment */

		addr = rtos_net_malloc(size);
		if (!addr)
			goto exit;

		addr += NET_HEADROOM;
		offset = NET_ALLOC_ALIGNMENT - (((uintptr_t)addr) & (NET_ALLOC_ALIGNMENT - 1));
		addr += offset;				/* aligned payload address */
		addr -= NET_DATA_OFFSET;	/* descriptor address */

		hdr = (struct net_allocator_hdr *)addr - 1;
		hdr->index = index;
		hdr->offset = offset;
	}

exit:
	return addr;
}

void __net_free(void *addr)
{
	struct net_allocator_hdr *hdr = (struct net_allocator_hdr *)addr - 1;

	rtos_spin_lock(&rtos_global_spinlock, &rtos_global_key);

	if (likely(alloc.blocks <= NET_ALLOC_BLOCK_MAX)) {
		hdr->next = alloc.head[hdr->index];
		alloc.head[hdr->index] = addr;

		alloc.blocks += hdr->index;

		rtos_spin_unlock(&rtos_global_spinlock, rtos_global_key);

	} else {
		rtos_spin_unlock(&rtos_global_spinlock, rtos_global_key);

		rtos_net_free((uint8_t *)hdr - hdr->offset);
	}
}
