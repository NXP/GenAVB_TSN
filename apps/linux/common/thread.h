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
 @file thread.h
 @brief      This file defines interfaces of thread management.
 @details    Copyright 2016 Freescale Semiconductor, Inc.
*/

#ifndef __THREAD_MANAGEMENT_H__
#define __THREAD_MANAGEMENT_H__

#include "thread_config.h"

extern thr_thread_t g_thread_array[MAX_THREADS];

/**
 * @addtogroup thread
 * @{
 */

/** Create all routing required threads.
 *
 * @details    This function will create four threads (equals to number of CPU cores) to routing
 *             data between AVB stream and ALSA device. Each thread executes on a specified CPU
 *             core.
 *
 * @return     0 if success or negative error code
 */
int thread_init();

/** Terminate all routing threads, free resources.
 *
 * @details    This function will terminate all threads are created by @ref thread_init function.
 *
 * @return     0 if success or negative error code
 */
int thread_exit();

/** Add a route slot into thread.
 *
 * @details    This function finds a thread which has free slot and thread capabilities match
 *             parameter then add this new slot with its information.
 *
 * @param[in]  capabilities    Required capabilities of the slot
 * @param[in]  fd              Input file descriptor of input source
 * @param[in]  data            Private data of the slot, this data will be passed to handler
 *                             function whenever this function is called.
 * @param[in]  handler         Handler function, will be called to process data whenever fd has
 *                             input data.
 * @param[in]  timeout_handler Handler function will be executed in case epoll timed out
 * @param[in]  max_timeout     Max timeout (ns) since last poll before timeout handler is executed
 * @param[out] slot            When thread slot is successful added, this will contains created
 *                             thread slot information
 *
 * @return     0 if success or negative error code
 */
int thread_slot_add(int capabilities, int fd, unsigned int events, void *data,
		    int (*handler)(void *data, unsigned int events),
		    int (*timeout_handler)(void *data),
		    int max_timeout, thr_thread_slot_t **slot);

/** Free a thread slot and its resources.
 *
 * @details    This function will remove the slot fd from monitor fds.
 *
 * @param[in]  slot  Pointer to slot that you want to remove.
 *
 * @return     0 if success or negative error code
 */
int thread_slot_free(thr_thread_slot_t *slot);

/** Modify the fd monitor of a thread slot.
 *
 * @details    	This function will enable or disable the requested events on the monitored fd
 * 		of the slot.
 *
 * @param[in]  slot  Pointer to slot that you want to remove.
 * @param[in]  enable  flag to enable or disable.
 * @param[in]  req_events  The events to enbale/disable: EPOLLIN and/or EPOLLOUT
 *
 * @return     0 if success or negative error code
 */
int thread_slot_set_events(thr_thread_slot_t *slot, int enable, unsigned int req_events);

/** Print global thread scheduling statistics
 *
 * @details    This function prints all threads statistics when available.
 *	       Should be polled regularly.
 *
 * @return     None
 */
void thread_print_stats();

/**
 * @}
 */

#endif /* __THREAD_MANAGEMENT_H__ */
