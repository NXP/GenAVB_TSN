/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief FTM driver implementation
 @details
*/

#include "config.h"

#include "common/log.h"
#include "common/types.h"

#include "ftm.h"
#include "hw_timer.h"
#include "hw_clock.h"

#if CFG_NUM_FTM > 0

#include "fsl_ftm.h"

#define FTM_TIMER_CHANNEL_IRQ_NUM		CFG_FTM_NUM_IRQS
#define FTM_NUM_TIMER		(CFG_FTM_NUM_IRQS * 2)

struct ftm_output_compare {
	ftm_chnl_t channel;
	uint32_t interrupt_enable_mask;
	uint32_t status_flag_mask;
};

struct ftm_timer {
	unsigned int id;
	struct hw_timer hw_timer;
	struct ftm_output_compare compare;
};

struct ftm_dev {
	FTM_Type *base;
	IRQn_Type irq[FTM_TIMER_CHANNEL_IRQ_NUM];

	uint32_t ftm_clk_src_type;
	uint32_t ftm_input_clk_rate;
	ftm_clock_prescale_t prescale;

	struct ftm_timer timer[FTM_NUM_TIMER];

	struct hw_clock clock;
	hw_clock_id_t clock_id;
};

static struct ftm_dev ftm_devices[CFG_NUM_FTM] = {
	[0] = {
		.base = BOARD_FTM_0_BASE,
		.irq = {
			[0] = BOARD_FTM_0_CH01_IRQ,
#if FTM_TIMER_CHANNEL_IRQ_NUM > 1
			[1] = BOARD_FTM_0_CH23_IRQ,
#endif
#if FTM_TIMER_CHANNEL_IRQ_NUM > 2
			[2] = BOARD_FTM_0_CH45_IRQ,
#endif
#if FTM_TIMER_CHANNEL_IRQ_NUM > 3
			[3] = BOARD_FTM_0_CH67_IRQ
#endif
#if FTM_TIMER_CHANNEL_IRQ_NUM > 4
#error invalid FTM_TIMER_CHANNEL_IRQ_NUM
#endif
		},
#ifdef BOARD_FTM_0_PRESCALE
		.prescale = BOARD_FTM_0_PRESCALE,
#else
		.prescale = kFTM_Prescale_Divide_1,
#endif
		.clock = {
			.period = 0x10000ULL, /* 16-bit counter */
			.to_ns = {
				.shift = 16,
			},
			.to_cyc = {
				.shift = 30,
			},
		},
		.clock_id = HW_CLOCK_MONOTONIC,
		.timer = {
			[0] = {
				.id = 0,
				.compare = {
					.channel = kFTM_Chnl_0,
					.interrupt_enable_mask = kFTM_Chnl0InterruptEnable,
					.status_flag_mask = kFTM_Chnl0Flag,
				},
			},
#if FTM_NUM_TIMER > 1
			[1] = {
				.id = 1,
				.compare = {
					.channel = kFTM_Chnl_1,
					.interrupt_enable_mask = kFTM_Chnl1InterruptEnable,
					.status_flag_mask = kFTM_Chnl1Flag,
				},
			},
#endif
#if FTM_NUM_TIMER > 2
			[2] = {
				.id = 2,
				.compare = {
					.channel = kFTM_Chnl_2,
					.interrupt_enable_mask = kFTM_Chnl2InterruptEnable,
					.status_flag_mask = kFTM_Chnl2Flag,
				},
			},
#endif
#if FTM_NUM_TIMER > 3
			[3] = {
				.id = 3,
				.compare = {
					.channel = kFTM_Chnl_3,
					.interrupt_enable_mask = kFTM_Chnl3InterruptEnable,
					.status_flag_mask = kFTM_Chnl3Flag,
				},
			},
#endif
#if FTM_NUM_TIMER > 4
			[4] = {
				.id = 4,
				.compare = {
					.channel = kFTM_Chnl_4,
					.interrupt_enable_mask = kFTM_Chnl4InterruptEnable,
					.status_flag_mask = kFTM_Chnl4Flag,
				},
			},
#endif
#if FTM_NUM_TIMER > 5
			[5] = {
				.id = 5,
				.compare = {
					.channel = kFTM_Chnl_5,
					.interrupt_enable_mask = kFTM_Chnl5InterruptEnable,
					.status_flag_mask = kFTM_Chnl5Flag,
				},
			},
#endif
#if FTM_NUM_TIMER > 6
			[6] = {
				.id = 6,
				.compare = {
					.channel = kFTM_Chnl_6,
					.interrupt_enable_mask = kFTM_Chnl6InterruptEnable,
					.status_flag_mask = kFTM_Chnl6Flag,
				},
			},
#endif
#if FTM_NUM_TIMER > 7
			[7] = {
				.id = 7,
				.compare = {
					.channel = kFTM_Chnl_7,
					.interrupt_enable_mask = kFTM_Chnl7InterruptEnable,
					.status_flag_mask = kFTM_Chnl7Flag,
				},
			},
#endif
		},
	},
#if CFG_NUM_FTM > 1
	[1] = {
		.base = BOARD_FTM_1_BASE,
		.irq = {
			[0] = BOARD_FTM_1_CH01_IRQ,
#if FTM_TIMER_CHANNEL_IRQ_NUM > 1
			[1] = BOARD_FTM_1_CH23_IRQ,
#endif
#if FTM_TIMER_CHANNEL_IRQ_NUM > 2
			[2] = BOARD_FTM_1_CH45_IRQ,
#endif
#if FTM_TIMER_CHANNEL_IRQ_NUM > 3
			[3] = BOARD_FTM_1_CH67_IRQ
#endif
#if FTM_TIMER_CHANNEL_IRQ_NUM > 4
#error invalid FTM_TIMER_CHANNEL_IRQ_NUM
#endif
		},
#ifdef BOARD_FTM_1_PRESCALE
		.prescale = BOARD_FTM_1_PRESCALE,
#else
		.prescale = kFTM_Prescale_Divide_1,
#endif

		.clock = {
			.period = 0x10000ULL, /* 16-bit counter */
			.to_ns = {
				.shift = 16,
			},
			.to_cyc = {
				.shift = 16,
			},
		},
		.clock_id = HW_CLOCK_MONOTONIC_1,
		.timer = {
			[0] = {
				.id = 0,
				.compare = {
					.channel = kFTM_Chnl_0,
					.interrupt_enable_mask = kFTM_Chnl0InterruptEnable,
					.status_flag_mask = kFTM_Chnl0Flag,
				},
			},
#if FTM_NUM_TIMER > 1
			[1] = {
				.id = 1,
				.compare = {
					.channel = kFTM_Chnl_1,
					.interrupt_enable_mask = kFTM_Chnl1InterruptEnable,
					.status_flag_mask = kFTM_Chnl1Flag,
				},
			},
#endif
#if FTM_NUM_TIMER > 2
			[2] = {
				.id = 2,
				.compare = {
					.channel = kFTM_Chnl_2,
					.interrupt_enable_mask = kFTM_Chnl2InterruptEnable,
					.status_flag_mask = kFTM_Chnl2Flag,
				},
			},
#endif
#if FTM_NUM_TIMER > 3
			[3] = {
				.id = 3,
				.compare = {
					.channel = kFTM_Chnl_3,
					.interrupt_enable_mask = kFTM_Chnl3InterruptEnable,
					.status_flag_mask = kFTM_Chnl3Flag,
				},
			},
#endif
#if FTM_NUM_TIMER > 4
			[4] = {
				.id = 4,
				.compare = {
					.channel = kFTM_Chnl_4,
					.interrupt_enable_mask = kFTM_Chnl4InterruptEnable,
					.status_flag_mask = kFTM_Chnl4Flag,
				},
			},
#endif
#if FTM_NUM_TIMER > 5
			[5] = {
				.id = 5,
				.compare = {
					.channel = kFTM_Chnl_5,
					.interrupt_enable_mask = kFTM_Chnl5InterruptEnable,
					.status_flag_mask = kFTM_Chnl5Flag,
				},
			},
#endif
#if FTM_NUM_TIMER > 6
			[6] = {
				.id = 6,
				.compare = {
					.channel = kFTM_Chnl_6,
					.interrupt_enable_mask = kFTM_Chnl6InterruptEnable,
					.status_flag_mask = kFTM_Chnl6Flag,
				},
			},
#endif
#if FTM_NUM_TIMER > 7
			[7] = {
				.id = 7,
				.compare = {
					.channel = kFTM_Chnl_7,
					.interrupt_enable_mask = kFTM_Chnl7InterruptEnable,
					.status_flag_mask = kFTM_Chnl7Flag,
				},
			},
#endif
		},
	},
		},
	},
