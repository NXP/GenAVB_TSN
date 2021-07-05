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
 @brief FreeRTOS generic hw clock service implementation
 @details
*/

#ifndef _HW_CLOCK_H_
#define _HW_CLOCK_H_

#include "common/types.h"

typedef enum {
	HW_CLOCK_NONE = 0,
	HW_CLOCK_MONOTONIC = 1,
	HW_CLOCK_LOCAL_EP_0,
	HW_CLOCK_LOCAL_EP_1,
	HW_CLOCK_LOCAL_BR_0,
	HW_CLOCK_LOCAL_BR_1,
	HW_CLOCK_MAX
} hw_clock_id_t;

/*
 * Clock locking model.
 *
 * Two global locks are used. One protects from task preemption using
 * vTaskSuspendAll()/xTaskResumeAll(), the other from interrupts preemption
 * using taskENTER_CRITICAL()/taskEXIT_CRITICAL().
 *
 * The task preemption lock is the responsability of the hw_clock layer user.
 * The interrupt preemption lock allows calling readers in interrupt context and
 * is performed here to do it in the most efficient way.
 *
 * The hw_clock object needs to be updated atomically so that readers don't get
 * corrupted time.
 * Readers: hw_clock_time_to_cycles(), hw_clock_cycles_to_time()
 * Writers: hw_clock_read(), hw_clock_setoffset()
 *
 * Readers in task context are protected against task preemption and readers from
 * interrupt are protected by an IRQ disable done by the writer.
 */

/**
 * struct hw_clock - abstraction for a cycle counter
 *  	Provides time in ns since start of the HW counter and conversion functions
 *  	from time in nanoseconds to counter cycles.
 * 	This implementation has no restrictions concerning the period value (no
 * 	need to be a power of 2) but on the other hand does not scale to a full
 * 	64 bits register support.
 *
 * @read_counter:	returns raw counter value
 * @adj_freq:		adjust counter frequency in ppb
 * @period:		period in counter cycles
 * @rate: 		rate of the counter
 * @frac:		cycles remainder of time conversion
 * @nsecs:		ns count
 * @last_cycles:	last cycles read by read_counter
 */
struct hw_clock {
	uint64_t (*read_counter)(struct hw_clock *clk);
	int (*adj_freq)(struct hw_clock *clk, int32_t ppb);
	uint64_t period;
	uint32_t rate;
	struct {
		uint32_t shift;
		uint32_t mult;
	} to_ns;
	struct {
		uint32_t shift;
		uint32_t mult;
	} to_cyc;

	int64_t frac;
	uint64_t nsecs;
	uint64_t last_cycles;
};

int hw_clock_register(hw_clock_id_t id, struct hw_clock *clk);
struct hw_clock *hw_clock_get(hw_clock_id_t id);

uint64_t hw_clock_read(struct hw_clock *clk);
int hw_clock_setfreq(struct hw_clock *clk, int32_t ppb);

uint64_t hw_clock_cycles_to_time(struct hw_clock *clk, uint64_t cycles);
int hw_clock_time_to_cycles(struct hw_clock *clk, uint64_t ns, uint64_t *cycles);

#endif /* _HW_CLOCK_H_ */
