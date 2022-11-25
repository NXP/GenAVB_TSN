/*
 * AVB GPT driver
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
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

#ifndef _TPM_H_
#define _TPM_H_

#ifdef __KERNEL__

#include <linux/time.h>
#include "hw_timer.h"
#include "media_clock_rec_pll.h"

bool is_tpm_hw_timer_available(void);
int tpm_init(struct platform_driver *);
void tpm_exit(struct platform_driver *);

#endif /* __KERNEL__ */

#endif /* _TPM_H_ */
