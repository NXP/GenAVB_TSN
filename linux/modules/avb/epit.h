/*
 * AVB epit driver
 * Copyright 2014-2015 Freescale Semiconductor, Inc.
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

#ifndef _EPIT_H_
#define _EPIT_H_

#ifdef __KERNEL__

#include <asm/io.h>
#include "hw_timer.h"

/* Registers */
#define EPIT_CR		0x00
#define EPIT_SR		0x04
#define EPIT_LR		0x08
#define EPIT_CMPR	0x0c
#define EPIT_CNR	0x10

/* CR register bits */
#define EPIT_CR_EN(x)		(((x) & 0x1) << 0)
#define EPIT_CR_ENMOD(x)	(((x) & 0x1) << 1)
#define EPIT_CR_OCIEN(x)	(((x) & 0x1) << 2)
#define EPIT_CR_RLD(x)		(((x) & 0x1) << 3)
#define EPIT_CR_PRESCALAR(x)	(((x) & 0xfff) << 4)
#define EPIT_CR_SWR(x)		(((x) & 0x1) << 16)
#define EPIT_CR_IOVW(x)		(((x) & 0x1) << 17)
#define EPIT_CR_DBGEN(x)	(((x) & 0x1) << 18)
#define EPIT_CR_WAITEN(x)	(((x) & 0x1) << 19)
#define EPIT_CR_STOPEN(x)	(((x) & 0x1) << 21)
#define EPIT_CR_OM(x)		(((x) & 0x3) << 22)
#define EPIT_CR_CLKSRC(x)	(((x) & 0x3) << 24)

#define EPIT_CTRL (EPIT_CR_ENMOD(1) | EPIT_CR_RLD(0) | EPIT_CR_PRESCALAR(0) | \
			EPIT_CR_SWR(0) | EPIT_CR_IOVW(0) | EPIT_CR_DBGEN(0) | EPIT_CR_WAITEN(1) | \
			EPIT_CR_STOPEN(0) | EPIT_CR_OM(0) | EPIT_CR_CLKSRC(2))

struct epit {
	void *baseaddr;
	int irq;

	resource_size_t start;
	resource_size_t size;

	struct clk *clk;

	struct hw_timer_dev timer_dev;

	u32 next_cycles;
};

int epit_init(struct platform_driver *);
void epit_exit(struct platform_driver *);

#endif /* __KERNEL__ */

#endif /* _EPIT_H_ */
