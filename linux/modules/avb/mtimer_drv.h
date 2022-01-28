/*
 * AVB media clock timer driver
 * Copyright 2019 NXP
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
#ifndef _MTIMER_DRV_H_
#define _MTIMER_DRV_H_


#ifdef __KERNEL__

#include <linux/cdev.h>
#include <linux/device.h>

#define MTIMER_NAME		"mtimer"

struct mtimer_drv {
	struct cdev cdev;
	dev_t devno;
};

int mtimer_drv_init(struct mtimer_drv *drv, struct dentry *avb_dentry);
void mtimer_drv_exit(struct mtimer_drv *drv);

#endif /* __KERNEL__ */

#define MTIMER_IOC_MAGIC	't'

#define MTIMER_IOC_START	_IOW(MTIMER_IOC_MAGIC, 1, struct mtimer_start)
#define MTIMER_IOC_STOP		_IO(MTIMER_IOC_MAGIC, 2)

#endif /* _MTIMER_DRV_H_ */

