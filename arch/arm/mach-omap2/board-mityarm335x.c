/*
 * Code for MityARM-335X SOM.  Heavily lifted from AM335X EVM code.
 *
 * Copyright (C) 2012 Critical Link, LLC - http://www.criticallink.com
 *
 * (original copyright from EVM code)
 * Copyright (C) 2011 Texas Instruments, Inc. - http://www.ti.com/
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation version 2.
 *
 * This program is distributed "as is" WITHOUT ANY WARRANTY of any
 * kind, whether express or implied; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/i2c/at24.h>
#include <linux/phy.h>
#include <linux/gpio.h>
#include <linux/spi/spi.h>
#include <linux/spi/flash.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/partitions.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/ethtool.h>
#include <linux/mtd/mtd.h>
#include <linux/mfd/tps65910.h>

#include <mach/hardware.h>

#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/hardware/asp.h>

#include <plat/irqs.h>
#include <plat/board.h>
#include <plat/usb.h>
#include <plat/mmc.h>
#include <plat/nand.h>
#include <plat/mcspi.h>

#include "common.h"
#include "board-flash.h"
#include "cpuidle33xx.h"
#include "mux.h"
#include "devices.h"

/* Convert GPIO signal to GPIO pin number */
#define GPIO_TO_PIN(bank, gpio) (32 * (bank) + (gpio))

/* module pin mux structure */
struct pinmux_config {
	const char *string_name; /* signal name format */
	int val; /* Options for the mux register value */
};

static struct omap_board_config_kernel mityarm335x_config[] __initdata = {
};

#define FACTORY_CONFIG_MAGIC    0x012C0138
#define FACTORY_CONFIG_VERSION  0x00010001

struct factory_config {
	u32	magic;
	u32	version;
	u8	mac[6];
	u32	reserved;
	u32	spare;
	u32	serialnumber;
	char	partnum[32];
};

static struct factory_config factory_config;

/* Pin mux for on board nand flash */
static struct pinmux_config nand_pin_mux[] = {
	{"gpmc_ad0.gpmc_ad0",	  AM33XX_PIN_INPUT_PULLUP},
	{"gpmc_ad1.gpmc_ad1",	  AM33XX_PIN_INPUT_PULLUP},
	{"gpmc_ad2.gpmc_ad2",	  AM33XX_PIN_INPUT_PULLUP},
	{"gpmc_ad3.gpmc_ad3",	  AM33XX_PIN_INPUT_PULLUP},
	{"gpmc_ad4.gpmc_ad4",	  AM33XX_PIN_INPUT_PULLUP},
	{"gpmc_ad5.gpmc_ad5",	  AM33XX_PIN_INPUT_PULLUP},
	{"gpmc_ad6.gpmc_ad6",	  AM33XX_PIN_INPUT_PULLUP},
	{"gpmc_ad7.gpmc_ad7",	  AM33XX_PIN_INPUT_PULLUP},
	{"gpmc_wait0.gpmc_wait0", AM33XX_PIN_INPUT_PULLUP},
	{"gpmc_wpn.gpmc_wpn",	  AM33XX_PIN_INPUT_PULLUP},
	{"gpmc_csn0.gpmc_csn0",	  AM33XX_PULL_DISA},
	{"gpmc_advn_ale.gpmc_advn_ale",  AM33XX_PULL_DISA},
	{"gpmc_oen_ren.gpmc_oen_ren",	 AM33XX_PULL_DISA},
	{"gpmc_wen.gpmc_wen",     AM33XX_PULL_DISA},
	{"gpmc_ben0_cle.gpmc_ben0_cle",	 AM33XX_PULL_DISA},
	{NULL, 0},
};

