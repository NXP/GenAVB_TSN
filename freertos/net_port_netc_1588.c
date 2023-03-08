/*
* Copyright 2022-2023 NXP
*
* SPDX-License-Identifier: BSD-3-Clause
*/

/**
 @file
 @brief FreeRTOS specific Network service implementation
 @details
*/

#include "common/log.h"
#include "clock.h"

#include "net_port_netc_1588.h"

#include "config.h"

#if (CFG_NUM_ENETC_EP_MAC || CFG_NUM_NETC_SW)

#include "fsl_netc_timer.h"
#include "hw_clock.h"

static uint16_t netc_1588_ref_count = 0;
static netc_timer_handle_t timer_handle;

uint64_t netc_1588_hwts_to_u64(void *handle, uint32_t hwts_ns)
{
	netc_timer_handle_t *timer_handle = (netc_timer_handle_t *)handle;
	uint64_t hw_time;
	uint64_t nanoseconds = 0;

	NETC_TimerGetCurrentTime(timer_handle, &hw_time);

	nanoseconds = (hw_time & 0xffffffff00000000ULL) | ((uint64_t)hwts_ns);
	if (hwts_ns > (hw_time & 0xffffffff)) // the 32-bit clock counter wrapped between hwts_ns and now
		nanoseconds -= 0x100000000ULL;

	return nanoseconds;
}

uint64_t netc_1588_read_counter(void *priv)
{
	netc_timer_handle_t *timer_handle = (netc_timer_handle_t *)priv;
	uint64_t timestamp;

	NETC_TimerGetCurrentTime(timer_handle, &timestamp);

	return timestamp;
}

int netc_1588_clock_adj_freq(void *priv, int32_t ppb)
{
	netc_timer_handle_t *timer_handle = (netc_timer_handle_t *)priv;

	NETC_TimerAdjustFreq(timer_handle, ppb);

	return 0;
}

void netc_1588_exit(void)
{
	netc_1588_ref_count--;

	if (!netc_1588_ref_count)
		NETC_TimerDeinit(&timer_handle);
}

void *netc_1588_init(void)
{
	netc_timer_config_t timer_config;

	if (!netc_1588_ref_count) {
		memset(&timer_config, 0, sizeof(netc_timer_config_t));
		timer_config.clockSelect = kNETC_TimerExtRefClk;
		timer_config.refClkHz = CLOCK_GetRootClockFreq(kCLOCK_Root_Tmr_1588);
		timer_config.enableTimer = true;

		os_log(LOG_INFO, "refClkHz: %u defaultPpb: %u\n", timer_config.refClkHz, timer_config.defaultPpb);

		if (NETC_TimerInit(&timer_handle, &timer_config) != kStatus_Success)
			return NULL;
	}

	netc_1588_ref_count++;

	return &timer_handle;
}
#else
void *netc_1588_init(void) { return NULL; }
void netc_1588_exit(void) {}
uint64_t netc_1588_hwts_to_u64(void *handle, uint32_t hwts_ns) { return 0; }
uint64_t netc_1588_read_counter(void *priv) {return 0; }
int netc_1588_clock_adj_freq(void *priv, int32_t ppb) { return 0; }
#endif /* CFG_NUM_ENETC_EP_MAC || CFG_NUM_NETC_SWITCHES */
