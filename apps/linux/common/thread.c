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
 @file thread.c
 @brief      This file implements interfaces of thread management.
 @details    Copyright 2016 Freescale Semiconductor, Inc.
*/

#define _GNU_SOURCE             /* See feature_test_macros(7) */
#include <sched.h>
#include <errno.h>
#include <stdint.h>
#include <inttypes.h>
#include <sys/epoll.h>
#include <time.h>
#include <unistd.h>
#include "log.h"
#include "thread.h"
#include "time.h"

#define THREAD_STATS_PERIOD_NS	  (10llu * NSECS_PER_SEC)
#define THREAD_HANDLER_TIMEOUT_NS (100llu * NSECS_PER_MSEC)

static pthread_mutex_t thread_mutex = PTHREAD_MUTEX_INITIALIZER;

/** Thread handle function.
 *
 * After thread starts, this function will check the registered file descriptor for incoming data.
 * If any data came, it will read from input then transfer to output. This function only exit
 * whenever error occurs or exit_flag of thread is set.
 *
 * @param[in]  param  Index of thread
 *
 * @return     handler result
 */
static void *s_data_thread_handle(void *param);

/** Count number of used slot in thread
 *
 * @param[in]  thread  Pointer of thread that want to check
 *
 * @return     number of used slot
 */
static int thread_count_used_slot(thr_thread_t *thread);

/** Check thread matches specified capabilities.
 *
 * @param[in]  thread        Pointer to thread that need to check
 * @param[in]  capabilities  Required capabilities
 *
 * @return     0 if the thread does not match required capabilities
 * @return     1 if the thread matches required capabilities
 */
static int thread_check_capability_match(thr_thread_t *thread, int capabilities);

/** Get first idle slot of thread
 *
 * @param[in]  thread  Pointer to thread
 *
 * @return     pointer to idle slot, NULL if do not have any idle slot.
 */
static thr_thread_slot_t *thread_get_first_idle_slot(thr_thread_t *thread);

/** Check for timed out slots and execute its handlers
 *
 * @param[in]  thread  Pointer to thread
 * @param[in]  timeout_time The timeout value in ns
 *
 * @return     pointer to idle slot, NULL if do not have any idle slot.
 */
static void thread_check_timeout_slots(thr_thread_t *thread, unsigned timeout_time);

/**
 * @brief      Get the thread that contains given slot
 *
 * @param      slot  The pointer of slot
 *
 * @return     Pointer to thread that contains the slot, else NULL
 */
static thr_thread_t *thread_get_from_slot(thr_thread_slot_t *slot);

/**
 * @brief      Initialize data of thread slot
 *
 * @param      thread  The thread pointer
 */
static void thread_slot_init_data(thr_thread_t *thread);

/**
 * @brief      De-initialize data of thread slot
 *
 * @param      thread  The thread pointer
 */
static void thread_slot_deinit_data(thr_thread_t *thread);

int thread_init(void)
{
	int count;
	int result;

	for (count = 0; count < MAX_THREADS; ++count) {
		thr_thread_t *thread_ptr = &g_thread_array[count];

		// Create epoll for the thread
		result = epoll_create1(0);
		if (result < 0) {
			ERR("Create epoll handle failed. Error %s", strerror(errno));
			return -1;
		}
		thread_ptr->poll_fd = result;

		DBG("epoll %d fd %d created", count, thread_ptr->poll_fd);

		// Init statistics
		stats_init(&thread_ptr->stats.epoll_event, 31, thread_ptr, NULL);
		stats_init(&thread_ptr->stats.time_2epoll, 31, thread_ptr, NULL);
		stats_init(&thread_ptr->stats.processing_time, 31, thread_ptr, NULL);

		thread_ptr->is_first_poll = 1;
		thread_ptr->last_poll_time = 0;
		thread_ptr->exit_flag = 0;
		thread_ptr->num_slots = 0;

		// Slot data initialization
		thread_slot_init_data(thread_ptr);

		result = pthread_create(&thread_ptr->id, NULL, &s_data_thread_handle, (void *)(intptr_t) count);
		if (result != 0) {
			ERR("thread %d create failed, error %s", count, strerror(result));
			return -1;
		}
		DBG("Thread %d created with id %d", count, (int)thread_ptr->id);

	}

	// Initialize mutex
	pthread_mutex_init(&thread_mutex, NULL);

	return 0;
}

