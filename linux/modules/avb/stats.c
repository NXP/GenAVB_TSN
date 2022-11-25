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

#include <linux/math64.h>
#include "stats.h"


void stats_reset(struct stats *s)
{
	s->current_count = 0;
	s->current_min = 0x7fffffff;
	s->current_mean = 0;
	s->current_max = -0x7fffffff;
	s->current_ms = 0;
}


/** stats_update() - Update stats with a given sample.
 * @s: handler for the stats being monitored
 * @val: sample to be added
 *
 * This function adds a sample to the set.
 * If the number of accumulated samples is equal to the requested size of the set, the following indicators will be computed:
 *    . minimum observed value,
 *    . maximum observed value,
 *    . mean value,
 *    . square of the RMS (i.e. mean of the squares)
 *    . square of the standard deviation (i.e. variance)
 */
void stats_update(struct stats *s, s32 val)
{
	s->current_count++;

	s->current_mean += val;
	s->current_ms += (s64)val * val;

	if (val < s->current_min) {
		s->current_min = val;
		if (val < s->abs_min)
			s->abs_min = val;
	}

	if (val > s->current_max) {
		s->current_max = val;
		if (val > s->abs_max)
			s->abs_max = val;
	}

	if (s->current_count == (1U << s->log2_size)) {
		s->ms = s->current_ms >> s->log2_size;
		s->variance = s->ms - ((s->current_mean * s->current_mean) >> (2*s->log2_size));
		s->mean = s->current_mean >> s->log2_size;

		s->min = s->current_min;
		s->max = s->current_max;

		stats_reset(s);
	}
}


/** stats_compute() - Compute current stats event if set size hasn't been reached yet.
 * @s: handler for the stats being monitored
 *
 * This function computes current statistics for the stats:
 *    . minimum observed value,
 *    . maximum observed value,
 *    . mean value,
 *    . square of the RMS (i.e. mean of the squares)
 *    . square of the standard deviation (i.e. variance)
 */
void stats_compute(struct stats *s)
{
	if (s->current_count) {
		s->ms = div_u64(s->current_ms, s->current_count);

		s->variance = s->ms - div64_s64(s->current_mean * s->current_mean, ((s64)s->current_count * s->current_count));

		s->mean = div_s64(s->current_mean, s->current_count);
	} else {
		s->mean = 0;
		s->ms = 0;
		s->variance = 0;
	}

	s->min = s->current_min;
	s->max = s->current_max;
}
