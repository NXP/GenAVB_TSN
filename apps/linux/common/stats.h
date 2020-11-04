/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2020 NXP
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *    Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *    Neither the name of NXP Semiconductors nor the
 *    names of its contributors may be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _COMMON_STATS_H_
#define _COMMON_STATS_H_

struct stats {
	unsigned int log2_size;
	unsigned int current_count;

	int current_min;
	int current_max;
	long long current_mean;
	unsigned long long current_ms;

	/* Stats snapshot */
	int min;
	int max;
	int mean;
	unsigned long long ms;
	unsigned long long variance;

	/* absolute min/max (never reset) */
	int abs_min;
	int abs_max;

	void *priv;
	void (*func)(struct stats *s);
};

#define MAX_SLOTS 256

struct hist {
	unsigned int slots[MAX_SLOTS];
	unsigned int n_slots;
	unsigned int slot_size;
};

void stats_reset(struct stats *s);
void stats_print(struct stats *s);
void stats_init(struct stats *s, unsigned int log2_size, void *priv, void (*func)(struct stats *s));
void stats_update(struct stats *s, int val);
void stats_compute(struct stats *s);

int hist_init(struct hist *hist, unsigned int n_slots, unsigned slot_size);
void hist_update(struct hist *hist, unsigned int value);
void hist_reset(struct hist *hist);

void hist_print(struct hist *hist);
#endif /* _COMMON_STATS_H_ */
