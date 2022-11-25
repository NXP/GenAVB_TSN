/*
 * AVB epit driver
 * Copyright 2014-2015 Freescale Semiconductor, Inc.
 * Copyright 2017, 2020, 2022 NXP
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

bool is_epit_hw_timer_available(void);
int epit_init(struct platform_driver *);
void epit_exit(struct platform_driver *);

#endif /* __KERNEL__ */

#endif /* _EPIT_H_ */
