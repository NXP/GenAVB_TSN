/*
* Copyright 2019-2020 NXP
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
 @brief Management linux specific code
 @details Setups linux thread for Management stack component. Implements Management main loop and event handling.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <sys/epoll.h>

#include "common/log.h"
#include "common/net.h"
#include "common/timer.h"
#include "common/ipc.h"

#include "linux/fgptp.h"

#include "management/management_entry.h"

#define EPOLL_MAX_EVENTS	8

/* Linux specific Management code entry point */

static void management_thread_cleanup(void *arg)
{
	struct management_ctx *management = arg;

	management_exit(management);

	os_log(LOG_INIT, "done\n");
}

static void management_status(struct fgptp_ctx *fgptp, int status)
{
	pthread_mutex_lock(&fgptp->status_mutex);

	fgptp->management_status = status;

	pthread_cond_signal(&fgptp->management_cond);

	pthread_mutex_unlock(&fgptp->status_mutex);
}

void *management_thread_main(void *arg)
{
	struct fgptp_ctx *fgptp = arg;
	struct management_ctx *management;
	int epoll_fd;
	struct epoll_event event[EPOLL_MAX_EVENTS];
	struct sched_param param = {
		.sched_priority = MANAGEMENT_CFG_PRIORITY,
	};
	int rc;

	rc = pthread_setschedparam(pthread_self(), SCHED_FIFO, &param);
	if (rc) {
		os_log(LOG_ERR, "pthread_setschedparam(), %s\n", strerror(rc));
		goto err_setschedparam;
	}

	epoll_fd = epoll_create(1);
	if (epoll_fd < 0) {
		os_log(LOG_CRIT, "epoll_create(), %s\n", strerror(errno));
		goto err_epoll_create;
	}

	management = management_init(&fgptp->management_cfg, epoll_fd);
	if (!management)
		goto err_management_init;

	pthread_cleanup_push(management_thread_cleanup, management);

	os_log(LOG_INIT, "started\n");

	management_status(fgptp, 1);

	while (1) {
		int ready, i;
		struct linux_epoll_data *epoll_data;

		/* thread main loop */
		/* use epoll to wait for events from all open file descriptors */

		pthread_testcancel();

		ready = epoll_wait(epoll_fd, event, EPOLL_MAX_EVENTS, -1);
		if (ready < 0) {
			if (errno == EINTR)
				continue;

			os_log(LOG_ERR, "epoll_wait(), %s\n", strerror(errno));
			break;
		}

		for (i = 0; i < ready; i++) {
			if (event[i].events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP))
				os_log(LOG_ERR, "event error, %x\n", event[i].events);

			if (event[i].events & EPOLLIN) {
				epoll_data = (struct linux_epoll_data *)event[i].data.ptr;

				switch (epoll_data->type) {
				case EPOLL_TYPE_TIMER:
					os_timer_process((struct os_timer *)epoll_data->ptr);
					break;

				case EPOLL_TYPE_IPC:
					ipc_rx((struct ipc_rx *)epoll_data->ptr);
					break;

				default:
					break;
				}
			}
		}
	}

	pthread_cleanup_pop(1);

	close(epoll_fd);

	return (void *)0;

err_management_init:
	close(epoll_fd);

err_epoll_create:
err_setschedparam:
	management_status(fgptp, -1);

	return (void *)-1;
}