#endif
#if CFG_NUM_FTM > 2
#error invalid CFG_NUM_FTM
#endif
};

static void ftm_timer_interrupt(struct ftm_dev *dev, uint32_t flags, uint32_t enabled_irqs)
{
	struct ftm_timer *timer;
	int i;

	for (i = 0; i < FTM_NUM_TIMER; i++) {
		timer = &dev->timer[i];

		if ((flags & timer->compare.status_flag_mask) && (enabled_irqs & timer->compare.interrupt_enable_mask)) {
			FTM_DisableInterrupts(dev->base, timer->compare.interrupt_enable_mask);
			FTM_ClearStatusFlags(dev->base, timer->compare.status_flag_mask);

			if (timer->hw_timer.func)
				timer->hw_timer.func(timer->hw_timer.data);
		}
	}
}

static void ftm_dev_interrupt(struct ftm_dev *dev)
{
	uint32_t enabled_interrupts = FTM_GetEnabledInterrupts(dev->base);
	uint32_t flags = FTM_GetStatusFlags(dev->base);

	ftm_timer_interrupt(dev, flags, enabled_interrupts);
}

static void BOARD_IRQ_HANDLER_COMMON(unsigned int ftm_instance_index)
{
	ftm_dev_interrupt(&ftm_devices[ftm_instance_index]);

	SDK_ISR_EXIT_BARRIER;
}

