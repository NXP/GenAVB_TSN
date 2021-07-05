/*
 * AVB dma driver
 * Copyright 2014-2015 Freescale Semiconductor, Inc.
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

#ifndef _DMA_DRV_H_
#define _DMA_DRV_H_

#ifdef __KERNEL__

#include <linux/types.h>
#include <linux/platform_device.h>

#include "media_clock.h"
#include "media_clock_rec_pll.h"

typedef enum {
	VF610_DMA,
	IMX6QDL_DMA,
} mclock_dma_devtype;

struct mclock_dma_stats {
	unsigned int init;
	unsigned int start;
	unsigned int stop;
	unsigned int dma_clean;
	unsigned int ts_freq_p;
	unsigned int ts_freq_q;
	unsigned int err_drift;
	unsigned int err_init;
};

struct mclock_dma {
	struct mclock_dev dev;
	struct dma_chan *chan;
	unsigned int eth_port;
	void *tmode_addr;
	dma_addr_t tmode_addr_p;
	u32 tmode;
	unsigned int dma_rem;
	void __iomem *tccr_addr;
	void __iomem *tcsr_addr;

	struct rational clk_media;
	unsigned int next_clean;
	unsigned int clean_period;
	unsigned int nb_clean_total;
	atomic_t nb_clean;
	atomic_t rec_status;

	unsigned int total_clean;

	struct mclock_ext_dev *ext_dev;

	struct mclock_dma_stats stats;
};

/* One instance per domain */
struct mclock_dma_drv
{
	int domain;
	void __iomem *enet;
	u32 enet_p;
	mclock_dma_devtype dev_type;

	struct mclock_dma rec;
	struct mclock_dma gen;
};


#define DMADRV_READY			(1 << 0)


#define FEC_ENET_TCSR(base, i) (base + 0x608 + (8 * i))
#define FEC_ENET_TCCR(base, i) (base + 0x60C + (8 * i))

static inline void dmadrv_w_idx_update(struct mclock_dev *dev, int nb_clean)
{
	if (dev->type == GEN) {
		*dev->count = *dev->count + nb_clean;
		*dev->w_idx = (*dev->w_idx + nb_clean) & (dev->num_ts - 1);
	}
}

int dmadrv_init(struct platform_driver *);
void dmadrv_exit(struct platform_driver *);

#endif /* __KERNEL__ */

#endif /* _DMA_DRV_H_ */