/* Module pin mux for SPI fash */
static struct pinmux_config spi1_pin_mux[] = {
	{"ecap0_in_pwm0_out.spi1_sclk",	AM33XX_PULL_ENBL | AM33XX_INPUT_EN },
	{"mcasp0_fsx.spi1_d0",		AM33XX_PULL_ENBL | AM33XX_PULL_UP |
					AM33XX_INPUT_EN },
	{"mcasp0_axr0.spi1_d1",		AM33XX_PULL_ENBL | AM33XX_INPUT_EN },
	{"mcasp0_ahclkr.spi1_cs0",	AM33XX_PULL_ENBL | AM33XX_PULL_UP |
					AM33XX_INPUT_EN },
	{NULL, 0},
};

/*
 * Module pin mux for I2C1, shares factory config PROM, port SCLSR_EN1/
 * SDASR_EN2 (pins 11/10, ID0) of TPS65910.  Also goes to edge connector.
 */
static struct pinmux_config i2c1_pin_mux[] = {
	{"mii1_crs.i2c1_sda",	AM33XX_SLEWCTRL_SLOW | AM33XX_PULL_ENBL |
				AM33XX_INPUT_EN | AM33XX_PIN_OUTPUT},
	{"mii1_rxerr.i2c1_scl",	AM33XX_SLEWCTRL_SLOW | AM33XX_PULL_ENBL |
				AM33XX_INPUT_EN | AM33XX_PIN_OUTPUT},
	{NULL, 0},
};

/*
 * Module pin mux for I2C2.  Connected to port SCL_SCK/SDA_SDI (pins 9/8, ID1)
 * of TPS65910.  Does not leave module.
 */
static struct pinmux_config i2c2_pin_mux[] = {
	{"uart1_ctsn.i2c2_sda",	AM33XX_SLEWCTRL_SLOW | AM33XX_PULL_ENBL |
				AM33XX_INPUT_EN | AM33XX_PIN_OUTPUT},
	{"uart1_rtsn.i2c2_scl",	AM33XX_SLEWCTRL_SLOW | AM33XX_PULL_ENBL |
				AM33XX_INPUT_EN | AM33XX_PIN_OUTPUT},
	{NULL, 0},
};

/*
* @pin_mux - single module pin-mux structure which defines pin-mux
*			details for all its pins.
*/
static void setup_pin_mux(struct pinmux_config *pin_mux)
{
	int i;

	for (i = 0; pin_mux->string_name != NULL; pin_mux++)
		omap_mux_init_signal(pin_mux->string_name, pin_mux->val);

}

/* NAND partition information */
static struct mtd_partition mityarm335x_nand_partitions[] = {
/* All the partition sizes are listed in terms of NAND block size */
	{
		.name           = "SPL",
		.offset         = 0,			/* Offset = 0x0 */
		.size           = SZ_128K,
	},
	{
		.name           = "SPL.backup1",
		.offset         = MTDPART_OFS_APPEND,	/* Offset = 0x20000 */
		.size           = SZ_128K,
	},
	{
		.name           = "SPL.backup2",
		.offset         = MTDPART_OFS_APPEND,	/* Offset = 0x40000 */
		.size           = SZ_128K,
	},
	{
		.name           = "SPL.backup3",
		.offset         = MTDPART_OFS_APPEND,	/* Offset = 0x60000 */
		.size           = SZ_128K,
	},
	{
		.name           = "U-Boot",
		.offset         = MTDPART_OFS_APPEND,   /* Offset = 0x80000 */
		.size           = 15 * SZ_128K,
	},
	{
		.name           = "U-Boot Env",
		.offset         = MTDPART_OFS_APPEND,   /* Offset = 0x260000 */
		.size           = 1 * SZ_128K,
	},
	{
		.name           = "Kernel",
		.offset         = MTDPART_OFS_APPEND,   /* Offset = 0x280000 */
		.size           = 40 * SZ_128K,
	},
	{
		.name           = "File System",
		.offset         = MTDPART_OFS_APPEND,   /* Offset = 0x780000 */
		.size           = MTDPART_SIZ_FULL,
	},
};

// TODO board-am335x has identical struct
static struct gpmc_timings am335x_nand_timings = {

