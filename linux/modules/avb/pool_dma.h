/*
 * AVB DMA buffer pool
 * Copyright 2014-2015 Freescale Semiconductor, Inc.
 * Copyright 2020 NXP
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef _POOL_DMA_H_
#define _POOL_DMA_H_

#ifdef __KERNEL__
#include <linux/dma-mapping.h>

#include "pool.h"

struct pool_dma {
	struct pool pool;

	struct device *dma_device;
	dma_addr_t dma_baseaddr;
	unsigned int map_count;
};

int pool_dma_init(struct pool_dma *pool, void *baseaddr, unsigned int size, unsigned int obj_order);
void pool_dma_exit(struct pool_dma *);
int pool_dma_map(struct pool_dma *pool, struct device *dev);
void pool_dma_unmap(struct pool_dma *pool);

void *pool_dma_alloc(struct pool_dma *pool);
int pool_dma_alloc_array(struct pool_dma *pool, void **addr, unsigned int count);
int pool_dma_alloc_array_shmem(struct pool_dma *pool, unsigned long *addr_shmem, unsigned int count);

int pool_dma_free(struct pool_dma *pool, void *buf);
void pool_dma_free_array(struct pool_dma *pool, void **addr, unsigned int count);
void pool_dma_free_array_shmem(struct pool_dma *pool, unsigned long *addr_shmem, unsigned int count);

void pool_dma_free_virt(void *pool, unsigned long entry);

/**
 * pool_dma_shmem_to_virt() - Convert shared memory relative to kernel virtual address
 * @pool: pointer to the pool dma handle
 * @addr_shmem: shared memory relative address
 *
 * Return: kernel virtual address.
 */
static inline void *pool_dma_shmem_to_virt(struct pool_dma *pool, unsigned long addr_shmem)
{
	return pool_shmem_to_virt(&pool->pool, addr_shmem);
}

/**
 * pool_dma_virt_to_shmem() - Convert kernel virtual to shared memory relative address
 * @pool: pointer to the pool dma handle
 * @addr: kernel virtual address
 *
 * Return: shared memory relative address.
 */
static inline unsigned long pool_dma_virt_to_shmem(struct pool_dma *pool, void *addr)
{
	return pool_virt_to_shmem(&pool->pool, addr);
}

/**
 * pool_dma_virt_to_dma() - Convert kernel virtual to dma address
 * @pool: pointer to the pool dma handle
 * @addr: kernel virtual address
 *
 * Return: dma address.
 */
static inline dma_addr_t pool_dma_virt_to_dma(struct pool_dma *pool, void *addr)
{
	return pool->dma_baseaddr + (addr - pool->pool.baseaddr);
}

#endif /* __KERNEL__ */


#endif /* _POOL_DMA_H_ */
