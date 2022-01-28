/*
* Copyright 2015 Freescale Semiconductor, Inc.
* Copyright 2020 NXP
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
 @brief Generic filtering functions
 @details
*/

#ifndef _COMMON_FILTER_H_
#define _COMMON_FILTER_H_

#include "common/types.h"

union filter_params {
	struct {
		double *array;
		double mean;
		u32 array_size;
		u32 position;
		u32 count;
	} mean;

	struct {
		double mean;
		double coef;
		int m_factor;
		int count;
	} exp_decay;

	struct {
		double *array;
		u32 array_size;
		u32 position;
		u32 count;
		double *taps_array;
		double taps_sum;
	} fir;
};


struct filter {
	void (*reset)(union filter_params *params);
	void (*close)(union filter_params *params);
	double (*filter)(union filter_params *params, double val);

	union filter_params params;
};


/** Call filter reset function.
 * @f: pointer to the filter to reset
 *
 */
static inline void filter_reset(struct filter *f)
{
	if (f->reset)
		f->reset(&f->params);
}

/** Call filter close function.
 * @f: pointer to the filter to close
 *
 */
static inline void filter_close(struct filter *f)
{
	if (f->close)
		f->close(&f->params);
}

/** Call filter processing function.
 * @f: pointer to the filter to process
 * @val: sampling value to filter
 *
 */
static inline double filter(struct filter *f, double val)
{
	return f->filter(&f->params, val);
}

int filter_identity_init(struct filter *f);
int filter_mean_init(struct filter *f, u32 size);
int filter_exp_decay_init(struct filter *f, double coef);
int filter_fir_init(struct filter *f, u32 size, double *filter_taps);

#endif /* _COMMON_FILTER_H_ */