int thread_exit(void)
{
	int count;
	int result;

	for (count = 0; count < MAX_THREADS; count++) {
		thr_thread_t *thread_ptr = &g_thread_array[count];

		// Request thread quit
		thread_ptr->exit_flag = 1;

		// join thread
		pthread_join(thread_ptr->id, (void *)&result);
		DBG("thread %d id %d joint with result %d", count, (int)thread_ptr->id, result);

		// Close epoll
		result = close(thread_ptr->poll_fd);
		if (result != 0) {
			ERR("epoll %d failed to close, error %s", thread_ptr->poll_fd, strerror(errno));
		} else {
			DBG("epoll %d closed", thread_ptr->poll_fd);
		}

		// De-init slot data
		thread_slot_deinit_data(thread_ptr);
	}
	pthread_mutex_destroy(&thread_mutex);

	return 0;
}

static void __thread_print_stats(thr_thread_t *thread_ptr)
{
	thr_thread_stat_t *stats = &thread_ptr->stats_snap;

	if (stats->stats_ready) {
		INF_LOG("Thread (%p) epoll count: %d, epoll number: %d/%d/%d, epoll time: %6d/%7d/%7d processing: %6d/%7d/%7d",
			thread_ptr, stats->epoll_event.current_count, stats->epoll_event.min,
			stats->epoll_event.mean, stats->epoll_event.max, stats->time_2epoll.min,
			stats->time_2epoll.mean, stats->time_2epoll.max, stats->processing_time.min,
			stats->processing_time.mean, stats->processing_time.max);
		stats->stats_ready = 0;
	}
}

void thread_print_stats()
{
	int i;

	for (i = 0; i < MAX_THREADS; i++) {
		thr_thread_t *thread_ptr = &g_thread_array[i];

		if (thread_ptr->num_slots > 0)
			__thread_print_stats(thread_ptr);
	}
}

static void thread_dump_stats(thr_thread_t *thread_ptr)
{
	thr_thread_stat_t *stats = &thread_ptr->stats;
	thr_thread_stat_t *stats_snap = &thread_ptr->stats_snap;

	// Thread global statistics
	if (thread_ptr->num_slots > 0) {
		stats_compute(&stats->epoll_event);
		stats_compute(&stats->time_2epoll);
		stats_compute(&stats->processing_time);
		memcpy(stats_snap, stats, sizeof(thr_thread_stat_t));
		stats_reset(&stats->time_2epoll);
		stats_reset(&stats->epoll_event);
		stats_reset(&stats->processing_time);
		stats_snap->stats_ready = 1;
	}
}

