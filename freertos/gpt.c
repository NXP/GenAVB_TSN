/*
* Copyright 2018-2020 NXP
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
 @brief AVB GPT driver
 @details
*/

#include "gpt.h"
#include "common/log.h"
#include "net_port.h"
#include "gpt_rec.h"


/* Board specific */
#include "dev_itf.h"
#include "system_config.h"

static struct gpt_dev gpt_devices[GPT_DEVICES] = {
	[0] = {
		.base = BOARD_GPT_0_BASE,
		.irq = BOARD_GPT_0_IRQ,
		.func_mode = GPT_HW_CLOCK_MODE | GPT_HW_TIMER_MODE,
		.prescale = 1,
		.clk_src_type = kGPT_ClockSource_Periph,
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
					.channel = kGPT_OutputCompare_Channel1,
					.interrupt_mask = kGPT_OutputCompare1InterruptEnable,
					.status_flag_mask = kGPT_OutputCompare1Flag,
				},
			},
			[1] = {
				.id = 1,
				.compare = {
					.channel = kGPT_OutputCompare_Channel2,
					.interrupt_mask = kGPT_OutputCompare2InterruptEnable,
					.status_flag_mask = kGPT_OutputCompare2Flag,
				},
			},
			[2] = {
				.id = 2,
				.compare = {
					.channel = kGPT_OutputCompare_Channel3,
					.interrupt_mask = kGPT_OutputCompare3InterruptEnable,
					.status_flag_mask = kGPT_OutputCompare3Flag,
				},
			},
		},
	},
	[1] = {
		.base = BOARD_GPT_1_BASE,
		.irq = BOARD_GPT_1_IRQ,
		.prescale = 1,
		.func_mode = GPT_MCLOCK_REC_MODE | GPT_AVB_HW_TIMER_MODE,
		/* Legacy support */
		.avb_timer = {
			.channel = kGPT_OutputCompare_Channel1,
			.interrupt_mask = kGPT_OutputCompare1InterruptEnable,
			.status_flag_mask = kGPT_OutputCompare1Flag,
		},
		.gpt_rec = {
			.capture = {
				.channel = BOARD_GPT_1_CHANNEL,
				.interrupt_mask = BOARD_GPT_1_INTERRUPT_MASK,
				.status_flag_mask = BOARD_GPT_1_STATUS_FLAG_MASK,
				.operation_mode = kGPT_InputOperation_BothEdge,
			},
		},
	},
};

static struct gpt_timer *isr_channel_to_timer[4];

static int gpt_isr_register_timer(struct gpt_timer *timer)
{
	isr_channel_to_timer[timer->compare.channel] = timer;

	return 0;
}

static void gpt_timer_interrupt(struct gpt_dev *dev, uint32_t flags)
{
		int id = 0;
		struct gpt_timer *timer;

		while (flags) {
			if (flags & 1) {
				timer = isr_channel_to_timer[id];

				GPT_DisableInterrupts(dev->base,
						      timer->compare.interrupt_mask);

				GPT_ClearStatusFlags(dev->base,
						     timer->compare.status_flag_mask);

				if (timer->hw_timer.func)
					timer->hw_timer.func(timer->hw_timer.data);
			}
			flags >>= 1;
			id++;
		};
}

static void gpt_dev_interrupt(struct gpt_dev *dev)
{
	uint32_t flags = GPT_GetStatusFlags(dev->base,
					    dev->base->IR & (kGPT_OutputCompare1InterruptEnable |
							     kGPT_OutputCompare2InterruptEnable |
							     kGPT_OutputCompare3InterruptEnable));

	if (dev->func_mode & GPT_AVB_HW_TIMER_MODE)
		hw_avb_timer_interrupt(&dev->avb_timer_dev);

	if (dev->func_mode & GPT_HW_TIMER_MODE)
		gpt_timer_interrupt(dev, flags);
}