	.sync_clk = 0,

	.cs_on = 0,
	.cs_rd_off = 44,
	.cs_wr_off = 44,

	.adv_on = 6,
	.adv_rd_off = 34,
	.adv_wr_off = 44,

	.we_off = 40,
	.oe_off = 54,

	.access = 64,
	.rd_cycle = 82,
	.wr_cycle = 82,

	.wr_access = 40,
	.wr_data_mux_bus = 0,
};

static void mityarm335x_nand_init(void)
{
	struct omap_nand_platform_data *pdata;
	struct gpmc_devices_info gpmc_device[2] = {
		{ NULL, 0 },
		{ NULL, 0 },
	};

	setup_pin_mux(nand_pin_mux);
	pdata = omap_nand_init(mityarm335x_nand_partitions,
		ARRAY_SIZE(mityarm335x_nand_partitions), 0, 0,
		&am335x_nand_timings);
	if (!pdata)
		return;
	pdata->ecc_opt =OMAP_ECC_BCH8_CODE_HW;
	pdata->elm_used = true;
	gpmc_device[0].pdata = pdata;
	gpmc_device[0].flag = GPMC_DEVICE_NAND;

	omap_init_gpmc(gpmc_device, sizeof(gpmc_device));
	omap_init_elm();
}

/* SPI flash information (reserved for user applications) */
static struct mtd_partition mityarm335x_spi_partitions[] = {
	{
		.name       = "NOR User Defined",
		.offset     = 0,
		.size       = MTDPART_SIZ_FULL,
	}
};

static const struct flash_platform_data mityarm335x_spi_flash = {
	.name      = "spi_flash",
	.parts     = mityarm335x_spi_partitions,
	.nr_parts  = ARRAY_SIZE(mityarm335x_spi_partitions),
	.type      = "m25p64-nonjedec",
};

static const struct omap2_mcspi_device_config spi1_ctlr_data = {
	.turbo_mode = 0,
	.d0_is_mosi = 1,
};

/*
 * On board SPI NOR FLASH
 */
static struct spi_board_info mityarm335x_spi1_slave_info[] = {
	{
		.modalias		= "m25p80",
		.platform_data		= &mityarm335x_spi_flash,
		.controller_data	= (void*)&spi1_ctlr_data,
		.irq			= -1,
		.max_speed_hz		= 30000000,
		.bus_num		= 2,
		.chip_select		= 0,
		.mode			= SPI_MODE_3,
	},
};

/* setup spi1 */
static void spi1_init(void)
{
	setup_pin_mux(spi1_pin_mux);
	spi_register_board_info(mityarm335x_spi1_slave_info,
			ARRAY_SIZE(mityarm335x_spi1_slave_info));
	return;
}

static struct regulator_init_data am335x_dummy;

static struct regulator_consumer_supply am335x_vdd1_supply[] = {
	REGULATOR_SUPPLY("mpu", "mpu.0"),
};

static struct regulator_init_data am335x_vdd1 = {
	.constraints = {
		.min_uV			= 600000,
		.max_uV			= 1500000,
		.valid_modes_mask	= REGULATOR_MODE_NORMAL,
		.valid_ops_mask		= REGULATOR_CHANGE_VOLTAGE,
		.always_on		= 1,
	},
	.num_consumer_supplies	= ARRAY_SIZE(am335x_vdd1_supply),
	.consumer_supplies	= am335x_vdd1_supply,
};

