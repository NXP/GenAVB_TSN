/*
 * Copyright 2016 Freescale Semiconductor, Inc.
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
 @file thread_config.h
 @brief This file defines configurations for thread management.
 @details

 Copyright 2016 Freescale Semiconductor, Inc.
 All Rights Reserved.
*/
#ifndef __THREAD_CONFIG_H__
#define __THREAD_CONFIG_H__

#include <pthread.h>
#include "stats.h"
/**
 * @addtogroup thread
 * @{
 */
#define MAX_THREADS           8     /**< Maximum number of threads can be used for data transfer */
#define MAX_THREAD_SLOTS      10     /**< Maximum number of slots in threads */
/**
 * @brief     Thread capability mask
 */
typedef enum _THREAD_CAPABILITY {
	THR_CAP_STREAM_TALKER   = 0x01, /**< Thread can process AVB talker stream */
	THR_CAP_STREAM_LISTENER = 0x02, /**< Thread can process AVB listener stream */
	THR_CAP_STREAM_AUDIO    = 0x04, /**< Thread can process AVB stream has format AAF */
	THR_CAP_STREAM_CRF      = 0x08, /**< Thread can process AVB stream has format CRF */
	THR_CAP_STREAM_VIDEO	= 0x10,
	THR_CAP_ALSA		= 0x20,
	THR_CAP_GSTREAMER	= 0x40,
	THR_CAP_GSTREAMER_SYNC	= 0x80,
	THR_CAP_STATS		= 0x100,
	THR_CAP_TIMER		= 0x200,
	THR_CAP_CONTROLLED	= 0x400,
	THR_CAP_GST_MULTI	= 0x800,
	THR_CAP_GST_BUS_TIMER	= 0x1000,
	THR_CAP_TSN_LOOP 	= 0x2000,
	THR_CAP_TSN_PT	 	= 0x4000
} thr_capability_t;

/**
 * @brief      Thread slot structure.
 */
typedef struct _THREAD_SLOT {
	char is_used;                    /**< This flag specifies whether the current slot is used or not */
	char is_enabled; 		 /**< This flag specifies whether the current fd is enabled for monitoring or not*/
	int fd;                          /**< File descriptor of slot, this used to mornitoring input events */
	void *data;                      /**< User parameter for slot handler */
	int (*handler)(void *data, unsigned int events); /**< Slot handler function */
	unsigned int max_timeout;	 /**< Maximum time since last poll before executing the timeout handler */
	unsigned int timeout_count;	 /**< Time since last poll */
	int (*timeout_handler) (void *data);     /**< Timeout handler */
	pthread_mutex_t slot_lock;       /**< Thread slot mutex lock */
} thr_thread_slot_t;

typedef struct _THREAD_STAT {
	struct stats epoll_event;     /**< Number of events ready when epoll returns */
	struct stats time_2epoll;     /**< Time between two continuous epoll returns */
	struct stats processing_time; /**< Time to process all ready epoll events */
	int stats_ready;
} thr_thread_stat_t;
/**
 * @brief       Thread data structure, store require information for each thread
 */
typedef struct _THREAD_THREAD_DATA {
	pthread_t id;              /**< ID of the thread */
	int poll_fd;               /**< File descriptor of epoll */
	int thread_capabilities;   /**< Capabilities of this thread. Can combine flags of @thr_capability_t */
	char cpu_core;             /**< CPU core number that thread will run on */
	int priority;              /**< Thread priority */
	char max_slots;            /**< Max number of slots in a thread */
	char exit_flag;            /**< If this flag is set, the thread will try to terminate itself */
	thr_thread_slot_t slots[MAX_THREAD_SLOTS];  /**< Array of data slots */
	int num_slots;             /**< Number of used slots */

	char is_first_poll;        /**< Indicate the first poll loop of thread handle */
	unsigned int last_poll_time;   /**< Time of the last poll */
	thr_thread_stat_t stats;
	thr_thread_stat_t stats_snap;
} thr_thread_t;

/** @} */
#endif /* __THREAD_CONFIG_H__ */