void BOARD_GPT_0_IRQ_HANDLER(void)
{
	gpt_dev_interrupt(&gpt_devices[0]);

	/* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F, Cortex-M7, Cortex-M7F Store immediate overlapping
	  exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U || __CORTEX_M == 7U)
	__DSB();
#endif
}

void BOARD_GPT_1_IRQ_HANDLER(void)
{
	gpt_dev_interrupt(&gpt_devices[1]);

#if defined __CORTEX_M && (__CORTEX_M == 4U || __CORTEX_M == 7U)
	__DSB();
#endif
}

static void gpt_hw_timer_start(struct hw_avb_timer_dev *timer_dev)
{
	struct gpt_dev *gpt_dev = container_of(timer_dev, struct gpt_dev, avb_timer_dev);
	uint32_t cycles_now;

	GPT_EnableInterrupts(gpt_dev->base, gpt_dev->avb_timer.interrupt_mask);

	cycles_now = GPT_GetCurrentTimerCount(gpt_dev->base);

	gpt_dev->next_cycles = cycles_now + (uint32_t)timer_dev->period;
	GPT_SetOutputCompareValue(gpt_dev->base, gpt_dev->avb_timer.channel, gpt_dev->next_cycles);
}

static void gpt_hw_timer_stop(struct hw_avb_timer_dev *timer_dev)
{
	struct gpt_dev *gpt_dev = container_of(timer_dev, struct gpt_dev, avb_timer_dev);

	GPT_DisableInterrupts(gpt_dev->base, gpt_dev->avb_timer.interrupt_mask);

	GPT_ClearStatusFlags(gpt_dev->base, gpt_dev->avb_timer.status_flag_mask);
}

static void gpt_hw_timer_set_period(struct hw_avb_timer_dev *timer_dev, unsigned long period_us)
{
	struct gpt_dev *gpt_dev = container_of(timer_dev, struct gpt_dev, avb_timer_dev);

	timer_dev->period = (period_us * (timer_dev->rate / 1000)) / 1000;

	if (((timer_dev->period * 1000) / (timer_dev->rate / 1000)) != period_us)
		os_log(LOG_INFO, "timer_dev(%p) warning the HW timer period is incorrect",
		       timer_dev);

	os_log(LOG_INFO, "gpt_dev (%p) set period %lu(us), %u(cycles)\n", gpt_dev, period_us, timer_dev->period);
}

static unsigned int gpt_hw_timer_elapsed(struct hw_avb_timer_dev *timer_dev, unsigned int cycles)
{
	struct gpt_dev *gpt_dev = container_of(timer_dev, struct gpt_dev, avb_timer_dev);
	uint32_t cycles_now;

	cycles_now = GPT_GetCurrentTimerCount(gpt_dev->base);

	return cycles_now - (uint32_t)cycles;
}

static unsigned int gpt_hw_timer_irq_ack(struct hw_avb_timer_dev *timer_dev, unsigned int *cycles, unsigned int *dcycles)
{
	struct gpt_dev *gpt_dev = container_of(timer_dev, struct gpt_dev, avb_timer_dev);
	unsigned int ticks = 0;
	uint32_t cycles_now, num_recovery = 0;

	cycles_now = GPT_GetCurrentTimerCount(gpt_dev->base);

	*cycles = cycles_now;
try_again:
	GPT_ClearStatusFlags(gpt_dev->base, gpt_dev->avb_timer.status_flag_mask);

	*dcycles = cycles_now - gpt_dev->next_cycles;

	/* Check how many periods have elapsed since last time
	 * Normally a single period has elapsed, but we may be late */
	do {
		gpt_dev->next_cycles += (uint32_t)timer_dev->period;
		ticks++;
	} while ((int)(gpt_dev->next_cycles - cycles_now) < 0);


	/* Set next timer value */
	GPT_SetOutputCompareValue(gpt_dev->base, gpt_dev->avb_timer.channel, gpt_dev->next_cycles);

	/* Check if the free running counter has already passed
	 * the newly programmed target value */
	cycles_now = GPT_GetCurrentTimerCount(gpt_dev->base);

	if ((int)(cycles_now - gpt_dev->next_cycles) > 0) {
		num_recovery++;

		/* hard protection against potential dead loop (> 2*125us) */
		if (num_recovery > 2)
			os_log(LOG_ERR, "gpt_dev(%p) - recovery failure\n", gpt_dev);
		else
			/* try to set again the OCR below CNT */
			goto try_again;
	}

	timer_dev->recovery_errors += num_recovery;

	return ticks;
}

static int gpt_hw_timer_set_next_event(struct hw_timer *hw_timer, uint64_t cycles)
{
	struct gpt_timer *gpt_timer = container_of(hw_timer, struct gpt_timer, hw_timer);
	struct gpt_dev *gpt_dev = container_of(gpt_timer, struct gpt_dev, timer[gpt_timer->id]);

	GPT_ClearStatusFlags(gpt_dev->base, gpt_timer->compare.status_flag_mask);
	GPT_EnableInterrupts(gpt_dev->base, gpt_timer->compare.interrupt_mask);
	GPT_SetOutputCompareValue(gpt_dev->base, gpt_timer->compare.channel, cycles);

	return 0;
}

static int gpt_hw_timer_cancel_event(struct hw_timer *hw_timer)
{
	struct gpt_timer *gpt_timer = container_of(hw_timer, struct gpt_timer, hw_timer);
	struct gpt_dev *gpt_dev = container_of(gpt_timer, struct gpt_dev, timer[gpt_timer->id]);

	GPT_DisableInterrupts(gpt_dev->base, gpt_timer->compare.interrupt_mask);

	return 0;
}

static uint64_t gpt_read_counter(struct hw_clock *clock)
{
	struct gpt_dev *gpt_dev = container_of(clock, struct gpt_dev, clock);

	return GPT_GetCurrentTimerCount(gpt_dev->base);
}

