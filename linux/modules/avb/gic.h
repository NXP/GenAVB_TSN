/*
 * AVB gic driver
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

#ifndef _GIC_H_
#define _GIC_H_

#define SCU_BASE_ADDR	0x00a00000

struct gic {
	void *scu_base_addr;
	void *cpu_baseaddr;
	void *dist_base_addr;
};

unsigned int gic_intack(struct gic *gic);
void gic_eoi(struct gic *gic, unsigned int irq);
void gic_irq_set(struct gic *gic, unsigned int irq);
void gic_set_priority(struct gic *gic, unsigned int irq, int priority);
void gic_set_fiq_secure(struct gic *gic, unsigned int irq);
void gic_fiq_init(struct gic *gic, unsigned int irq);
int gic_local_init(struct gic *gic);
void gic_local_exit(struct gic *gic);

void gic_mask(struct gic *gic, unsigned int irq);
void gic_unmask(struct gic *gic, unsigned int irq);

#endif /* _GIC_H_ */
