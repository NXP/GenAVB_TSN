/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief RTOS specific Network service implementation
 @details
*/

#ifndef _RTOS_NET_ALLOCATOR_H_
#define _RTOS_NET_ALLOCATOR_H_

#define NET_ALLOC_ALIGNMENT 64U

void *__net_alloc(unsigned int size);
void __net_free(void *addr);

#endif /* _RTOS_NET_ALLOCATOR_H_ */