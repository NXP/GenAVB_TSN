/*
 * AVB GPT driver
 * Copyright 2016 - 2022 NXP
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

#ifndef _GPT_H_
#define _GPT_H_

#ifdef __KERNEL__

bool is_gpt_hw_timer_available(void);
int gpt_init(struct platform_driver *);
void gpt_exit(struct platform_driver *);

#endif /* __KERNEL__ */

#endif /* _GPT_H_ */
