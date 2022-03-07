/*
 * CS2000 driver
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

#include <linux/slab.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/clk.h>
#include <asm/div64.h>

#include "media_clock_drv.h"

/**
 * DOC: CS2000
 *
 *
 */

#define DEVICE_ID		0x1
#define DEVICE_CTRL		0x2
#define DEVICE_CFG1		0x3
#define DEVICE_CFG2		0x4
#define DEVICE_GLOBAL_CFG	0x5

#define DEVICE_RATIO0		0x6
#define DEVICE_RATIO1		0xa
#define DEVICE_RATIO2		0xe
#define DEVICE_RATIO3		0x12

#define DEVICE_FUNC_CFG1	0x16
#define DEVICE_FUNC_CFG2	0x17
#define DEVICE_FUNC_CFG3	0x1e

/* Device Control */
#define CLKOUTDIS(x)	(((x) & 0x1) << 0)
#define AUXOUTDIS(x)	(((x) & 0x1) << 1)
#define UNLOCK		(1 << 7)

/* Device Configuration 1 */
#define RMODSEL(x)	(((x) & 0x7) << 5)
#define RSEL(x)		(((x) & 0x3) << 3)
#define AUXOUTSRC(x)	(((x) & 0x3) << 1)
#define ENDEVCFG1	(1 << 0)

/* Device Configuration 2 */
#define LOCKCLK(x)	(((x) & 0x3) << 1)
#define FRACNSRC(x)	(((x) & 0x1) << 0)

/* Global Configuration */
#define FREEZE(x)	(((x) & 0x1) << 3)
#define ENDEVCFG2	(1 << 0)

/* Function Configuration 1 */
#define CLKSKIPEN(x)	(((x) & 0x1) << 7)
#define AUXLOCKCFG(x)	(((x) & 0x1) << 6)
#define REFCLKDIV(x)	(((x) & 0x3) << 3)

/* Function Configuration 2 */
#define CLKOUTUNL(x)	(((x) & 0x1) << 4)
#define LFRATIOCFG(x)	(((x) & 0x1) << 3)

/* Function Configuration 3 */
#define CLKINBW(x)	(((x) & 0x7) << 4)


#define CS2000_ID	0x0

#define MAX_HIGH_RES_RATIO	(1 << 12)
#define MAX_HIGH_MULT_RATIO	(1 << 20)

struct cs2000_priv {
	struct mclock_ext_dev ext_dev;
	struct clk *ref_clk;
	struct clk *clk_out;
};

static int cs2000_reg_write(struct i2c_client *client, u8 reg, u8 value)
{
	return i2c_smbus_write_byte_data(client, reg, value);
}

static u8 cs2000_reg_read(struct i2c_client *client, u8 reg)
{
	return i2c_smbus_read_byte_data(client, reg);
}

/*
 * Writes ratio in 12.20 or 20.12 format
 */
static void cs2000_write_ratio(struct i2c_client *client, int ratio_index, u32 ratio)
{
	cs2000_reg_write(client, ratio_index++, (ratio >> 24) & 0xff);
	cs2000_reg_write(client, ratio_index++, (ratio >> 16) & 0xff);
	cs2000_reg_write(client, ratio_index++, (ratio >> 8) & 0xff);
	cs2000_reg_write(client, ratio_index, (ratio >> 0) & 0xff);
}

