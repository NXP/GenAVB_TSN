/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief AVB STM driver
 @details
*/

#include "config.h"

#include "common/log.h"

#include "stm.h"
#include "hw_timer.h"
#include "net_port.h"

#if CFG_NUM_STM > 0

#include "fsl_stm.h"

struct stm_output_compare {
	stm_channel_t channel;
	stm_channel_match_t status_flag_mask; /* Channels used as timer: logical OR of members of stm_status_flags_t */
	/* note: the driver doesn't implement an interrupt_enable structure */
};

struct stm_timer {
	unsigned int id;
	struct hw_timer hw_timer;
	struct stm_output_compare compare;
};

struct stm_dev {
	STM_Type *base;
	IRQn_Type irq;

	uint32_t stm_input_clk_rate;
	uint8_t prescale;

	struct stm_timer timer[STM_CHANNEL_COUNT];

	struct hw_clock clock;
	hw_clock_id_t clock_id;
};

static struct stm_dev stm_devices[CFG_NUM_STM] = {
	[0] = {
		.base = BOARD_STM_0_BASE,
		.irq = BOARD_STM_0_IRQ,

#ifdef BOARD_STM_0_PRESCALE
		.prescale = BOARD_STM_0_PRESCALE,
#else
		.prescale = 1,
#endif
		.clock = {
			.period = ((uint64_t)1 << 32),
			.to_ns = {
				.shift = 24,
			},
			.to_cyc = {
				.shift = 32,
			},
		},
		.clock_id = HW_CLOCK_MONOTONIC,
		.timer = {
			[0] = {
				.id = 0,
				.compare = {
					.channel = kSTM_Channel_0,
					.status_flag_mask = kSTM_Channel_Match_Msk_0,
				},
			},
			[1] = {
				.id = 1,
				.compare = {
					.channel = kSTM_Channel_1,
					.status_flag_mask = kSTM_Channel_Match_Msk_1,
				},
			},
			[2] = {
				.id = 2,
				.compare = {
					.channel = kSTM_Channel_2,
					.status_flag_mask = kSTM_Channel_Match_Msk_2,
				},
			},
			[3] = {
				.id = 3,
				.compare = {
					.channel = kSTM_Channel_3,
					.status_flag_mask = kSTM_Channel_Match_Msk_3,
				},
			},
		},
	},
#if CFG_NUM_STM > 1
	[1] = {
		.base = BOARD_STM_1_BASE,
		.irq = BOARD_STM_1_IRQ,
#ifdef BOARD_STM_1_PRESCALE
		.prescale = BOARD_STM_1_PRESCALE,
#else
		.prescale = 1,
#endif
		.clock = {
			.period = ((uint64_t)1 << 32),
			.to_ns = {
				.shift = 24,
			},
			.to_cyc = {
				.shift = 32,
			},
		},
		.clock_id = HW_CLOCK_MONOTONIC_1,
		.timer = {
			[0] = {
				.id = 0,
				.compare = {
					.channel = kSTM_Channel_0,
					.status_flag_mask = kSTM_Channel_Match_Msk_0,
				},
			},
			[1] = {
				.id = 1,
				.compare = {
					.channel = kSTM_Channel_1,
					.status_flag_mask = kSTM_Channel_Match_Msk_1,
				},
			},
			[2] = {
				.id = 2,
				.compare = {
					.channel = kSTM_Channel_2,
					.status_flag_mask = kSTM_Channel_Match_Msk_2,
				},
			},
			[3] = {
				.id = 3,
				.compare = {
					.channel = kSTM_Channel_3,
					.status_flag_mask = kSTM_Channel_Match_Msk_3,
				},
			},
		},
	},
#endif
#if CFG_NUM_STM > 2
#error invalid CFG_NUM_STM
#endif
};

static void stm_timer_interrupt(struct stm_dev *dev, uint32_t flags)
{
	struct stm_timer *timer = &dev->timer[0];

	while (flags) {
		if (flags & 1) {
			STM_ClearStatusFlags(dev->base,
					     timer->compare.channel);
			STM_DisableCompareChannel(dev->base,
						  timer->compare.channel);
			if (timer->hw_timer.func)
				timer->hw_timer.func(timer->hw_timer.data);
		}

		flags >>= 1;
		timer++;
	};
}

