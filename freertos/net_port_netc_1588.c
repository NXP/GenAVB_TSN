/*
* Copyright 2022-2023 NXP
*
* SPDX-License-Identifier: BSD-3-Clause
*/

/**
 @file net_port_netc_1588.c
 @brief FreeRTOS specific Timer service implementation
 @details
*/

#include "common/types.h"
#include "common/log.h"
#include "clock.h"

#include "net_port_netc_1588.h"
#include "msgintr.h"

#include "config.h"

#if CFG_NUM_NETC_HW_CLOCK > 0

#include "fsl_netc_timer.h"
#include "hw_clock.h"
#include "hw_timer.h"

#define TIMER_MSG_DATA 0U
#define NETC_CLOCK_MAX_TIMER 2

struct netc_1588_timer {
	int id;	/* Timer source like Alarm, Fiper */
	netc_timer_irq_flags_t irq_flag;
	struct hw_timer hw_timer;
};

struct msgintr {
	MSGINTR_Type *base;
	uint8_t channel;
};

/* Global context for 1588_timer */
struct netc_hw_clock {
	unsigned int id;
	bool owner;
	struct hw_clock clock;
	netc_timer_handle_t timer_handle;
	uint8_t timer_n; /* Store the timers in use for a specific clock */
	struct netc_1588_timer timer[NETC_CLOCK_MAX_TIMER];
	struct msgintr msgintr;
};

static struct netc_hw_clock hw_clock[CFG_NUM_NETC_HW_CLOCK] =
{
	[0] = {
		.id = BOARD_NETC_HW_CLOCK_0_ID,
		.owner = BOARD_NETC_HW_CLOCK_0_OWNER,

		.clock = {
			.rate = NSECS_PER_SEC,
			.period = UINT64_MAX,
			.to_ns = {
				.shift = 0,
			},
			.to_cyc = {
				.shift = 0,
			},
		},

		#ifdef BOARD_HW_CLOCK0_NUM_TIMERS

		.timer_n = BOARD_HW_CLOCK0_NUM_TIMERS,

		.timer = {
		#if BOARD_HW_CLOCK0_NUM_TIMERS > 0
			[0] = {
				.id = BOARD_HW_CLOCK0_TIMER0_ID,
			},
		#endif

		#if BOARD_HW_CLOCK0_NUM_TIMERS > 1
			[1] = {
				.id = BOARD_HW_CLOCK0_TIMER1_ID,
			},
		#endif

		#if BOARD_HW_CLOCK0_NUM_TIMERS > 2 || BOARD_HW_CLOCK0_NUM_TIMERS < 0
		#error `BOARD_HW_CLOCK0_NUM_TIMERS` value is not supported
		#endif

		},
		#endif

		#ifdef BOARD_NETC_HW_CLOCK_0_MSGINTR_BASE
		.msgintr = {
			.base = BOARD_NETC_HW_CLOCK_0_MSGINTR_BASE,
			.channel = BOARD_NETC_HW_CLOCK_0_MSGINTR_CH,
		}
		#endif
	}
#if CFG_NUM_NETC_HW_CLOCK > 1
#error
#endif
};

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

static uint64_t netc_1588_read_counter(void *priv)
{
	netc_timer_handle_t *timer_handle = (netc_timer_handle_t *)priv;
	uint64_t timestamp;

	NETC_TimerGetCurrentTime(timer_handle, &timestamp);

	return timestamp;
}

static int netc_1588_clock_adj_freq(void *priv, int32_t ppb)
{
	netc_timer_handle_t *timer_handle = (netc_timer_handle_t *)priv;

	NETC_TimerAdjustFreq(timer_handle, ppb);

	return 0;
}

static void netc_1588_timer_start_alarm(void *handle, uint8_t alarmId, uint64_t nanosecond)
{
	NETC_TimerStartAlarm(handle, alarmId, nanosecond);
}

static void netc_1588_timer_stop_alarm(void *handle, uint8_t alarmId)
{
	NETC_TimerStopAlarm(handle, alarmId);
}

