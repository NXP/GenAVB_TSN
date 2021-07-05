/*
 * AVB stats helpers
 * Copyright 2017 NXP
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

#ifndef _STATS_H_
#define _STATS_H_

#include <linux/kernel.h>

struct stats {
	u32 log2_size;
	u32 current_count;

	s32 current_min;
	s32 current_max;
	s64 current_mean;
	u64 current_ms;

	/* Stats snapshot */
	s32 min;
	s32 max;
	s32 mean;
	u64 ms;
	u64 variance;

	/* absolute min/max (never reset) */
	s32 abs_min;
	s32 abs_max;
};

void stats_reset(struct stats *s);
void stats_update(struct stats *s, int val);
void stats_compute(struct stats *s);


/** stats_init() - Initialize a stats structure.
 * @s: 			Pointer to structure to be initialized
 * @log2_size:	Set size to be reached before statistics are computed, expressed as a power of 2
 *
 */
static inline void stats_init(struct stats *s, unsigned int log2_size)
{
	s->log2_size = log2_size;

	s->abs_min = 0x7fffffff;
	s->abs_max = -0x7fffffff;

	stats_reset(s);
}


#endif /* _STATS_H_ */
