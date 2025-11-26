/*
 * Glue layer code between Linux kernel code and common code under common/os/
 * Copyright 2024-2025 NXP
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef _COMMON_OS_H_
#define _COMMON_OS_H_

#ifdef __KERNEL__

#include <linux/math64.h>
#include <linux/types.h>
#include <linux/printk.h>

/* Map common_os_atomic declarations to Linux atomic API */
#define common_os_atomic_t      atomic_t
#define common_os_atomic_read   atomic_read
#define common_os_atomic_set    atomic_set
#define common_os_atomic_add    atomic_add
#define common_os_atomic_xchg   atomic_xchg


#define common_os_div64_u64     div64_u64
#define common_os_div64_s64     div64_s64
#define common_os_div_s64       div_s64
#define common_os_div_u64       div_u64
#define common_os_do_div        do_div

#define common_os_log_err(...)     pr_err(__VA_ARGS__)
#define common_os_log_info(...)    pr_info( __VA_ARGS__)

#endif /* __KERNEL__ */

#endif /* _COMMON_OS_H_ */
