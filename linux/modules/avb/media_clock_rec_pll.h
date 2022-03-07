/*
 * AVB media clock recovery
 * Copyright 2014-2015 Freescale Semiconductor, Inc.
 * Copyright 2018 NXP
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
#ifndef _MEDIA_CLOCK_REC_PLL_
#define _MEDIA_CLOCK_REC_PLL_

#include "media_clock.h"
#include "pi.h"
#include "imx-pll.h"

#define MCLOCK_PLL_REC_TRACE   0

typedef enum {
	RESET,
	START,
	ADJUST,
} rec_pll_state_t;

struct mclock_rec_pll_stats {
	unsigned int err_meas;
	unsigned int err_port_down;
	unsigned int err_fec;
	unsigned int err_wd;
	unsigned int err_ts;
	unsigned int err_drift;
	unsigned int err_set_pll_rate;
	unsigned int err_pll_prec;
	unsigned int reset;
	unsigned int start;
	unsigned int stop;
	unsigned int adjust;
	int last_app_adjust;
	int err_cum;
	int err_per_sec;
	unsigned int err_time;
};

#if MCLOCK_PLL_REC_TRACE

#define MCLOCK_REC_TRACE_SIZE  512

struct mclock_rec_trace {
	unsigned int meas;
	unsigned int ticks;
	unsigned int state;
	int err;
	int adjust_value;
	int pi_control_output;
	int pi_err_input;
	unsigned long new_rate;
	unsigned long previous_rate;
};

#endif


struct mclock_rec_pll {
	struct mclock_dev dev;
	struct imx_pll pll;
	rec_pll_state_t state;
	unsigned int fec_tc_id;
	struct rational fec_next_ts;
	struct rational fec_period;
	unsigned int fec_nb_meas;
	struct rational pll_clk_target;
	struct rational pll_clk_period; //per FEC sampling interval
	unsigned int pll_clk_meas;
	unsigned int pll_ref_freq; //PLL frequency at timer input clk
	struct rational clk_media;
	unsigned int div;
	unsigned int r_idx;
	unsigned int ts_slot;
	unsigned int ts_offset;
	atomic_t ts_read;
	atomic_t status;
	struct pi pi;
	unsigned int factor;
	int max_adjust;
	int meas;
	int ppb_adjust; /* Exact ppb adjust returned from the PLL control layer*/
	int req_ppb_adjust; /* Requested pbb adjust passed to the PLL control layer*/
	unsigned int zero_err_nb; /* Number of measurements with zero error since last detected error */
	unsigned int wd;
#if MCLOCK_PLL_REC_TRACE
	unsigned int trace_count;
	unsigned int trace_freeze;
	struct mclock_rec_trace trace[MCLOCK_REC_TRACE_SIZE];
#endif
	struct mclock_rec_pll_stats stats;
};

int  mclock_rec_pll_init(struct mclock_rec_pll *rec);
void mclock_rec_pll_exit(struct mclock_rec_pll *rec);
int  mclock_rec_pll_timer_irq(struct mclock_rec_pll *rec, int fec_event, unsigned int meas, unsigned int ticks);
void mclock_rec_pll_reset(struct mclock_rec_pll *rec);
int  mclock_rec_pll_ready(struct mclock_rec_pll *rec);
int  mclock_rec_pll_stop(struct mclock_rec_pll *rec);
int  mclock_rec_pll_start(struct mclock_rec_pll *rec, struct mclock_start *start);
int  mclock_rec_pll_config(struct mclock_dev *dev, struct mclock_sconfig *cfg);
int  mclock_rec_pll_clean_get(struct mclock_dev *dev, struct mclock_clean *clean);

static inline void mclock_rec_pll_wd_reset(struct mclock_rec_pll *rec)
{
	rec->wd = rec->dev.clk_timer + 4 * rec->fec_period.i;
}

static inline void mclock_rec_pll_inc_r_idx(struct mclock_rec_pll *rec)
{
	rec->r_idx = (rec->r_idx + 1) & (rec->dev.num_ts - 1);
}

#define MCLOCK_REC_NUM_TS		256	//Needs to be power of 2. Cannot be increased without kernel SDMA driver change (max 682 BD)
#define MCLOCK_REC_BUF_SIZE		(MCLOCK_REC_NUM_TS * sizeof(unsigned int))
#define MCLOCK_REC_MMAP_SIZE		(MCLOCK_REC_BUF_SIZE + sizeof(unsigned int)) //w_idx
#define MCLOCK_REC_DMA_SIZE		(MCLOCK_REC_MMAP_SIZE + sizeof(unsigned int)) //One more to hold tmode value
#define MCLOCK_REC_TS_FREQ_INIT		6000

#define MCLOCK_REC_PLL_NB_MEAS		10
#define MCLOCK_REC_PLL_LOCKED_ERR_NB	3 /*Number of consecutive zero measurement errors before we declare the recovery locked*/

/* Default sampling frequency in internal mode and target for external TS */
#define MCLOCK_PLL_SAMPLING_FREQ 	100
#define MCLOCK_PLL_SAMPLING_PERIOD_MS	(1000 / MCLOCK_PLL_SAMPLING_FREQ)
#define MCLOCK_PLL_SAMPLING_PERIOD_NS	(MCLOCK_PLL_SAMPLING_PERIOD_MS * 1000 * 1000)

#endif /* _MEDIA_CLOCK_REC_PLL_ */