void BOARD_FTM_0_CH01_IRQ_HANDLER(void)
{
	BOARD_IRQ_HANDLER_COMMON(0);
}

#ifdef BOARD_FTM_0_CH23_IRQ_HANDLER
void BOARD_FTM_0_CH23_IRQ_HANDLER(void)
{
	BOARD_IRQ_HANDLER_COMMON(0);
}
#endif

#ifdef BOARD_FTM_0_CH45_IRQ_HANDLER
void BOARD_FTM_0_CH45_IRQ_HANDLER(void)
{
	BOARD_IRQ_HANDLER_COMMON(0);
}
#endif

#ifdef BOARD_FTM_0_CH67_IRQ_HANDLER
void BOARD_FTM_0_CH67_IRQ_HANDLER(void)
{
	BOARD_IRQ_HANDLER_COMMON(0);
}
#endif

#if CFG_NUM_FTM > 1
void BOARD_FTM_1_CH01_IRQ_HANDLER(void)
{
	BOARD_IRQ_HANDLER_COMMON(1);
}

void BOARD_FTM_1_CH23_IRQ_HANDLER(void)
{
	BOARD_IRQ_HANDLER_COMMON(1);
}

void BOARD_FTM_1_CH45_IRQ_HANDLER(void)
{
	BOARD_IRQ_HANDLER_COMMON(1);
}

void BOARD_FTM_1_CH67_IRQ_HANDLER(void)
{
	BOARD_IRQ_HANDLER_COMMON(1);
}
#endif

static int ftm_hw_timer_set_next_event(struct hw_timer *hw_timer, uint64_t cycles)
{
	struct ftm_timer *ftm_timer = container_of(hw_timer, struct ftm_timer, hw_timer);
	struct ftm_dev *ftm_dev = container_of(ftm_timer, struct ftm_dev, timer[ftm_timer->id]);

	FTM_ClearStatusFlags(ftm_dev->base, ftm_timer->compare.status_flag_mask);
	FTM_EnableInterrupts(ftm_dev->base, ftm_timer->compare.interrupt_enable_mask);
	FTM_SetChannelMatchValue(ftm_dev->base, ftm_timer->compare.channel, cycles);

	return 0;
}