__init static void gpt_reg_init(struct gpt_dev *dev)
{
	gpt_config_t cfg;

	GPT_GetDefaultConfig(&cfg);

	cfg.enableFreeRun = true;
	cfg.enableRunInWait = true;
	cfg.clockSource = dev->clk_src_type;
	cfg.divider = dev->prescale;

	GPT_Init(dev->base, &cfg);

	GPT_StartTimer(dev->base);
}

__init int gpt_init(struct gpt_dev *dev)
{
	struct hw_avb_timer_dev *timer_dev = &dev->avb_timer_dev;
	struct hw_clock *clock = &dev->clock;
	int rc = 0;
	const struct board_config *board_cfg;

	if (!dev->func_mode)
		return 0;

	board_cfg = system_config_get_board();

	/* BOARD_GPT_1_BASE configuration comes from board configuration */
	if (dev->base == BOARD_GPT_1_BASE) {
		dev->clk_src_type = board_cfg->board_gpt_1_clock_source;
		dev->gpt_input_clk_rate = board_cfg->board_gpt_1_input_freq;
	}

	/* It's either provided externally by freeRTOS because board specific
	 * or it is a known system clock
	 */
	switch(dev->clk_src_type) {
	case kGPT_ClockSource_Ext:
		if (!dev->gpt_input_clk_rate) {
			os_log(LOG_ERR, "need to provide gpt input rate\n");
			rc = -1;
			goto err;
		}
		break;

	case kGPT_ClockSource_Periph:
		dev->gpt_input_clk_rate = dev_get_gpt_ipg_freq(dev->base);

		break;

	default:
		os_log(LOG_ERR, "not supported clock source\n");
		rc = -1;
		goto err;
		break;
	}

	EnableIRQ(dev->irq);

	gpt_reg_init(dev);

	if (dev->func_mode & GPT_AVB_HW_TIMER_MODE) {
		timer_dev->rate = dev->gpt_input_clk_rate / dev->prescale;
		timer_dev->min_delay_cycles = (HW_AVB_TIMER_MIN_DELAY_US * timer_dev->rate)
						/ USECS_PER_SEC;
		timer_dev->start = gpt_hw_timer_start;
		timer_dev->stop = gpt_hw_timer_stop;
		timer_dev->irq_ack = gpt_hw_timer_irq_ack;
		timer_dev->elapsed = gpt_hw_timer_elapsed;
		timer_dev->set_period = gpt_hw_timer_set_period;
		timer_dev->initialized = true;

		if (hw_avb_timer_register_device(timer_dev) < 0)
			os_log(LOG_ERR, "%s : failed to register hw clock\n", __func__);
		else
			os_log(LOG_INIT,
	       		"%s : registered AVB HW timer(%p) channel: %u, prescale: %d\n",
			__func__, timer_dev, dev->avb_timer.channel, dev->prescale);
	}

	if (dev->func_mode & GPT_HW_CLOCK_MODE) {
		clock->rate = dev->gpt_input_clk_rate / dev->prescale;
		clock->read_counter = gpt_read_counter;

		if (hw_clock_register(dev->clock_id, clock) < 0)
			os_log(LOG_ERR, "failed to register hw clock\n");
	}

	if (dev->func_mode & GPT_HW_TIMER_MODE) {
		int i;

		for (i = 0; i < GPT_NUM_TIMER; i++) {
			struct hw_timer *hw_timer = &dev->timer[i].hw_timer;

			hw_timer->set_next_event = gpt_hw_timer_set_next_event;
			hw_timer->cancel_event = gpt_hw_timer_cancel_event;

			if (hw_timer_register(dev->clock_id, hw_timer, false) < 0) {
				os_log(LOG_ERR, "%s : failed to register hw timer\n", __func__);
				continue;
			}

			gpt_isr_register_timer(&dev->timer[i]);
		}
	}

	if (dev->func_mode & GPT_MCLOCK_REC_MODE) {
		if (gpt_rec_init(&dev->gpt_rec) < 0)
			os_log(LOG_ERR, "%s : failed to register GPT media clock recovery\n", __func__);
	}

	return 0;

err:
	return rc;
}

__exit int gpt_exit(struct gpt_dev *dev)
{
	GPT_Deinit(dev->base);

	if (dev->func_mode & GPT_HW_TIMER_MODE)
		hw_avb_timer_unregister_device(&dev->avb_timer_dev);

	if (dev->func_mode & GPT_MCLOCK_REC_MODE)
		gpt_rec_exit(&dev->gpt_rec);

	return 0;
}

__init int gpt_driver_init(void)
{
	int i, rc = 0;

	for (i = 0; i < GPT_DEVICES; i++) {
		if (gpt_init(&gpt_devices[i]) < 0) {
			os_log(LOG_ERR, "gpt_init failed: device %d\n", i);
			rc = -1;
			goto err;
		}
	}

	return rc;
err:
	for (i--; i >= 0; i--)
		gpt_exit(&gpt_devices[i]);

	return rc;
}

__exit void gpt_driver_exit(void)
{
	int i;

	for (i = 0; i < GPT_DEVICES; i++)
		gpt_exit(&gpt_devices[i]);
}

