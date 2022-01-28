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
 @brief AVB media clock timer driver
*/
#ifndef _MTIMER_H_
#define _MTIMER_H_

#include "media_clock.h"

struct mtimer_start {
	unsigned int p; /* in Hz */
	unsigned int q; /* in Hz */
};

#define MTIMER_MAX	4

#define MTIMER_FLAGS_STARTED_EXT		(1 << 0)
#define MTIMER_FLAGS_STARTED_INT		(1 << 1)

struct mtimer_dev {
	mclock_t type;
	unsigned int domain;

	void *mclock_dev;

	struct slist_node list;
	unsigned int flags;
	struct rational wake_up_next;
	struct rational wake_up_per;

	void (*cb)(void *);
	void *cb_data;
};

void mtimer_wake_up_init(struct mtimer_dev *dev, unsigned int now);
void mtimer_wake_up_init_now(struct mtimer_dev *dev, unsigned int now);
void mtimer_wake_up(struct mtimer_dev *dev, unsigned int now);

#ifdef CONFIG_AVTP
int mtimer_start(struct mtimer_dev *dev, struct mtimer_start *start, unsigned int time_now,
		bool internal, bool is_media_clock_running);
int mtimer_stop(struct mtimer_dev *dev, bool internal);
struct mtimer_dev *mtimer_open(mclock_t type, unsigned int domain);
void mtimer_release(struct mtimer_dev *dev);
void mtimer_set_callback(struct mtimer_dev *dev, void (*cb)(void *), void *data);
#else
int mtimer_start(struct mtimer_dev *dev, struct mtimer_start *start, unsigned int time_now,
		bool internal, bool is_media_clock_running)
{
	return -1;
}
int mtimer_stop(struct mtimer_dev *dev, bool internal)
{
	return -1;
}
struct mtimer_dev *mtimer_open(mclock_t type, unsigned int domain)
{
	return NULL;
}
void mtimer_release(struct mtimer_dev *dev)
{
	return;
}
void mtimer_set_callback(struct mtimer_dev *dev, void (*cb)(void *), void *data)
{
	return;
}
#endif /* CONFIG_AVTP */


#endif /* _MTIMER_H_ */

