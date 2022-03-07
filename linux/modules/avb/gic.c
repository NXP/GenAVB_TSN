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

#include <asm/io.h>
#include <linux/sched.h>
#include <linux/version.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(3,10,0)
#include <asm/hardware/gic.h>
#define GIC_DIST_IGROUP		0x80
#else
#include <linux/irqchip/arm-gic.h>
#endif

#include "gic.h"

#define SECURITY_EXTENSIONS	1 /* 1 is cpu implements security extensions, 0 if not */
#define SECURE_MODE		1 /* 1 is cpu is running is secure mode, 0 if not */

unsigned int gic_intack(struct gic *gic)
{
	return readl_relaxed(gic->cpu_baseaddr + GIC_CPU_INTACK);
}

void gic_eoi(struct gic *gic, unsigned int irq)
{
	writel_relaxed(irq, gic->cpu_baseaddr + GIC_CPU_EOI);
}

void gic_irq_set(struct gic *gic, unsigned int irq)
{
	writel_relaxed(1 << (irq % 32), gic->dist_base_addr + GIC_DIST_PENDING_SET + (irq / 32) * 4);
}

void gic_mask(struct gic *gic, unsigned int irq)
{
	unsigned int bit = irq % 32;
	unsigned long addr = (irq / 32) * 4;

//	pr_info("%s: %d %lx %x\n", __func__, irq, addr, (1 << bit));
	writel_relaxed((1 << bit), gic->dist_base_addr + 0x180 + addr);
}

void gic_unmask(struct gic *gic, unsigned int irq)
{
	unsigned int bit = irq % 32;
	unsigned long addr = (irq / 32) * 4;

//	pr_info("%s: %d %lx %x\n", __func__, irq, addr, (1 << bit));
	writel_relaxed((1 << bit), gic->dist_base_addr + 0x100 + addr);
}


void gic_set_priority(struct gic *gic, unsigned int irq, int priority)
{
	pr_info("IRQ: %d priority:%x\n", irq, priority);

	writeb_relaxed(priority & 0xff, gic->dist_base_addr + GIC_DIST_PRI + irq);

	pr_info("IRQ: %d priority:%x\n", irq, readb_relaxed(gic->dist_base_addr + GIC_DIST_PRI + irq));
}

void gic_set_fiq_secure(struct gic *gic, unsigned int irq)
{
	unsigned int addr, bit, val;

	pr_info("%s: %d\n", __func__, irq);

	/* Set the IRQ to secure mode/Group 0 */
	addr = (irq / 32) * 4;
	bit = irq % 32;
	val = readl_relaxed(gic->dist_base_addr + GIC_DIST_IGROUP + addr);
	writel_relaxed(val & ~(1 << bit), gic->dist_base_addr + GIC_DIST_IGROUP + addr);

	pr_info("%s: %x %x %x\n", __func__, addr, (1 << bit), readl_relaxed(gic->dist_base_addr + GIC_DIST_IGROUP + addr));
}

/* This may have been done by the main gic code */
/* This can conflict with configuration done by the main gic code */
/* The point is to group all gic configuration in one place, for reference */
void gic_fiq_init(struct gic *gic, unsigned int irq)
{
	pr_info("%s %d\n", __func__, irq);

#if SECURITY_EXTENSIONS

#if SECURE_MODE

	gic_set_fiq_secure(gic, irq);

#else
	/* regular Linux is running in secure mode */
#endif
#else
	/* Both C2000 and i.MX6 implement security extensions */
#endif

	pr_info("%s exit\n", __func__);
}

int gic_local_init(struct gic *gic)
{
	pr_info("%s\n", __func__);

	gic->scu_base_addr = ioremap(SCU_BASE_ADDR, SZ_8K);
	if (!gic->scu_base_addr)
		return -ENOMEM;

	pr_info("%s: mapped %x at %p\n", __func__, SCU_BASE_ADDR, gic->scu_base_addr);

	gic->cpu_baseaddr = gic->scu_base_addr + 0x100;
	gic->dist_base_addr = gic->scu_base_addr + 0x1000;

	pr_info("%s: gic_cpu_baseaddr: %p, gic_dist_baseaddr: %p\n", __func__, gic->cpu_baseaddr, gic->dist_base_addr);

	pr_info("GIC_CPU_CTRL: %08x\n", readl(gic->cpu_baseaddr));
	pr_info("GIC_DIST_ICPIDR2: %08x\n", readl(gic->dist_base_addr + 0xfe8));
	pr_info("GIC_DIST_CTRL: %08x\n", readl(gic->dist_base_addr));

	pr_info("GIC_APBR: %08x\n", readl(gic->cpu_baseaddr + 0x1c));
	pr_info("GIC_BPR: %08x\n", readl(gic->cpu_baseaddr + GIC_CPU_BINPOINT));

	return 0;
}

void gic_local_exit(struct gic *gic)
{
	iounmap(gic->scu_base_addr);
}
