/*
* Copyright 2018, 2020 NXP
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
 @brief Media clock interface handling
*/
#ifndef _MCLOCK_H_
#define _MCLOCK_H_

#include "genavb/types.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "os/sys_types.h"
#include "os/clock.h"
#include "rational.h"
#include "slist.h"

#include "hw_timer.h"

#define MCLOCK_TIMER_MAX 	4

#define MCLOCK_FLAGS_FREE		(1 << 0)
#define MCLOCK_FLAGS_RUNNING		(1 << 1)
#define MCLOCK_FLAGS_REGISTERED		(1 << 2)
#define MCLOCK_FLAGS_WAKE_UP		(1 << 3)
#define MCLOCK_FLAGS_INIT		(1 << 4)
#define MCLOCK_FLAGS_RUNNING_LOCKED	(1 << 5)

#define MCLOCK_TIMER_ACTIVE 		(1 << 0)

#define MCLOCK_DRIFT_PPM_MAX		250

/* Media clock types */
typedef enum {
	REC, /* Recovery, associated to a HW audio clock domain  */
	GEN, /* Generation, associated to a HW audio clock domain */
	PTP  /* Virtual, not associated to a HW audio clock domain */
} mclock_t;

typedef enum {
	TS_EXTERNAL,		/* Timestamps are provided by user-space */
	TS_INTERNAL 		/* Timestamps are generated locally, in sync with PTP */
} mclock_ts_src_t;

typedef enum  {
	MCLOCK_CFG_FREQ,
	MCLOCK_CFG_TS_SRC,
	MCLOCK_CFG_WAKE_FREQ,
} mclock_sconfig_cmd;

typedef enum  {
	MCLOCK_STOPPED = -1,
	MCLOCK_RUNNING = 0,
	MCLOCK_RUNNING_LOCKED = 1
} mclock_status;

struct mclock_sconfig {
	mclock_sconfig_cmd cmd;
	union {
		mclock_ts_src_t ts_src;
		struct {
			unsigned int p; /* in Hz */
			unsigned int q; /* in Hz */
		} wake_freq;
		struct {
			unsigned int p;
			unsigned int q;
		} ts_freq;
	};
};

struct mclock_gconfig {
	mclock_ts_src_t ts_src;
	unsigned int ts_freq_p;
	unsigned int ts_freq_q;
	void *array_addr;
	unsigned int array_size; /* in u32 timestamps */
	unsigned int timer_period; /* ns */
};

struct mclock_start {
	uint32_t ts_0;
	uint32_t ts_1;
};

struct mclock_clean {
	unsigned int nb_clean;
	mclock_status status;
};

struct mclock_timer {
	unsigned int flags;
	struct mclock_dev *dev;
	int (*irq_func)(struct mclock_dev *, void *, unsigned int);
	unsigned int period;
	unsigned int count;
	unsigned int ticks;
};

struct mclock_timer_event {
	struct mclock_timer *timer;
};

struct mclock_dev {
	mclock_t type;
	mclock_ts_src_t ts_src;
	struct slist_node list_node;
	struct slist_head mtimer_devices;
	unsigned int flags;
	int domain; //the audio domain ID
	struct net_port *eth;
	int id; //ISR id
	struct mclock_drv *drv;

	void *sh_mem;
	unsigned int sh_mem_size;
	unsigned int mmap_size;
	unsigned int num_ts;
	unsigned int *w_idx;
	unsigned int *ptp;
	unsigned int *count;

	int (*start)(struct mclock_dev *, struct mclock_start *);
	int (*stop)(struct mclock_dev *);
	int (*clean)(struct mclock_dev *, struct mclock_clean *);
	int (*reset)(struct mclock_dev *);
	int (*config)(struct mclock_dev *, struct mclock_sconfig *);
	int (*open)(struct mclock_dev *, int);
	void (*release)(struct mclock_dev *);

	unsigned int ts_freq_p;
	unsigned int ts_freq_q;
	struct rational ts_period;
	unsigned int clk_timer;
	unsigned int timer_period;
	unsigned int next_drift;
	unsigned int drift_period;
};

#define MCLOCK_TASK_NAME		"MCR"
#define MCLOCK_TASK_STACK_DEPTH 	(256)
#define MCLOCK_TASK_PRIORITY		(configMAX_PRIORITIES - 6)

#define MCLOCK_EVENT_QUEUE_LENGTH	16

struct mclock_ctx {
	TaskHandle_t task_handle;
	StaticQueue_t queue;
	QueueHandle_t queue_handle;
	uint8_t queue_buffer[MCLOCK_EVENT_QUEUE_LENGTH * sizeof(struct hw_avb_timer_event)];
};

#ifdef CONFIG_AVTP
int  mclock_init(void);
void mclock_exit(void);
#else
static inline int mclock_init(void)
{
	return 0;
}
static inline void mclock_exit(void)
{
	return;
}
#endif

void mclock_set_ts_freq(struct mclock_dev *dev, unsigned int ts_freq_p, unsigned int ts_freq_q);
int  mclock_drift_adapt(struct mclock_dev *dev, unsigned int clk_media);

void mclock_wake_up_init(struct mclock_dev *dev, unsigned int now_ns);
void mclock_wake_up_init_now(struct mclock_dev *dev, unsigned int now_ns);
int  mclock_wake_up_configure(struct mclock_dev *dev, unsigned int wake_freq_p, unsigned int wake_freq_q);
void  mclock_wake_up(struct mclock_dev *dev, unsigned int elapsed_ns);

struct mtimer_dev;
struct mtimer_start;

#ifdef CONFIG_AVTP
void mclock_interrupt(unsigned int hw_ticks, BaseType_t *wake);
int mclock_timer_start(struct mtimer_dev *mtimer_dev, struct mtimer_start *start);
void mclock_timer_stop(struct mtimer_dev *mtimer_dev);
#else
static inline void mclock_interrupt(unsigned int hw_ticks, BaseType_t *wake)
{
	return;
}
int mclock_timer_start(struct mtimer_dev *mtimer_dev, struct mtimer_start *start)
{
	return -1;
}
void mclock_timer_stop(struct mtimer_dev *mtimer_dev)
{
	return;
}
#endif


int  mclock_register_timer(struct mclock_dev *dev, int (*irq_func)(struct mclock_dev *, void *, unsigned int), unsigned int period);
void mclock_unregister_timer(struct mclock_dev *dev);
void mclock_register_device(struct mclock_dev *dev);
int  mclock_unregister_device(struct mclock_dev *dev);

/*
 * Based on a single timer period adjustment at a time,
 * returns the period for drift adaptation.
 */
static inline u32 mclock_drift_period(struct mclock_dev *dev)
{
	return ((dev->timer_period / MCLOCK_DRIFT_PPM_MAX) * 1000000);
}

static inline unsigned int mclock_mem_read(struct mclock_dev *dev, unsigned int idx)
{
	return *(unsigned int *)((char *) dev->sh_mem + ((idx) & (dev->num_ts - 1)) * sizeof(unsigned int));
}

static inline void mclock_mem_write(struct mclock_dev *dev, unsigned int idx, unsigned int value)
{
	*(unsigned int *)((char *) dev->sh_mem + ((idx) & (dev->num_ts - 1)) * sizeof(unsigned int)) = value;
}

#endif /* _MCLOCK_H_ */

