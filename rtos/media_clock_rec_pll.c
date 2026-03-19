/*
 * Copyright 2018-2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief AVB media clock recovery driver
*/

#include "config.h"

#include "rtos_abstraction_layer.h"

#include "media_clock_rec_pll.h"
#include "net_port.h"
#include "avtp.h"

#include "os/sys_types.h"
#include "hw_timer.h"
#include "gptp_dev.h"
#include "common/log.h"

#include "common/os/media_clock_rec_pll_common.c"

static void mclock_rec_pll_deferred_adjust(struct mclock_rec_pll *rec)
{
	return;
}

static int mclock_rec_pll_gptp_compare_reload(struct mclock_rec_pll *rec, uint32_t next_ts)
{
	return gptp_tc_reload(rec->gptp_event_dev, next_ts);
}

static int mclock_rec_pll_gptp_compare_start(struct mclock_rec_pll *rec, uint32_t ts_0, uint32_t ts_1)
{
	return gptp_event_start(rec->gptp_event_dev, ts_0, ts_1);
}

static void mclock_rec_pll_gptp_stop(struct mclock_rec_pll *rec)
{
	gptp_stop(rec->gptp_event_dev);
}

static int mclock_rec_pll_gptp_read(struct mclock_rec_pll *rec, uint32_t *now)
{
	return os_clock_gettime32(rec->gptp_event_dev->port->clock[PORT_CLOCK_GPTP_0], now);
}

static unsigned int mclock_shared_mem_read(struct mclock_dev *dev, unsigned int idx)
{
	return mclock_mem_read(dev, idx);
}

int mclock_rec_pll_start(struct mclock_rec_pll *rec, struct mclock_start *start)
{
	return mclock_rec_pll_common_start(&rec->c, start, DEFAULT_REC_PI_KI_FACTOR, DEFAULT_REC_PI_KP_FACTOR);
}

int mclock_rec_pll_stop(struct mclock_rec_pll *rec)
{
	mclock_rec_pll_common_stop(&rec->c);

	return 0;
}

void mclock_rec_pll_reset(struct mclock_rec_pll *rec)
{
	struct mclock_rec_pll_common *rec_common = &rec->c;

	mclock_rec_pll_stop(rec);
	mclock_rec_pll_start(rec, NULL);
	rec_common->stats.reset++;
}

__init int mclock_rec_pll_init(struct mclock_rec_pll *rec)
{
	struct mclock_dev *dev = &rec->c.dev;
	int rc = 0;

	mclock_rec_pll_common_init(&rec->c);

	rec->gptp_event_dev = gptp_event_init(GPTP_ENET_DEV_INDEX);
	if (!rec->gptp_event_dev) {
		rc = -1;
		os_log(LOG_ERR, "gptp_event_init failed\n");
		goto exit;
	}

	dev->sh_mem = rtos_malloc(MCLOCK_REC_MMAP_SIZE);
	if (!dev->sh_mem) {
		rc = -1;
		os_log(LOG_ERR, "rtos_malloc failed\n");
		goto err_malloc;
	}

	dev->w_idx = (unsigned int *)((char *)dev->sh_mem + MCLOCK_REC_BUF_SIZE);
	dev->sh_mem_size = MCLOCK_REC_MMAP_SIZE;
	dev->num_ts = MCLOCK_REC_NUM_TS;
	dev->timer_period = NET_RX_TX_PERIOD;

	mclock_register_device(dev);

err_malloc:
	gptp_event_exit(rec->gptp_event_dev);
exit:
	return rc;
}

__exit void mclock_rec_pll_exit(struct mclock_rec_pll *rec)
{
	mclock_unregister_device(&rec->c.dev);

	rtos_free(rec->c.dev.sh_mem);
}