static int cs2000_set_ratio(struct i2c_client *client, int ratio_index, unsigned int in_rate_p, unsigned int in_rate_q,
					unsigned int out_rate, int dynamic)
{
	u64 ratio = (u64)out_rate * in_rate_q;
	int rc = 0;

	/*
	 * First, a regular base 10 ratio is computed for sanity checks and cs2000 mode selection.
	 * Then it's computed in 12.20 or 20.12 format.
	 */

	do_div(ratio, in_rate_p);

	if (ratio >= MAX_HIGH_RES_RATIO) {

		if (!dynamic) {
			pr_err("%s: invalid static ratio: %u\n", __func__, (unsigned int)ratio);
			rc = -1;
			goto out;
		}

		if (ratio >= MAX_HIGH_MULT_RATIO) {
			pr_err("%s: ratio too high: %u\n", __func__, (unsigned int)ratio);
			rc = -1;
			goto out;
		}

		ratio = ((u64)out_rate) << 12;

		/* In dynamic mode (clock recovery) the 12.20 ratio needs to be an integer
		 * it is not allowed to drift from the input ts clock
		 */
		if (do_div(ratio, in_rate_p) && dynamic) {
			pr_err("%s: ratio is not perfect, clk_in: %u / %u, clk_out: %u\n",
				 __func__, in_rate_p, in_rate_q, out_rate);
			rc = -1;
			goto out;
		}

		ratio *= in_rate_q;

		if (dynamic)
			/* ClkOutUnl = 1 (enable clock ouput when unlocked) */
			/* LfRatioCfg = 0 (High Multiplier, 20.12) */
			cs2000_reg_write(client, DEVICE_FUNC_CFG2, CLKOUTUNL(1) | LFRATIOCFG(0));
	}
	else {
		ratio = ((u64)out_rate) << 20;

		/* In dynamic mode (clock recovery) the 20.12 ratio needs to be an integer
		 * it is not allowed to drift from the input ts clock
		 */
		if (do_div(ratio, in_rate_p) && dynamic) {
			pr_err("%s: ratio is not perfect, clk_in: %u / %u, clk_out: %u\n",
				 __func__, in_rate_p, in_rate_q, out_rate);
			rc = -1;
			goto out;

		}

		ratio *= in_rate_q;

		if (!ratio) {
			pr_err("%s: ratio null\n", __func__);
			rc = -1;
			goto out;
		}

		if (dynamic)
			/* ClkOutUnl = 1 (enable clock ouput when unlocked) */
			/* LfRatioCfg = 1 (High Precision, 12.20) */
			cs2000_reg_write(client, DEVICE_FUNC_CFG2, CLKOUTUNL(1) | LFRATIOCFG(1));
	}

	cs2000_write_ratio(client, ratio_index, ratio);

out:
	return rc;
}

static int cs2000_init_config(struct i2c_client *client, struct cs2000_priv *cs2000)
{
	unsigned int ref_clk_rate = clk_get_rate(cs2000->ref_clk);
	unsigned int clk_out_rate = clk_get_rate(cs2000->clk_out);
	int count = 0;

	pr_info("%s: ref_clk: %u,  clk_out: %u\n", __func__,
			ref_clk_rate, clk_out_rate);

	/*
	 * Initial configuration and static ratio configuration
	 */

	/* Set freeze */
	/* EnDevCfg2 = 1 (enabled) */
	cs2000_reg_write(client, DEVICE_GLOBAL_CFG, FREEZE(1) | ENDEVCFG2);

	/* ClkOutDis = 0 (enabled) */
	/* AuxOutDis = 0 (enabled) */
	cs2000_reg_write(client, DEVICE_CTRL, AUXOUTDIS(0) | CLKOUTDIS(0));

	/* RModSel = 0 (x1) */
	/* Rsel = 0 (Ratio0 1.0) */
	/* AuxOutSrc = 11 (PLL lock status indicator) */
	/* EnDevCfg1 = 1 (enabled) */
	cs2000_reg_write(client, DEVICE_CFG1, RMODSEL(0) | RSEL(0) | AUXOUTSRC(3) | ENDEVCFG1);

	/* LockClk = 1 (Ratio1 4096.0) */
	/* FracNsrc = x */
	cs2000_reg_write(client, DEVICE_CFG2, LOCKCLK(1));

	/* ClkSkipEn = 0 (clock skipping mode disabled) */
	/* AuxLockCfg = 1 (Open-drain, Active low) */
	/* RefClkDiv = 01 (/ 2) */
	cs2000_reg_write(client, DEVICE_FUNC_CFG1, CLKSKIPEN(0) | AUXLOCKCFG(1) | REFCLKDIV(1));

	/* ClkInBw = 0 (1 Hz) */ /* FIXME what is the best value ? */
	cs2000_reg_write(client, DEVICE_FUNC_CFG3, CLKINBW(0));

	/* Static Ratio 0 */
	if (cs2000_set_ratio(client, DEVICE_RATIO0, ref_clk_rate, 1, clk_out_rate, 0) < 0)
		goto err_unfreeze;

	/* Remove freeze */
	cs2000_reg_write(client, DEVICE_GLOBAL_CFG, FREEZE(0) | ENDEVCFG2);

	while (cs2000_reg_read(client, DEVICE_CTRL) & UNLOCK) {
		udelay(10);

		if (count++ >= (USEC_PER_SEC / 10)) {
			pr_err("%s: cs2000 PLL lock timeout...\n", __func__);
			goto err;
		}
	}

	pr_info("%s: cs2000 PLL locked\n", __func__);

	return 0;

err_unfreeze:
	/* Remove freeze */
	cs2000_reg_write(client, DEVICE_GLOBAL_CFG, FREEZE(0) | ENDEVCFG2);
err:
	return -1;
}

