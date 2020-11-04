/*
 * Copyright 2017 NXP
 * All rights reserved.
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

/**
 * @file stream_stats.h
 *
 * @brief This file defines generic stream statistics.
 *
 * @details    Copyright 2017 NXP
 */

#ifndef __STREAM_STATS_H__
#define __STREAM_STATS_H__

#include "avb_stream_config.h"
#include "stats.h"

#define APP_FLAG_FIRST_HANDLE	(1 << 0)

struct stream_stats {
	unsigned long app_ptr;          /**< AVB route data pointer value, just for printout */
	unsigned long avb_stream_ptr;     /**< AVB stream pointer value, just for printout */
	int is_listener;  /**< This is statistic for listener route or not */

	unsigned int period;
	unsigned int flags;
	unsigned int last_stat_dump_time;

	char is_updated_flag;   /**< Flag indicate statistic is updated by data thread */
	aar_avb_stats_t avb_stats;   /**< AVB statistic structure */
};


void stream_stats_dump(struct stream_stats *stats);
int stream_stats_store(struct stream_stats *stats, aar_avb_stats_t *avb_stats);
int stream_stats_is_time(struct stream_stats *stats);
void stream_stats_set_time(struct stream_stats *stats);


static inline int stream_stats_is_updated(struct stream_stats *stats)
{
	return stats->is_updated_flag;
}

static inline void stream_stats_set_updated(struct stream_stats *stats)
{
	stats->is_updated_flag = 1;
}


#endif /* __STREAM_STATS_H__ */