static int ftm_hw_timer_cancel_event(struct hw_timer *hw_timer)
{
	struct ftm_timer *ftm_timer = container_of(hw_timer, struct ftm_timer, hw_timer);
	struct ftm_dev *ftm_dev = container_of(ftm_timer, struct ftm_dev, timer[ftm_timer->id]);

	FTM_DisableInterrupts(ftm_dev->base, ftm_timer->compare.interrupt_enable_mask);

	return 0;
}

static uint64_t ftm_read_counter(void *priv)
{
	struct ftm_dev *ftm_dev = (struct ftm_dev *)priv;

	return (uint64_t)FTM_GetCurrentTimerCount(ftm_dev->base);
}

__init static void ftm_reg_init(struct ftm_dev *dev)
{
	ftm_config_t cfg;
	int i;

	FTM_GetDefaultConfig(&cfg);

	cfg.prescale = dev->prescale;

	FTM_Init(dev->base, &cfg);

	/* Enable free running counter and synchronization compatible with TPM */
	dev->base->MODE &= ~FTM_MODE_FTMEN_MASK;

	FTM_SetTimerPeriod(dev->base, dev->clock.period - 1);

	for (i = 0; i < FTM_NUM_TIMER; i++)
		FTM_SetupOutputCompare(dev->base, dev->timer[i].compare.channel,
					kFTM_SetOnMatch, 0);

	FTM_StartTimer(dev->base, dev->ftm_clk_src_type);
}

__init static int ftm_init(struct ftm_dev *dev)
{
	struct hw_clock *clock = &dev->clock;
	unsigned int flags = 0;
	int i, j;

	dev->ftm_input_clk_rate = BOARD_FTM_clk_freq(dev->base);
	dev->ftm_clk_src_type = BOARD_FTM_clk_src(dev->base);

	ftm_reg_init(dev);
	for (j = 0; j < FTM_TIMER_CHANNEL_IRQ_NUM; j++) {
		EnableIRQ(dev->irq[j]);
	}

	/* HW Clock */
	clock->rate = dev->ftm_input_clk_rate / (1U << dev->prescale);
	clock->read_counter = &ftm_read_counter;
	clock->priv = dev;

	if (hw_clock_register(dev->clock_id, clock) < 0) {
		os_log(LOG_ERR, "failed to register hw_clock(%d)\n", dev->clock_id);
		return -1;
	}

	/* HW Timers */
	for (i = 0; i < FTM_NUM_TIMER; i++) {
		struct hw_timer *hw_timer = &dev->timer[i].hw_timer;

		hw_timer->set_next_event = &ftm_hw_timer_set_next_event;
		hw_timer->cancel_event = &ftm_hw_timer_cancel_event;

		if (hw_timer_register(dev->clock_id, hw_timer, flags) < 0) {
			os_log(LOG_ERR, "failed to register hw timer %d\n", i);
			continue;
		}
	}

	return 0;
}

__exit static int ftm_exit(struct ftm_dev *dev)
{
	int i;

	for (i = 0; i < FTM_TIMER_CHANNEL_IRQ_NUM; i++) {
		DisableIRQ(dev->irq[i]);
	}

	FTM_Deinit(dev->base);
	hw_clock_unregister(dev->clock_id);

	return 0;
}

__init int ftm_driver_init(void)
{
	int i, rc = 0;

	for (i = 0; i < CFG_NUM_FTM; i++) {
		if (ftm_init(&ftm_devices[i]) < 0) {
			os_log(LOG_ERR, "ftm_init failed: device %d\n", i);
			rc = -1;
			goto err;
		}
	}

	return rc;
err:
#if CFG_NUM_FTM > 1
	for (i--; i >= 0; i--)
		ftm_exit(&ftm_devices[i]);
#endif

	return rc;
}

__exit void ftm_driver_exit(void)
{
	int i;

	for (i = 0; i < CFG_NUM_FTM; i++)
		ftm_exit(&ftm_devices[i]);
}

#else
__init int ftm_driver_init(void) { return 0; }
__exit void ftm_driver_exit(void) {}
#endif /* CFG_NUM_FTM */