static int cs2000_set_clk_in_rate(struct device *dev, unsigned int clk_in_rate_p, unsigned int clk_in_rate_q)
{
	struct i2c_client *client = container_of(dev, struct i2c_client, dev);
	struct cs2000_priv *cs2000 = i2c_get_clientdata(client);
	unsigned int clk_out_rate = clk_get_rate(cs2000->clk_out);
	int rc = 0;

	pr_debug("%s: clk_out: %u,  clk_in: %u/%u \n", __func__, clk_out_rate, clk_in_rate_p, clk_in_rate_q);

	/* Set freeze */
	cs2000_reg_write(client, DEVICE_GLOBAL_CFG, FREEZE(1) | ENDEVCFG2);

	rc = cs2000_set_ratio(client, DEVICE_RATIO1, clk_in_rate_p, clk_in_rate_q, clk_out_rate, 1);

	/* Remove freeze */
	cs2000_reg_write(client, DEVICE_GLOBAL_CFG, FREEZE(0) | ENDEVCFG2);

	return rc;
}

static int cs2000_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct cs2000_priv *cs2000;
	u8 val = cs2000_reg_read(client, DEVICE_ID);
	int rc = 0;

	cs2000 = devm_kzalloc(&client->dev, sizeof(*cs2000), GFP_KERNEL);
	if (!cs2000) {
		rc = -ENOMEM;
		goto err;
	}

	cs2000->ref_clk = devm_clk_get(&client->dev, "ref_clk");
	if (IS_ERR(cs2000->ref_clk)) {
		rc = PTR_ERR(cs2000->ref_clk);
		goto err;
	}

	cs2000->clk_out = devm_clk_get(&client->dev, "clk_out");
	if (IS_ERR(cs2000->clk_out)) {
		rc = PTR_ERR(cs2000->clk_out);
		goto err;
	}

	i2c_set_clientdata(client, cs2000);

	rc = cs2000_init_config(client, cs2000);
	if (rc < 0)
		goto err;

	cs2000->ext_dev.dev = &client->dev;
	cs2000->ext_dev.configure = cs2000_set_clk_in_rate;
	mclock_drv_register_ext_device(&cs2000->ext_dev);

	pr_info("cs2000 id: %x, revision: %x\n", val >> 3, val & 0x7);

err:
	return rc;
}

static int cs2000_remove(struct i2c_client *client)
{
	/* Only cleanup software resources, hardware keeps running */

	struct cs2000_priv *cs2000 = i2c_get_clientdata(client);

	mclock_drv_unregister_ext_device(&cs2000->ext_dev);

	return 0;
}

static struct i2c_device_id cs2000_idtable[] = {
	{ "cs2000", CS2000_ID },
        { },
};

MODULE_DEVICE_TABLE(i2c, cs2000_idtable);

static const struct of_device_id cs2000_dt_ids[] = {
	{ .compatible = "cirrus,cs2000", },
	{ },
};

static struct i2c_driver cs2000_driver = {
	.driver = {
		.name = "cs2000",
		.owner = THIS_MODULE,
                .of_match_table = cs2000_dt_ids,
	},

	.id_table = cs2000_idtable,
	.probe = cs2000_probe,
	.remove = cs2000_remove,
};


int cs2000_init(void)
{
	pr_info("%s\n", __func__);

	return i2c_add_driver(&cs2000_driver);
}


void cs2000_exit(void)
{
	i2c_del_driver(&cs2000_driver);
}
