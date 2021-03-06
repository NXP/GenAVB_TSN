/*
* Copyright 2019-2021 NXP
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

#ifndef _FREERTOS_CLOCK_H_
#define _FREERTOS_CLOCK_H_

#include "common/types.h"

#include "hw_clock.h"

struct os_sw_clock {

	/* software clock parameters */
	struct {
		uint64_t t0;
		uint64_t mul;
		unsigned int shift;
	} sw;

	/* hardware clock parameters */
	struct {
		uint64_t t0;
		uint64_t mul;
		unsigned int shift;
	} hw;
};

struct os_clock {
	void *hw_clk;
	unsigned int flags; /* used for hardware clock adjustments capability */

	/* software clock adjustment: */
	struct os_sw_clock sw_clk;
	int32_t ppb;	/* currently configured software frequency adjustment */
	int32_t ppb_internal;	/* adjustment between sw clock and local clock */

	int (*setfreq)(struct os_clock *c, int32_t ppb);
	int (*setoffset)(struct os_clock *c, int64_t offset);
};

/* Clock ratio adjustments affect the hardware clock */
#define OS_CLOCK_FLAGS_HW_RATIO  (1 << 0)
/* Clock offset adjustments affect the hardware clock */
#define OS_CLOCK_FLAGS_HW_OFFSET (1 << 1)
/* Clock is a local clock */
#define OS_CLOCK_FLAGS_IS_LOCAL  (1 << 2)

/**
 * Convert hw clock time to sw clock time.
 * The time must come from the root hw clock of the sw clock
 * given as argument.
 * \param id		clock id.
 * \param hw_ns		hw time to convert
 * \param ns		pointer to u64 variable that will hold the result.
 * \return		0 on success, or negative value on error.
 */
int clock_time_from_hw(os_clock_id_t id, uint64_t hw_ns, uint64_t *ns);

/**
 * Convert sw clock ns time to hw clock cycles.
 * The provided cycles is the raw counter value of the base hw clock
 * of sw clock time given as argument.
 * The _isr variant needs to be used when called in IRQ context.
 * \param id		clock id.
 * \param ns		time to convert in ns.
 * \param cycles	pointer to u64 variable that will hold the result.
 * \return		0 on success, or negative value on error.
 */
int clock_time_to_cycles(os_clock_id_t id, uint64_t ns, uint64_t *cycles);
int clock_time_to_cycles_isr(os_clock_id_t id, uint64_t ns, uint64_t *cycles);

hw_clock_id_t clock_to_hw_clock(os_clock_id_t clk_id);
bool clock_has_hw_adjust_ratio(os_clock_id_t clk_id);

/**
 * Initialize clock layer.
 * \return     0 on success or -1 on error.
 */
int os_clock_init(void);

/**
 * Close clock layer.
 * \return     none
 */
void os_clock_exit(void);

#endif /* _FREERTOS_CLOCK_H_ */