static void *s_data_thread_handle(void *param)
{
	intptr_t final_result = 0;
	cpu_set_t cpu_set;
	int thread_index = (int)(intptr_t)param;
	thr_thread_t *thread_ptr = &g_thread_array[thread_index];
	struct epoll_event recv_events[MAX_THREAD_SLOTS];
	struct sched_param thread_param = {
		.sched_priority = thread_ptr->priority,
	};
	unsigned int poll_time;
	struct timespec ts;
	unsigned long long tlast_timeout = 0, now = 0, tlast_stat = 0;

	DBG("thread %d, cpu %d, poll_fd: %d, max_slots: %d", thread_index, thread_ptr->cpu_core, thread_ptr->poll_fd, thread_ptr->max_slots);
	// If the max_slots is 0, means the thread is not be used. Just quit.
	if (0 >= thread_ptr->max_slots) {
		INF("The thread %d on cpu %d will not be used because max_slots = 0", thread_index, thread_ptr->cpu_core);
		goto lb_exit;
	}

	// Make thread more real time
	if (sched_setscheduler(0, SCHED_FIFO, &thread_param) < 0) {
		ERR("%d - sched_setscheduler failed with error %d - %s", thread_index, errno, strerror(errno));
		final_result = -1;
		goto lb_exit;
	}

	// Specify which core number that the thread will run on
	CPU_ZERO(&cpu_set);
	CPU_SET(thread_ptr->cpu_core, &cpu_set);
	if (sched_setaffinity(0, sizeof(cpu_set), &cpu_set) == -1) {
		ERR("%d - sched_setaffinity failed with error %d - %s", thread_index, errno, strerror(errno));
//		final_result = -1;
//		goto lb_exit;
	}

	// Loop until exit flag of the thread is set
	while (!thread_ptr->exit_flag) {
		int n, i, res;

		// Wait fd events for each 100ms
		n = epoll_wait(thread_ptr->poll_fd, recv_events, thread_ptr->max_slots, 100);
		if (n < 0) {
			if (errno == EINTR)
				continue;
			else {
				// Error occurs
				ERR("%d - epoll_wait(%d) failed: %s", thread_index, thread_ptr->poll_fd, strerror(errno));
				final_result = -1;
				goto lb_exit;
			}
		}

		res = clock_gettime(CLOCK_MONOTONIC, &ts);

		if (res == 0) {
			now = (unsigned long long) ts.tv_sec * NSECS_PER_SEC + ts.tv_nsec;

			if ((now - tlast_stat) >= THREAD_STATS_PERIOD_NS) {
				// Dump statistic each 10s
				thread_dump_stats(thread_ptr);
				tlast_stat = now;
			}

			if (!tlast_timeout)
				tlast_timeout = now;

			if (now - tlast_timeout >= THREAD_HANDLER_TIMEOUT_NS) {
				/*Check if we have timeout handlers to execute, every 100 ms*/
				thread_check_timeout_slots(thread_ptr, now - tlast_timeout);
				tlast_timeout = now;
			}
		} else {
			ERR("clock_gettime() failed: %s", strerror(errno));
		}

		if (n == 0) {
			stats_update(&thread_ptr->stats.epoll_event, n);
			continue;
		}

		// Update stats
		stats_update(&thread_ptr->stats.epoll_event, n);
		if (0 != res) {
			ERR("could not get clock real-time, error: %s", strerror(errno));
		} else {
			poll_time = (unsigned long long)ts.tv_sec*NSECS_PER_SEC + ts.tv_nsec;
			if (!thread_ptr->is_first_poll) {
				stats_update(&thread_ptr->stats.time_2epoll, poll_time - thread_ptr->last_poll_time);
				// DBG("%p time_2epoll: %d - %d - %d", thread_ptr, thread_ptr->time_2epoll.current_count, poll_time - thread_ptr->last_poll_time, thread_ptr->time_2epoll.min);
				// DBG("poll: %d, %d", n, poll_time - last_poll_time);
			} else {
				thread_ptr->is_first_poll = 0;
			}
			thread_ptr->last_poll_time = poll_time;
		}

		// Events received, process them
		for (i = 0; i < n; ++i) {
			int events = recv_events[i].events;
			int fd = recv_events[i].data.fd;
			thr_thread_slot_t *slot = (thr_thread_slot_t *)recv_events[i].data.ptr;

			if (events & EPOLLERR) {
				ERR("%d - fd %d with poll error", thread_index, fd);
				continue;
			}

			if (!slot)
				continue;

			/* Reset timeout counter*/
			if (events & (EPOLLIN | EPOLLOUT))
				slot->timeout_count = 0;

			slot->handler(slot->data, events);
		}

		res = clock_gettime(CLOCK_MONOTONIC, &ts);
		if (res == 0) {
			poll_time = (unsigned long long)ts.tv_sec * NSECS_PER_SEC + ts.tv_nsec;
			stats_update(&thread_ptr->stats.processing_time, poll_time - thread_ptr->last_poll_time);
		} else {
			ERR("clock get real-time failed");
		}

		sched_yield();
	}

lb_exit:
	return (void *)final_result;
}

static int thread_count_used_slot(thr_thread_t *thread)
{
	int result = 0;
	int i;

	for (i = 0; i < thread->max_slots; ++i) {
		if (thread->slots[i].is_used != 0) {
			result ++;
		}
	}

	return result;
}

static int thread_check_capability_match(thr_thread_t *thread, int capabilities)
{
	int tmp = thread->thread_capabilities;

	// if not match capacity, return 0
	if ((tmp & capabilities) != capabilities) {
		return 0;
	}

	return 1;
}

static thr_thread_slot_t *thread_get_first_idle_slot(thr_thread_t *thread)
{
	int i;

	for (i = 0; i < thread->max_slots; ++i) {
		if (!thread->slots[i].is_used) {
			return &thread->slots[i];
		}
	}

	return NULL;
}