static struct tps65910_board mityarm335x_tps65910_info = {
	.tps65910_pmic_init_data[TPS65910_REG_VRTC]	= &am335x_dummy,
	.tps65910_pmic_init_data[TPS65910_REG_VIO]	= &am335x_dummy,
	.tps65910_pmic_init_data[TPS65910_REG_VDD1]	= &am335x_vdd1,
	.tps65910_pmic_init_data[TPS65910_REG_VDD2]	= &am335x_dummy,
	.tps65910_pmic_init_data[TPS65910_REG_VDD3]	= &am335x_dummy,
	.tps65910_pmic_init_data[TPS65910_REG_VDIG1]	= &am335x_dummy,
	.tps65910_pmic_init_data[TPS65910_REG_VDIG2]	= &am335x_dummy,
	.tps65910_pmic_init_data[TPS65910_REG_VPLL]	= &am335x_dummy,
	.tps65910_pmic_init_data[TPS65910_REG_VDAC]	= &am335x_dummy,
	.tps65910_pmic_init_data[TPS65910_REG_VAUX1]	= &am335x_dummy,
	.tps65910_pmic_init_data[TPS65910_REG_VAUX2]	= &am335x_dummy,
	.tps65910_pmic_init_data[TPS65910_REG_VAUX33]	= &am335x_dummy,
	.tps65910_pmic_init_data[TPS65910_REG_VMMC]	= &am335x_dummy,
};

static struct i2c_board_info __initdata mityarm335x_i2c2_boardinfo[] = {
	{
		I2C_BOARD_INFO("tps65910", TPS65910_I2C_ID1),
		.platform_data  = &mityarm335x_tps65910_info,
	},
};

static void read_factory_config(struct memory_accessor *a, void* context)
{
	int ret;
	const char *partnum = NULL;

	ret = a->read(a, (char *)&factory_config, 0, sizeof(factory_config));
	if (ret != sizeof(struct factory_config)) {
		pr_warning("MityARM-335x: Read Factory Config Failed: %d\n",
			ret);
		goto bad_config;
	}

	if (factory_config.magic != FACTORY_CONFIG_MAGIC) {
		pr_warning("MityARM-335x: Factory Config Magic Wrong (%X)\n",
			factory_config.magic);
		goto bad_config;
	}

	if (factory_config.version != FACTORY_CONFIG_VERSION) {
		pr_warning("MityARM-335x: Factory Config Version Wrong (%X)\n",
			factory_config.version);
		goto bad_config;
	}

	pr_info("MityARM-335x: Found MAC = %pM\n", factory_config.mac);

	am33xx_cpsw_macidfillup(factory_config.mac, factory_config.mac);

	partnum = factory_config.partnum;
	pr_info("MityARM-335x: Part Number = %s\n", partnum);

bad_config:

	/* turn on the switch regardless */
	am33xx_cpsw_init(1);
}

static struct at24_platform_data mityarm335x_fd_info = {
	.byte_len	= 256,
	.page_size	= 8,
	.flags		= AT24_FLAG_READONLY | AT24_FLAG_IRUGO,
	.setup		= read_factory_config,
	.context	= NULL,
};

static struct i2c_board_info __initdata mityarm335x_i2c1_boardinfo[] = {
	{
		I2C_BOARD_INFO("24c02", 0x50),
		.platform_data  = &mityarm335x_fd_info,
	},
};

static void __init mityarm335x_i2c_init(void)
{
	setup_pin_mux(i2c2_pin_mux);
	omap_register_i2c_bus(3, 100, mityarm335x_i2c2_boardinfo,
				ARRAY_SIZE(mityarm335x_i2c2_boardinfo));
	setup_pin_mux(i2c1_pin_mux);
	omap_register_i2c_bus(2, 100, mityarm335x_i2c1_boardinfo,
				ARRAY_SIZE(mityarm335x_i2c1_boardinfo));
}

static struct resource am335x_rtc_resources[] = {
	{
		.start		= AM33XX_RTC_BASE,
		.end		= AM33XX_RTC_BASE + SZ_4K - 1,
		.flags		= IORESOURCE_MEM,
	},
	{ /* timer irq */
		.start		= AM33XX_IRQ_RTC_TIMER,
		.end		= AM33XX_IRQ_RTC_TIMER,
		.flags		= IORESOURCE_IRQ,
	},
	{ /* alarm irq */
		.start		= AM33XX_IRQ_RTC_ALARM,
		.end		= AM33XX_IRQ_RTC_ALARM,
		.flags		= IORESOURCE_IRQ,
	},
};

