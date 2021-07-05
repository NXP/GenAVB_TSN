/*
* Copyright 2014 Freescale Semiconductor, Inc.
* Copyright 2020 NXP
* 
* NXP Confidential. This software is owned or controlled by NXP and may only 
* be used strictly in accordance with the applicable license terms.  By expressly 
* accepting such terms or by downloading, installing, activating and/or otherwise 
* using the software, you are agreeing that you have read, and that you agree to 
* comply with and are bound by, such license terms.  If you do not agree to be 
* bound by the applicable license terms, then you may not retain, install, activate 
* or otherwise use the software.
*/

/**
 @file
 @brief Statistics service implementation
 @details
*/

#ifndef _COMMON_STATS_H_
#define _COMMON_STATS_H_

#include "common/types.h"

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

	void *priv;
	void (*func)(struct stats *s);
};

void stats_reset(struct stats *s);
void stats_print(struct stats *s);
void stats_update(struct stats *s, s32 val);
void stats_compute(struct stats *s);


/** Initialize a stats structure.
 * @s: 			Pointer to structure to be initialized
 * @log2_size:	Set size to be reached before statistics are computed, expressed as a power of 2
 * @priv:		private field for use by func
 * @func:		pointer to the function to be called when stats are computed
 *
 */
static inline void stats_init(struct stats *s, unsigned int log2_size, void *priv, void (*func)(struct stats *s))
{
	s->log2_size = log2_size;
	s->priv = priv;
	s->func = func;

	s->abs_min = 0x7fffffff;
	s->abs_max = -0x7fffffff;

	stats_reset(s);
}


#endif /* _COMMON_STATS_H_ */