static int netc_1588_timer_set_next_event(struct hw_timer *hw_timer, uint64_t cycles)
{
	struct netc_1588_timer *netc_timer = container_of(hw_timer, struct netc_1588_timer, hw_timer);
	struct netc_hw_clock *hw_clock = container_of(netc_timer, struct netc_hw_clock, timer[netc_timer->id]);

	netc_1588_timer_start_alarm(&hw_clock->timer_handle, netc_timer->id, cycles);

	return 0;
}

static int netc_1588_timer_cancel_event(struct hw_timer *hw_timer)
{
	struct netc_1588_timer *netc_timer = container_of(hw_timer, struct netc_1588_timer, hw_timer);
	struct netc_hw_clock *hw_clock = container_of(netc_timer, struct netc_hw_clock, timer[netc_timer->id]);

	netc_1588_timer_stop_alarm(&hw_clock->timer_handle, netc_timer->id);

	return 0;
}

void netc_timer_callback(void *data)
{
	/* check which timer generated the IRQ (TMR_EVENT register, AFAIK), map to the struct hw_timer and call .func */
	struct netc_hw_clock *netc_hw_clock = data;
	int i;

	/* Check if the event is set in the timer event register */
	for (i = 0; i < netc_hw_clock->timer_n; i++) {
		if ((hw_clock->timer_handle.hw.base->TMR_TEVENT & netc_hw_clock->timer[i].irq_flag) != 0U) {
			if (netc_hw_clock->timer[i].hw_timer.func) {
				netc_hw_clock->timer[i].hw_timer.func(netc_hw_clock->timer[i].hw_timer.data);
			} else {
				os_log(LOG_ERR, "invalid isr, timer id: %d\n", netc_hw_clock->timer[i].id);
			}

			hw_clock->timer_handle.hw.base->TMR_TEVENT = netc_hw_clock->timer[i].irq_flag;
		}
	}
}

/**
 * \brief local function to deinitalize the Netc 1588 Timer
 *
 * \param id a clock id for local hw_clock array
 *
 */
void __netc_1588_exit(unsigned int id)
{
	struct netc_hw_clock *netc_hw_clock = &hw_clock[id];

	hw_clock_unregister(HW_CLOCK_PORT_BASE + netc_hw_clock->id);

	if (netc_hw_clock->owner)
		NETC_TimerDeinit(&netc_hw_clock->timer_handle);
}

/**
 * \brief local function to initalize the Netc 1588 Timer
 *
 * \param id a clock id for local hw_clock array
 * \return 0 on success and -1 on error
 */
