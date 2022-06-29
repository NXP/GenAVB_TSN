/*
 * MLE 145170 driver
 * Copyright 2016 Freescale Semiconductor, Inc.
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
#include <linux/spi/spi.h>
#include <linux/module.h>
#include <linux/clk.h>

#include "media_clock_drv.h"


/* C register Figure 14 */
#define	C_BIT(x)	(1 << x)

#define MLE145170_ID	0x0

#define DEFAULT_RATIO	40960
#define MIN_RATIO	40
#define MAX_RATIO	65535

struct mle145170_priv {
	struct mclock_ext_dev ext_dev;
	struct clk *clk_in;
};

static int mle145170_set_ratio(struct spi_device *spi, u16 ratio)
{
	u8 C_reg;
	u16 N_reg;
	int rc = 0;

	/* Set C register to enable fv output*/
	C_reg = C_BIT(1);
	rc = spi_write(spi, &C_reg, sizeof(C_reg));
	if (rc < 0) {
		pr_err("spi_write error: %d\n", rc);
		goto err;
	}

	/* Set N register to divide ratio */
	N_reg = htons(ratio);

	pr_debug("divider set to %u\n", ratio);

	rc = spi_write(spi, &N_reg, sizeof(N_reg));
	if (rc < 0) {
		pr_err("spi_write error: %d\n", rc);
		goto err;
	}

err:
	return rc;
}

static int mle145170_set_clk_out_freq(struct device *dev, unsigned int freq_p, unsigned int freq_q)
{
	struct spi_device *spi = container_of(dev, struct spi_device, dev);
	struct mle145170_priv *mle145170 = spi_get_drvdata(spi);
	unsigned int clk_out_rate = clk_get_rate(mle145170->clk_in);
	unsigned int ratio;

	ratio = (clk_out_rate / freq_p) * freq_q;

	if (((ratio / freq_q) * freq_p) != clk_out_rate)
		return -EINVAL;

	if ((ratio < MIN_RATIO) || (ratio > MAX_RATIO))
		return -EINVAL;

	return mle145170_set_ratio(spi, (u16)ratio);
}

static int mle145170_probe(struct spi_device *spi)
{
	struct mle145170_priv *mle145170;
	int rc = 0;

	mle145170 = devm_kzalloc(&spi->dev, sizeof(*mle145170), GFP_KERNEL);
	if (!mle145170) {
		rc = -ENOMEM;
		goto err;
	}

	mle145170->clk_in = devm_clk_get(&spi->dev, "clk_in");
	if (IS_ERR(mle145170->clk_in)) {
		rc = PTR_ERR(mle145170->clk_in);
		goto err;
	}
	spi_set_drvdata(spi, mle145170);

	rc = mle145170_set_ratio(spi, DEFAULT_RATIO);
	if (rc < 0)
		goto err;

	mle145170->ext_dev.dev = &spi->dev;
	mle145170->ext_dev.configure = mle145170_set_clk_out_freq;
	mclock_drv_register_ext_device(&mle145170->ext_dev);

	pr_info("MLE 145170, default ratio: %d\n", DEFAULT_RATIO);

err:
	return rc;
}

static int mle145170_remove(struct spi_device *spi)
{
	struct mle145170_priv *mle145170 = spi_get_drvdata(spi);

	mclock_drv_unregister_ext_device(&mle145170->ext_dev);

	pr_info("MLE 145170\n");

	return 0;
}


static struct spi_device_id mle145170_idtable[] = {
	{ "mle145170", MLE145170_ID },
	{ },
};

MODULE_DEVICE_TABLE(spi, mle145170_idtable);

static const struct of_device_id mle145170_dt_ids[] = {
	{ .compatible = "lansdale,mle145170", },
	{ },
};

static struct spi_driver mle145170_driver = {
	.driver = {
		.name = "mle145170",
		.owner = THIS_MODULE,
		.of_match_table = mle145170_dt_ids,
	},

	.id_table = mle145170_idtable,
	.probe = mle145170_probe,
	.remove = mle145170_remove,
};

int mle145170_init(void)
{
	pr_info("%s\n", __func__);

	return spi_register_driver(&mle145170_driver);
}

void mle145170_exit(void)
{
	spi_unregister_driver(&mle145170_driver);
}
