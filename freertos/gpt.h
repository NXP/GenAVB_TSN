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
#ifndef _GPT_H_
#define _GPT_H_

#include "fsl_gpt.h"
#include "hw_timer.h"
#include "media_clock.h"
#include "media_clock_rec_pll.h"
#include "hw_clock.h"

#define GPT_DEVICES		2

#define GPT_REC_FACTOR		16
#define GPT_REC_P_FACTOR	1
#define GPT_REC_I_FACTOR	3
#define GPT_REC_MAX_ADJUST	100
#define GPT_REC_NB_START	10

#define GPT_HW_TIMER_MODE	(1 << 0)
#define GPT_MCLOCK_REC_MODE	(1 << 1)
#define GPT_HW_CLOCK_MODE	(1 << 2)
#define GPT_AVB_HW_TIMER_MODE	(1 << 3)

#define DOMAIN_0		0
#define GPT_NUM_TIMER		3

struct gpt_output_compare {
	gpt_output_compare_channel_t channel;
	gpt_interrupt_enable_t interrupt_mask;
	gpt_status_flag_t status_flag_mask;
};

struct gpt_input_capture {
	gpt_input_capture_channel_t channel;
	gpt_interrupt_enable_t interrupt_mask;
	gpt_status_flag_t status_flag_mask;
	gpt_input_operation_mode_t operation_mode;
};

struct gpt_timer {
	unsigned int id;
	struct hw_timer hw_timer;
	struct gpt_output_compare compare;
};

struct gpt_rec {
	struct mclock_rec_pll rec;
	struct gpt_input_capture capture;

	uint32_t cnt_last;
	unsigned int eth_port;
};

struct gpt_dev {
	GPT_Type *base;
	IRQn_Type irq;
	unsigned int func_mode;

	clock_name_t input_clock;
	uint32_t gpt_input_clk_rate;
	gpt_clock_source_t clk_src_type;
	uint32_t prescale;

	/*
	 * To be removed.
	 * Legacy AVB HW timer support
	 */
  	struct {
		gpt_output_compare_channel_t channel;
		gpt_interrupt_enable_t interrupt_mask;
		gpt_status_flag_t status_flag_mask;
	} avb_timer;
	struct hw_avb_timer_dev avb_timer_dev;

	struct gpt_timer timer[GPT_NUM_TIMER];
	struct gpt_rec gpt_rec;

	uint32_t next_cycles;

	struct hw_clock clock;
	hw_clock_id_t clock_id;
};

struct gpt_dev *gpt_dev_from_device_id(unsigned int device_id);
int gpt_driver_init(void);
void gpt_driver_exit(void);
int gpt_init(struct gpt_dev *dev);
int gpt_exit(struct gpt_dev *dev);

#endif /* _GPT_H_ */