static void stm_dev_interrupt(struct stm_dev *dev)
{
	uint32_t flags = 0;
	uint32_t i;

	for (i = 0U; i < STM_CHANNEL_COUNT; i++) {
		/* Collect interrupt flag from all channels */
		flags |= STM_GetStatusFlags(dev->base, (stm_channel_t)i) << i;
	}

	stm_timer_interrupt(dev, flags);
}

void BOARD_STM_0_IRQ_HANDLER(void)
{
	stm_dev_interrupt(&stm_devices[0]);

	SDK_ISR_EXIT_BARRIER;
}

#if CFG_NUM_STM > 1
void BOARD_STM_1_IRQ_HANDLER(void)
{
	stm_dev_interrupt(&stm_devices[1]);

	SDK_ISR_EXIT_BARRIER;
}
#endif

static int stm_hw_timer_set_next_event(struct hw_timer *hw_timer, uint64_t cycles)
{
	struct stm_timer *stm_timer = container_of(hw_timer, struct stm_timer, hw_timer);
	struct stm_dev *stm_dev = container_of(stm_timer, struct stm_dev, timer[stm_timer->id]);
	STM_SetCompare(stm_dev->base, stm_timer->compare.channel, (uint32_t)cycles);
	return 0;
}

static int stm_hw_timer_cancel_event(struct hw_timer *hw_timer)
{
	struct stm_timer *stm_timer = container_of(hw_timer, struct stm_timer, hw_timer);
	struct stm_dev *stm_dev = container_of(stm_timer, struct stm_dev, timer[stm_timer->id]);
	STM_DisableCompareChannel(stm_dev->base, stm_timer->compare.channel);
	return 0;
}

static uint64_t stm_read_counter(void *priv)
{
	struct stm_dev *stm_dev = (struct stm_dev *)priv;

	return STM_GetTimerCount(stm_dev->base);
}

__init static void stm_reg_init(struct stm_dev *dev)
{
	stm_config_t cfg;

	STM_GetDefaultConfig(&cfg);

	cfg.prescale = dev->prescale;
	cfg.enableRunInDebug = false;

	STM_Init(dev->base, &cfg);

	STM_StartTimer(dev->base);
}

__init static int stm_init(struct stm_dev *dev)
{
	struct hw_clock *clock = &dev->clock;
	unsigned int flags = 0;
	int i;

	dev->stm_input_clk_rate = BOARD_STM_clk_freq(dev->base);

	stm_reg_init(dev);

	/* HW Clock */
	clock->rate = dev->stm_input_clk_rate / dev->prescale;
	clock->read_counter = &stm_read_counter;
	clock->priv = dev;

	if (hw_clock_register(dev->clock_id, clock) < 0)
		os_log(LOG_ERR, "failed to register hw_clock(%d)\n", dev->clock_id);

	/* HW Timers */
	for (i = 0; i < STM_CHANNEL_COUNT; i++) {
		struct hw_timer *hw_timer = &dev->timer[i].hw_timer;

		hw_timer->set_next_event = &stm_hw_timer_set_next_event;
		hw_timer->cancel_event = &stm_hw_timer_cancel_event;

		if (hw_timer_register(dev->clock_id, hw_timer, flags) < 0) {
			os_log(LOG_ERR, "failed to register hw timer\n");
			continue;
		}
	}

	return 0;
}

__exit static int stm_exit(struct stm_dev *dev)
{
	DisableIRQ(dev->irq);
	STM_Deinit(dev->base);

	hw_clock_unregister(dev->clock_id);

	return 0;
}

__init int stm_driver_init(void)
{
	int i, rc = 0;

	for (i = 0; i < CFG_NUM_STM; i++) {
		if (stm_init(&stm_devices[i]) < 0) {
			os_log(LOG_ERR, "stm_init failed: device %d\n", i);
			rc = -1;
			goto err;
		}
	}

	return rc;
err:
#if CFG_NUM_STM > 1
	for (i--; i >= 0; i--)
		stm_exit(&stm_devices[i]);
#endif

	return rc;
}

__exit void stm_driver_exit(void)
{
	int i;

	for (i = 0; i < CFG_NUM_STM; i++)
		stm_exit(&stm_devices[i]);
}
#else
__init int stm_driver_init(void) { return 0; }
__exit void stm_driver_exit(void) {}
#endif /* CFG_NUM_STM */
