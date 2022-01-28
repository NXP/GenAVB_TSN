/*
 * AVB ipc service driver
 * Copyright 2014-2015 Freescale Semiconductor, Inc.
 * Copyright 2018-2020 NXP
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

#include <linux/module.h>
#include <linux/slab.h>

#include "ipc.h"

struct ipc_module {
	struct ipc_drv ipc_drv;
};

static struct ipc_module *module;

static int ipc_module_init(void)
{
	int rc;

	module = kzalloc(sizeof(struct ipc_module), GFP_KERNEL);
	if (!module) {
		pr_err("%s: kmalloc() failed\n", __func__);
		rc = -ENOMEM;
		goto err_kmalloc;
	}

	rc = ipcdrv_init(&module->ipc_drv);
	if (rc < 0) {
		pr_err("%s: ipcdrv_init() failed\n", __func__);
		goto err_ipc_drv;
	}

	return 0;

err_ipc_drv:
	kfree(module);
	module = NULL;

err_kmalloc:
	return -1;
}

static void ipc_module_exit(void)
{
	ipcdrv_exit(&module->ipc_drv);

	kfree(module);
	module = NULL;
}

module_init(ipc_module_init);
module_exit(ipc_module_exit);

MODULE_LICENSE("GPL");