static void thread_check_timeout_slots(thr_thread_t *thread, unsigned int timeout_time)
{
	int i;

	for (i = 0; i < thread->max_slots; ++i) {
		if (thread->slots[i].is_used && thread->slots[i].timeout_handler) {
			thread->slots[i].timeout_count += timeout_time;
			if(thread->slots[i].timeout_count > thread->slots[i].max_timeout)
				thread->slots[i].timeout_handler(thread->slots[i].data);
		}
	}

}

int thread_slot_add(int capabilities, int fd, unsigned int events, void *data,
		    int (*handler)(void *data, unsigned int events),
		    int (*timeout_handler)(void *data),
		    int max_timeout, thr_thread_slot_t **slot)
{
	thr_thread_t *thread_ptr = NULL;
	thr_thread_slot_t *thread_slot_ptr = NULL;
	int i;
	int tmp;
	struct epoll_event event;

	// Lock data
	pthread_mutex_lock(&thread_mutex);

	// Find best idle slot
	for (i = 0; i < MAX_THREADS; ++i) {
		thr_thread_t *cur_thread = &g_thread_array[i];

		if (!thread_check_capability_match(cur_thread, capabilities)) {
			// Thread does not match capabilities
			continue;
		}

		tmp = thread_count_used_slot(cur_thread);
		if (tmp < cur_thread->max_slots) {
			// This thread has idle slot
			thread_ptr = cur_thread;
			INF("Using thread number %d", i);
			break;
		}
	}

	DBG("thread_ptr (%p)", thread_ptr);
	if (!thread_ptr) {
		// No thread found
		ERR("No thread found");
		pthread_mutex_unlock(&thread_mutex);
		return -1;
	}

	DBG("CPU core: %d", thread_ptr->cpu_core);
	thread_slot_ptr = thread_get_first_idle_slot(thread_ptr);
	DBG("thread_slot_ptr (%p)", thread_slot_ptr);
	if (!thread_slot_ptr) {
		// No slot found
		ERR("No thread slot found");
		pthread_mutex_unlock(&thread_mutex);
		return -1;
	}

	DBG("Update slot (%p) info, fd: %d, data: %p, handler: %p", thread_slot_ptr, fd, data, handler);
	// Update slot information
	thread_slot_ptr->is_used = 1;
	thread_slot_ptr->fd = fd;
	thread_slot_ptr->data = data;
	thread_slot_ptr->handler = handler;
	thread_slot_ptr->timeout_count = 0;
	thread_slot_ptr->max_timeout = max_timeout;
	thread_slot_ptr->timeout_handler = timeout_handler;
	// Unlock the slot lock for first time
	pthread_mutex_unlock(&thread_slot_ptr->slot_lock);

	// Update output value
	DBG("slot (%p) -> %p", slot, thread_slot_ptr);
	thread_ptr->num_slots++;
	*slot = thread_slot_ptr;

	// add slot into epoll monitor
	event.data.fd = fd;
	event.data.ptr = thread_slot_ptr;
	event.events = (events & (EPOLLIN | EPOLLOUT)) | EPOLLERR | EPOLLHUP;

	DBG("Add fd %d into epoll %d", fd, thread_ptr->poll_fd);
	if (-1 == epoll_ctl(thread_ptr->poll_fd, EPOLL_CTL_ADD, fd, &event)) {
		// epoll add fd error. Clear is_used flag and return NULL
		ERR("epoll_ctl failed, %s", strerror(errno));
		thread_slot_ptr->is_used = 0;
		thread_ptr->num_slots --;
		*slot = NULL;
		pthread_mutex_unlock(&thread_mutex);
		return -1;
	}

	thread_slot_ptr->is_enabled = 1;
	INF("Add fd %d into epoll of thread %p", fd, thread_ptr);

	pthread_mutex_unlock(&thread_mutex);

	return 0;
}