static struct platform_device am335x_rtc_device = {
	.name           = "omap_rtc",
	.id             = -1,
	.num_resources	= ARRAY_SIZE(am335x_rtc_resources),
	.resource	= am335x_rtc_resources,
};

static int am335x_rtc_init(void)
{
	void __iomem *base;
	struct clk *clk;

	clk = clk_get(NULL, "rtc_fck");
	if (IS_ERR(clk)) {
		pr_err("rtc : Failed to get RTC clock\n");
		return -1;
	}

	if (clk_enable(clk)) {
		pr_err("rtc: Clock Enable Failed\n");
		return -1;
	}

	base = ioremap(AM33XX_RTC_BASE, SZ_4K);

	if (WARN_ON(!base))
		return -ENOMEM;

	/* Unlock the rtc's registers */
	__raw_writel(0x83e70b13, base + 0x6c);
	__raw_writel(0x95a4f1e0, base + 0x70);

	/*
	 * Enable the 32K OSc
	 * TODO: Need a better way to handle this
	 * Since we want the clock to be running before mmc init
	 * we need to do it before the rtc probe happens
	 */
	__raw_writel(0x48, base + 0x54);

	iounmap(base);

	return  platform_device_register(&am335x_rtc_device);
}

extern void __iomem * __init am33xx_get_mem_ctlr(void);

static struct resource am33xx_cpuidle_resources[] = {
	{
		.start		= AM33XX_EMIF0_BASE,
		.end		= AM33XX_EMIF0_BASE + SZ_32K - 1,
		.flags		= IORESOURCE_MEM,
	},
};

/* AM33XX devices support DDR2 power down */
static struct am33xx_cpuidle_config am33xx_cpuidle_pdata = {
	.ddr2_pdown	= 1,
};

static struct platform_device am33xx_cpuidle_device = {
	.name			= "cpuidle-am33xx",
	.num_resources		= ARRAY_SIZE(am33xx_cpuidle_resources),
	.resource		= am33xx_cpuidle_resources,
	.dev = {
		.platform_data	= &am33xx_cpuidle_pdata,
	},
};

static void __init am33xx_cpuidle_init(void)
{
	int ret;

	am33xx_cpuidle_pdata.emif_base = am33xx_get_mem_ctlr();

	ret = platform_device_register(&am33xx_cpuidle_device);

	if (ret)
		pr_warning("AM33XX cpuidle registration failed\n");

}

static void __init mityarm335x_init(void)
{
	am33xx_cpuidle_init();
	am33xx_mux_init(NULL);
	omap_serial_init();
	am335x_rtc_init();
	mityarm335x_i2c_init();
	omap_sdrc_init(NULL, NULL);
	spi1_init();
	mityarm335x_nand_init();
	omap_board_config = mityarm335x_config;
	omap_board_config_size = ARRAY_SIZE(mityarm335x_config);
	/* Create an alias for icss clock */
	if (clk_add_alias("pruss", NULL, "icss_uart_gclk", NULL))
		pr_err("failed to create an alias: icss_uart_gclk --> pruss\n");
	/* Create an alias for gfx/sgx clock */
	if (clk_add_alias("sgx_ck", NULL, "gfx_fclk", NULL))
		pr_err("failed to create an alias: gfx_fclk --> sgx_ck\n");
}

static void __init mityarm335x_map_io(void)
{
	omap2_set_globals_am33xx();
	omapam33xx_map_common_io();
}

MACHINE_START(MITYARM335X, "mityarm335x")
	/* Maintainer: Critical Link, LLC */
	.atag_offset	= 0x100,
	.map_io		= mityarm335x_map_io,
	.init_irq	= ti81xx_init_irq,
	.init_early	= am33xx_init_early,
	.timer		= &omap3_am33xx_timer,
	.init_machine	= mityarm335x_init,
MACHINE_END