int __netc_1588_init(unsigned int id)
{
	struct netc_hw_clock *netc_hw_clock = &hw_clock[id];
	netc_timer_alarm_t alarm = {.pulseGenSync = true, .pulseWidth = 0, .polarity = false, .enableInterrupt = true};
	int i;

	if (netc_hw_clock->owner) {
		netc_timer_config_t timer_config;
		netc_msix_entry_t timerMsixEntry;

		/* MSIX configuration.*/
		timerMsixEntry.control = kNETC_MsixIntrMaskBit;
		timerMsixEntry.msgAddr = msgintr_msix_msgaddr(netc_hw_clock->msgintr.base, netc_hw_clock->msgintr.channel);
		timerMsixEntry.msgData = TIMER_MSG_DATA;

		memset(&timer_config, 0, sizeof(netc_timer_config_t));
		timer_config.msixEntry = &timerMsixEntry;
		timer_config.entryNum  = 1;
		timer_config.clockSelect = kNETC_TimerExtRefClk;
		timer_config.refClkHz = CLOCK_GetRootClockFreq(kCLOCK_Root_Tmr_1588);
		timer_config.enableTimer = true;

		os_log(LOG_INFO, "refClkHz: %u defaultPpb: %u\n", timer_config.refClkHz, timer_config.defaultPpb);

		if (NETC_TimerInit(&netc_hw_clock->timer_handle, &timer_config) != kStatus_Success)
			goto err;

		/* Unmask MSIX message interrupt. */
		NETC_TimerMsixSetEntryMask(&netc_hw_clock->timer_handle, 0, false);

	} else {
		os_log(LOG_INFO, "NETC 1588 Timer has started\n");
		NETC_TimerInitHandle(&netc_hw_clock->timer_handle);
	}

	/* The core which configures the timers will save the user context with callback */
	if (netc_hw_clock->timer_n) {
		if (msgintr_init_vector(netc_hw_clock->msgintr.base, netc_hw_clock->msgintr.channel, &netc_timer_callback, netc_hw_clock) == 0)
			goto err_msgintr_vector;
	}

	for (i = 0; i < netc_hw_clock->timer_n; i++) {

		if (netc_hw_clock->timer[i].id == kNETC_TimerAlarm1) {
			netc_hw_clock->timer[i].irq_flag = kNETC_TimerAlarm1IrqFlag;
		} else if (netc_hw_clock->timer[i].id == kNETC_TimerAlarm2) {
			netc_hw_clock->timer[i].irq_flag = kNETC_TimerAlarm2IrqFlag;
		} else {
			os_log(LOG_ERR, "failed to identify the timer source\n");
			goto err_irq_flag;
		}

		/* Configure Alarms */
		NETC_TimerConfigureAlarm(&netc_hw_clock->timer_handle, netc_hw_clock->timer[i].id, &alarm);
	}

	/* Enable Timer */
	NETC_TimerEnable(&netc_hw_clock->timer_handle, true);

	netc_hw_clock->clock.read_counter = netc_1588_read_counter;

	if (netc_hw_clock->owner)
		netc_hw_clock->clock.adj_freq = netc_1588_clock_adj_freq;

	netc_hw_clock->clock.priv = &netc_hw_clock->timer_handle;

	if (hw_clock_register(HW_CLOCK_PORT_BASE + netc_hw_clock->id, &netc_hw_clock->clock) < 0) {
		os_log(LOG_ERR, "failed to register hw_clock(%d)\n", HW_CLOCK_PORT_BASE + netc_hw_clock->id);
		goto err_clock_register;
	}

	for (i = 0; i < netc_hw_clock->timer_n; i++) {
		struct hw_timer *hw_timer = &netc_hw_clock->timer[i].hw_timer;
		unsigned int flags = 0;

		hw_timer->set_next_event = netc_1588_timer_set_next_event;
		hw_timer->cancel_event = netc_1588_timer_cancel_event;

		if (hw_timer_register(HW_CLOCK_PORT_BASE + netc_hw_clock->id, hw_timer, flags) < 0) {
			os_log(LOG_ERR, "%s : failed to register hw timer\n", __func__);
			continue;
		}
	}

	return 0;

err_clock_register:
err_irq_flag:
	if (netc_hw_clock->timer_n)
		msgintr_reset_vector(netc_hw_clock->msgintr.base, netc_hw_clock->msgintr.channel);

err_msgintr_vector:
	if (netc_hw_clock->owner) {
		NETC_TimerMsixSetEntryMask(&netc_hw_clock->timer_handle, 0, true); /* Mask MSI-X */
		NETC_TimerDeinit(&netc_hw_clock->timer_handle);
	}
err:
	return -1;
}

void netc_1588_exit(void)
{
	int i;

	for (i = 0; i < CFG_NUM_NETC_HW_CLOCK; i++)
		__netc_1588_exit(i);
}

int netc_1588_init(void)
{
	int i;

	for (i = 0; i < CFG_NUM_NETC_HW_CLOCK; i++)
		if (__netc_1588_init(i) < 0)
			goto err;

	return 0;

err:
#if CFG_NUM_NETC_HW_CLOCK > 1
	for (i--; i >= 0; i--)
		__netc_1588_exit(i);
#endif /* if CFG_NUM_NETC_HW_CLOCK > 1 */

	return -1;
}

#else
int netc_1588_init(void) { return 0; }
void netc_1588_exit(void) {}
uint64_t netc_1588_hwts_to_u64(void *handle, uint32_t hwts_ns) { return 0; }
#endif /* CFG_NUM_NETC_HW_CLOCK */