int thread_slot_set_events(thr_thread_slot_t *slot, int enable, unsigned int req_events)
{
	struct epoll_event event;
	thr_thread_t *thread;

	// Lock data
	pthread_mutex_lock(&thread_mutex);

	thread = thread_get_from_slot(slot);
	if (!thread) {
		ERR("Thread not found for slot %p", slot);
		pthread_mutex_unlock(&thread_mutex);
		return -1;
	}
	
	if(enable) {
		
		if (slot->is_enabled) {
			pthread_mutex_unlock(&thread_mutex);
			return 0;
		}
		
		// add slot into epoll monitor
		
		event.data.fd = slot->fd;
		event.data.ptr = slot;
		event.events = (req_events & (EPOLLIN | EPOLLOUT)) | EPOLLERR | EPOLLHUP;
	
		DBG("Add fd %d into epoll %d", slot->fd, thread->poll_fd);
		if (-1 == epoll_ctl(thread->poll_fd, EPOLL_CTL_ADD, slot->fd, &event)) {
			// epoll add fd error. Clear is_used flag and return NULL
			ERR("epoll_ctl failed, %s", strerror(errno));
			pthread_mutex_unlock(&thread_mutex);
			return -1;
		}
	
		slot->is_enabled = 1;		
	} else {

		if (!slot->is_enabled) {
			pthread_mutex_unlock(&thread_mutex);
			return 0;
		}
		// remove slot from epoll monitor
		DBG("remove fd %d from epoll", slot->fd);
		if (-1 == epoll_ctl (thread->poll_fd, EPOLL_CTL_DEL, slot->fd, NULL)) {
			// epoll delete fd error.
			ERR("remove the fd %d failed", slot->fd);
			pthread_mutex_unlock(&thread_mutex);
			return -1;
		}
		slot->is_enabled = 0;
	}

	pthread_mutex_unlock(&thread_mutex);
	return 0;
}

int thread_slot_free(thr_thread_slot_t *slot)
{
	thr_thread_t *thread;
	// Lock data
	pthread_mutex_lock(&thread_mutex);

	thread = thread_get_from_slot(slot);
	if (!thread) {
		ERR("Thread not found for slot %p", slot);
		pthread_mutex_unlock(&thread_mutex);
		return -1;
	}
	// remove slot from epoll monitor
	if (!slot->is_enabled)
		goto deinit;
	DBG("remove fd %d from epoll", slot->fd);
	if (-1 == epoll_ctl (thread->poll_fd, EPOLL_CTL_DEL, slot->fd, NULL)) {
		// epoll delete fd error.
		ERR("remove the fd %d failed", slot->fd);
		pthread_mutex_unlock(&thread_mutex);
		return -1;
	}
deinit:
	// epoll fd is removed
	slot->is_used = 0;
	thread->num_slots --;
	slot->is_enabled = 0;
	slot->timeout_count = 0;
	slot->max_timeout = 0;
	slot->timeout_handler = NULL;

	pthread_mutex_unlock(&thread_mutex);

	return 0;
}

static thr_thread_t *thread_get_from_slot(thr_thread_slot_t *slot)
{
	int i;
	int j;

	for (i = 0; i < MAX_THREADS; ++i) {
		thr_thread_t *cur_thread = &g_thread_array[i];
		for (j = 0; j < cur_thread->max_slots; ++j) {
			thr_thread_slot_t *cur_slot = &cur_thread->slots[j];

			if (!cur_slot->is_used) {
				continue;
			}

			if (cur_slot == slot) {
				return cur_thread;
			}
		}
	}

	return NULL;
}

static inline void thread_slot_reset_data(thr_thread_slot_t *slot)
{
	slot->is_used = 0;
	slot->fd = 0;
	slot->data = NULL;
	slot->handler = NULL;
	slot->timeout_count = 0;
	slot->max_timeout = 0;
	slot->timeout_handler = NULL;
}

static void thread_slot_init_data(thr_thread_t *thread)
{
	int i;

	for (i = 0; i < MAX_THREAD_SLOTS; ++i) {
		thr_thread_slot_t *slot = &thread->slots[i];

		// Reset data of slot
		thread_slot_reset_data(slot);
		// Initialize mutex lock
		pthread_mutex_init(&slot->slot_lock, NULL);
	}
}

static void thread_slot_deinit_data(thr_thread_t *thread)
{
	int i;

	for (i = 0; i < MAX_THREAD_SLOTS; ++i) {
		thr_thread_slot_t *slot = &thread->slots[i];

		// Reset data of slot
		thread_slot_reset_data(slot);
		// Destroy mutex lock
		pthread_mutex_destroy(&slot->slot_lock);
	}
}
