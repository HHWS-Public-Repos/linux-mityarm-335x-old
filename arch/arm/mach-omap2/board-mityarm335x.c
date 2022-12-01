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
#include <linux/export.h>
#include <linux/string.h>
#include <linux/ethtool.h>
#include <linux/mtd/mtd.h>
#include <linux/mfd/tps65910.h>
#include <linux/wl12xx.h>
#include <linux/rtc/rtc-omap.h>

#include <linux/seq_file.h>
#include <linux/debugfs.h>
#include <linux/uaccess.h>

#include <mach/hardware.h>

#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/hardware/asp.h>

#include <plat/omap_device.h>
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
#include "mityarm335x.h"

/* Convert GPIO signal to GPIO pin number */
#define GPIO_TO_PIN(bank, gpio) (32 * (bank) + (gpio))

/* module pin mux structure */
struct pinmux_config {
	const char *string_name; /* signal name format */
	int val; /* Options for the mux register value */
};

static struct omap_board_config_kernel mityarm335x_config[] __initdata = {
};

/**
 * ###################################################################
 * Factory config data and methods
 * ###################################################################
 */
static struct mityarm335x_factory_config factory_config;
static void __init setup_config_peripherals(void);
static void (*mityarm335x_config_callback)(const struct  mityarm335x_factory_config*)=NULL;

void mityarm335x_set_config_callback(
		void (*callback)(const struct  mityarm335x_factory_config*))
{
	mityarm335x_config_callback = callback;
}

/* Factory config block accessors */
const struct mityarm335x_factory_config* mityarm335x_factoryconfig(void)
{
	return &factory_config;
}

bool mityarm335x_valid_magic(void)
{
	return factory_config.magic == FACTORY_CONFIG_MAGIC;
}

bool mityarm335x_valid_version(void)
{
	return (factory_config.version == CONFIG_I2C_VERSION_1_1 ||
		factory_config.version == CONFIG_I2C_VERSION_1_2);
}

u32 mityarm335x_serial_number(void)
{
	return factory_config.serialnumber;
}

const char* mityarm335x_model_number(void)
{
	return factory_config.partnum;
}

const u8* mityarm335x_mac_addr(void)
{
	return factory_config.mac;
}

bool mityarm335x_has_tiwi(void)
{
	bool rv = false;
	if(strlen(factory_config.partnum) > OPTION_POSITION &&
	   (0 == strncmp(&factory_config.partnum[OPTION_POSITION],OPTION_TIWI,3) ||
	    0 == strncmp(&factory_config.partnum[OPTION_POSITION],OPTION_TIWI_BLE,4)))
		rv = true;
	return rv;
}

size_t mityarm335x_ram_size(void)
{
	char size_code = factory_config.partnum[RAM_SIZE_POSITION];
	size_t rv = 0;
	switch (size_code) {
		case RAM_SIZE_256MB_DDR2:
		case RAM_SIZE_256MB_DDR3:
			rv = 256*1024*1024;
			break;
		case RAM_SIZE_512MB_DDR3:
			rv = 512*1024*1024;
			break;
		case RAM_SIZE_1GB_DDR3:
			rv = 1024*1024*1024;
			break;
		default:
			break;
	}
	return rv;
}

size_t mityarm335x_nand_size(void)
{
	char size_code = factory_config.partnum[NAND_SIZE_POSITION];
	size_t rv = 0;
	switch (size_code) {
		case NAND_SIZE_256MB:
			rv = 256*1024*1024;
			break;
		case NAND_SIZE_512MB:
			rv = 512*1024*1024;
			break;
		case NAND_SIZE_1GB:
			rv = 1*1024*1024*1024;
			break;
		case NAND_SIZE_NONE:
		default:
			break;
	}
	return rv;
}

size_t mityarm335x_nor_size(void)
{
	char size_code = factory_config.partnum[NOR_SIZE_POSITION];
	size_t rv = 0;
	switch (size_code) {
		case NOR_SIZE_2MB:
			rv = 2*1024*1024;
			break;
		case NOR_SIZE_8MB:
			rv = 8*1024*1024;
			break;
		case NOR_SIZE_16MB:
			rv = 16*1024*1024;
			break;
		case NOR_SIZE_NONE:
		default:
			break;
	}
	return rv;
}

u32 mityarm335x_speed_grade(void)
{
	char speed_code = factory_config.partnum[SPEED_GRADE_POSITION];
	size_t rv = 0;
	switch (speed_code) {
		case SPEED_GRADE_720:
			rv = 720000000;
			break;
		default:
			break;
	}
	return rv;
}

/*
* am335x_evm_get_id - returns Board Type (EVM/BB/EVM-SK ...)
*
* Note:
*	returns -EINVAL if Board detection hasn't happened yet.
*/
int am335x_evm_get_id(void)
{
	return -EINVAL;
}
EXPORT_SYMBOL(am335x_evm_get_id);

/* END --- Factory config block accessors */

/**
 * ###################################################################
 *  Pin mux Settings for all on-board peripherals
 * ###################################################################
 */


/* Pin mux for on board nand flash */
static struct pinmux_config __initdata nand_pin_mux[] = {
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

/* Module pin mux for SPI flash */
static struct pinmux_config __initdata spi1_pin_mux[] = {
	// spi clk needs to be an input. Pull should be up for spi modes 2 and 3
	// https://e2e.ti.com/support/arm/sitara_arm/f/791/p/649334/2395792#2395792
	{"ecap0_in_pwm0_out.spi1_sclk",	AM33XX_PIN_INPUT_PULLUP },
	{"mcasp0_fsx.spi1_d0",		AM33XX_PIN_INPUT_PULLUP },
	{"mcasp0_axr0.spi1_d1",		AM33XX_PIN_INPUT_PULLDOWN },
	{"mcasp0_ahclkr.spi1_cs0",	AM33XX_PIN_INPUT_PULLUP },
	{NULL, 0},
};

/*
 * Module pin mux for I2C1, shares factory config PROM, port SCLSR_EN1/
 * SDASR_EN2 (pins 11/10, ID0) of TPS65910.  Also goes to edge connector.
 */
static struct pinmux_config __initdata i2c1_pin_mux[] = {
	{"mii1_crs.i2c1_sda",	AM33XX_SLEWCTRL_SLOW | AM33XX_PULL_ENBL |
				AM33XX_INPUT_EN | AM33XX_PIN_OUTPUT},
	{"mii1_rxerr.i2c1_scl",	AM33XX_SLEWCTRL_SLOW | AM33XX_PULL_ENBL |
				AM33XX_INPUT_EN | AM33XX_PIN_OUTPUT},
	{NULL, 0},
};

/*
 * Module pin mux for I2C2.  Connected to port SCL_SCK/SDA_SDI (pins 9/8, ID1)
 * of TPS65910.  Does not leave module.
 * Note: Not used for TiWi Module.
 */
static struct pinmux_config __initdata __maybe_unused i2c2_pin_mux[] = {
	{"uart1_ctsn.i2c2_sda",	AM33XX_SLEWCTRL_SLOW | AM33XX_PULL_ENBL |
				AM33XX_INPUT_EN | AM33XX_PIN_OUTPUT},
	{"uart1_rtsn.i2c2_scl",	AM33XX_SLEWCTRL_SLOW | AM33XX_PULL_ENBL |
				AM33XX_INPUT_EN | AM33XX_PIN_OUTPUT},
	{NULL, 0},
};

/*
 * Module pin mux for TiWi functions. 
 * The TiWi module sits on MMC/SDIO 1 and uses UART1 for Bluetooth.
 * There are 3 GPIOs used:
 *  - WL Enable
 *  - BT Enable
 *  - WL Interrupt 
 *  Does not leave module.
 */
static struct pinmux_config __initdata __maybe_unused tiwi_pin_mux[] = {
	{"gpmc_ad8.mmc1_dat0", AM33XX_PIN_INPUT_PULLUP},
	{"gpmc_ad9.mmc1_dat1", AM33XX_PIN_INPUT_PULLUP},
	{"gpmc_ad10.mmc1_dat2", AM33XX_PIN_INPUT_PULLUP},
	{"gpmc_ad11.mmc1_dat3", AM33XX_PIN_INPUT_PULLUP},
	{"gpmc_csn1.mmc1_clk",   AM33XX_PIN_INPUT_PULLUP},
	{"gpmc_csn2.mmc1_cmd",   AM33XX_PIN_INPUT_PULLUP},
	/* UART 1 goes to the BT function on the Tiwi module */
	{"uart1_ctsn.uart1_ctsn", AM33XX_PIN_OUTPUT},
	{"uart1_rtsn.uart1_rtsn", AM33XX_PIN_INPUT},
	{"uart1_rxd.uart1_rxd", AM33XX_PIN_INPUT_PULLUP},
	{"uart1_txd.uart1_txd", AM33XX_PULL_ENBL | AM33XX_PULL_ENBL},
	/* 3 GPIOs are needed */
	{"gpmc_ad12.gpio1_12", AM33XX_PIN_INPUT_PULLUP},/*IRQ*/
	{"gpmc_ad13.gpio1_13", AM33XX_PULL_ENBL | AM33XX_PIN_OUTPUT},/*WL EN*/
	{"gpmc_csn3.gpio2_0", AM33XX_PULL_ENBL | AM33XX_PIN_OUTPUT},/*BT EN*/
	{NULL, 0}
};

/*
* @pin_mux - single module pin-mux structure which defines pin-mux
*			details for all its pins.
*/
static void __init setup_pin_mux(struct pinmux_config *pin_mux)
{
	int i;

	for (i = 0; pin_mux->string_name != NULL; pin_mux++)
		omap_mux_init_signal(pin_mux->string_name, pin_mux->val);

}

#define TIWI_WLAN_IRQ_GPIO		GPIO_TO_PIN(1, 12)

/**
 * ###################################################################
 *  Platform data Settings for all on-board peripherals
 * ###################################################################
 */

static struct wl12xx_platform_data __maybe_unused am335x_tiwi_data = {
	.irq = OMAP_GPIO_IRQ(TIWI_WLAN_IRQ_GPIO),
	.board_ref_clock = WL12XX_REFCLOCK_38_XTAL, /* 38.4Mhz */
};

static struct omap2_hsmmc_info __initdata __maybe_unused *board_mmc_info = NULL;

/**
 * Called by baseboard to allow module to insert on-module device(s)
 * before baseboard calls omap2_hsmmc_init()
 * \param devinfo must be an array of omap2_hsmmc_info types sized to fit
 * the number of SDIO busses on the module (3)
 * \return -1 if no change made, else index of updated mmc entry
 */
int __init mityarm335x_som_mmc_fixup(struct omap2_hsmmc_info* devinfo)
{
	int rv=-1;
#ifdef CONFIG_MITYARM335X_TIWI
	if(0 == devinfo[1].mmc) {
		rv = 1;
		board_mmc_info = &devinfo[1];
		board_mmc_info->mmc = 2;
		board_mmc_info->name = "wl1271";
		board_mmc_info->caps = MMC_CAP_4_BIT_DATA | MMC_CAP_POWER_OFF_CARD
			| MMC_PM_KEEP_POWER;
		board_mmc_info->nonremovable = true;
		board_mmc_info->gpio_cd = -EINVAL;
		board_mmc_info->gpio_wp = -EINVAL;
		board_mmc_info->ocr_mask = MMC_VDD_32_33 | MMC_VDD_33_34; /* 3V3 */
	}
#endif
	return rv;
}

/**
 * Defined by baseboard to allow it to initialize its own nand.
 */
int __weak mityarm335x_baseboard_nand_fixup(struct gpmc_devices_info* devinfo)
{
	return 0;
}

/* NAND partition information */
static struct mtd_partition mityarm335x_nand_partitions_2k[] = {
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

static struct mtd_partition mityarm335x_nand_partitions_4k[] = {
	/* All the partition sizes are listed in terms of NAND block size */
	{
		.name = "SPL",
		.offset = 0, /* Offset = 0x0 */
		.size = SZ_256K,
	},
	{
		.name = "SPL.backup1",
		.offset = MTDPART_OFS_APPEND, /* Offset = 0x40000 */
		.size = SZ_256K,
	},
	{
		.name = "SPL.backup2",
		.offset = MTDPART_OFS_APPEND, /* Offset = 0x80000 */
		.size = SZ_256K,
	},
	{
		.name = "SPL.backup3",
		.offset = MTDPART_OFS_APPEND, /* Offset = 0xc0000 */
		.size = SZ_256K,
	},
	{
		.name = "U-Boot",
		.offset = MTDPART_OFS_APPEND, /* Offset = 0x100000 */
		.size = 8 * SZ_256K,
	},
	{
		.name = "U-Boot Env",
		.offset = MTDPART_OFS_APPEND, /* Offset = 0x300000 */
		.size = 1 * SZ_256K,
	},
	{
		.name = "Kernel",
		.offset = MTDPART_OFS_APPEND, /* Offset = 0x340000 */
		.size = 20 * SZ_256K,
	},
	{
		.name = "File System",
		.offset = MTDPART_OFS_APPEND, /* Offset = 0x840000 */
		.size = MTDPART_SIZ_FULL,
	},
};

static struct mtd_partition mityarm335x_nand_partitions_4k_1GB[] = {
	/* All the partition sizes are listed in terms of NAND block size */
	{
		.name = "SPL",
		.offset = 0, /* Offset = 0x0 */
		.size = SZ_512K,
	},
	{
		.name = "SPL.backup1",
		.offset = MTDPART_OFS_APPEND, /* Offset = 0x80000 */
		.size = SZ_512K,
	},
	{
		.name = "SPL.backup2",
		.offset = MTDPART_OFS_APPEND, /* Offset = 0x100000 */
		.size = SZ_512K,
	},
	{
		.name = "SPL.backup3",
		.offset = MTDPART_OFS_APPEND, /* Offset = 0x180000 */
		.size = SZ_512K,
	},
	{
		.name = "U-Boot",
		.offset = MTDPART_OFS_APPEND, /* Offset = 0x200000 */
		.size = 4 * SZ_512K,
	},
	{
		.name = "U-Boot Env",
		.offset = MTDPART_OFS_APPEND, /* Offset = 0x400000 */
		.size = 1 * SZ_512K,
	},
	{
		.name = "Kernel",
		.offset = MTDPART_OFS_APPEND, /* Offset = 0x480000 */
		.size = 10 * SZ_512K,
	},
	{
		.name = "File System",
		.offset = MTDPART_OFS_APPEND, /* Offset = 0x980000 */
		.size = MTDPART_SIZ_FULL,
	},
};

/* TODO board-am335x has identical struct */
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

static void __init mityarm335x_nand_init(size_t nand_size)
{
	int pinmux_nand = 0;
	struct omap_nand_platform_data *pdata;
	struct gpmc_devices_info gpmc_device[3] = {
		{ NULL, 0 },
		{ NULL, 0 },
		{ NULL, 0 },
	};

	if (nand_size > 0) {
		pinmux_nand = 1;
		/* if nand size >= 1GB NAND uses a 4K page size */
		if(nand_size >= 1024*1024*1024) {
			pdata = omap_nand_init(mityarm335x_nand_partitions_4k_1GB,
					ARRAY_SIZE(mityarm335x_nand_partitions_4k_1GB), 0, 0,
					&am335x_nand_timings);
			pdata->ecc_opt = OMAP_ECC_BCH16_CODE_HW;
		/* if nand size >= 512MB NAND uses a 4K page size */
		} else if(nand_size >= 512*1024*1024) {
			pdata = omap_nand_init(mityarm335x_nand_partitions_4k,
					ARRAY_SIZE(mityarm335x_nand_partitions_4k), 0, 0,
					&am335x_nand_timings);
			pdata->ecc_opt = OMAP_ECC_BCH16_CODE_HW;
		/* Everything else is a 2K page size */
		} else {
			pdata = omap_nand_init(mityarm335x_nand_partitions_2k,
					ARRAY_SIZE(mityarm335x_nand_partitions_2k), 0, 0,
					&am335x_nand_timings);
			pdata->ecc_opt = OMAP_ECC_BCH8_CODE_HW;
		}

		if (!pdata)
			return;
		pdata->elm_used = true;
		gpmc_device[0].pdata = pdata;
		gpmc_device[0].flag = GPMC_DEVICE_NAND;
	}

	/* Baseboard may still want nand pinmuxed even if there's no on-SOM nand */
	pinmux_nand |= mityarm335x_baseboard_nand_fixup(gpmc_device);

	if (pinmux_nand)
		setup_pin_mux(nand_pin_mux);

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
		.controller_data	= (void *)&spi1_ctlr_data,
		.irq			= -1,
		.max_speed_hz		= 30000000,
		.bus_num		= 2,
		.chip_select		= 0,
		.mode			= SPI_MODE_3,
	},
};

/* setup spi1 */
static void __init spi1_init(void)
{
	setup_pin_mux(spi1_pin_mux);
	spi_register_board_info(mityarm335x_spi1_slave_info,
			ARRAY_SIZE(mityarm335x_spi1_slave_info));
	return;
}

static struct regulator_init_data am335x_dummy = {
	.constraints.always_on	= true,
};

static struct regulator_consumer_supply am335x_vdd1_supply[] = {
	REGULATOR_SUPPLY("vdd_mpu", NULL),
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

static struct regulator_consumer_supply am335x_vdd2_supply[] = {
	REGULATOR_SUPPLY("vdd_core", NULL),
};

static struct regulator_init_data am335x_vdd2 = {
	.constraints = {
		.min_uV			= 600000,
		.max_uV			= 1500000,
		.valid_modes_mask	= REGULATOR_MODE_NORMAL,
		.valid_ops_mask		= REGULATOR_CHANGE_VOLTAGE,
		.always_on		= 1,
	},
	.num_consumer_supplies	= ARRAY_SIZE(am335x_vdd2_supply),
	.consumer_supplies	= am335x_vdd2_supply,
};

static struct tps65910_board mityarm335x_tps65910_info = {
	.tps65910_pmic_init_data[TPS65910_REG_VRTC]	= &am335x_dummy,
	.tps65910_pmic_init_data[TPS65910_REG_VIO]	= &am335x_dummy,
	.tps65910_pmic_init_data[TPS65910_REG_VDD1]	= &am335x_vdd1,
	.tps65910_pmic_init_data[TPS65910_REG_VDD2]	= &am335x_vdd2,
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

static void __init read_factory_config(struct memory_accessor *a, void* context)
{
	int ret;
	const char *partnum = NULL;

	ret = a->read(a, (char *)&factory_config, 0, sizeof(factory_config));
	if (ret != sizeof(struct mityarm335x_factory_config)) {
		pr_warning("MitySOM-335x: Read Factory Config Failed: %d\n",
			ret);
		goto bad_config;
	}

	if (!mityarm335x_valid_magic()) {
		pr_warning("MitySOM-335x: Factory Config Magic Wrong (%X)\n",
			factory_config.magic);
		goto bad_config;
	}

	if (!mityarm335x_valid_version()) {
		pr_warning("MitySOM-335x: Factory Config Version Wrong (%X)\n",
			factory_config.version);
		goto bad_config;
	}

	partnum = factory_config.partnum;
	pr_info("MitySOM-335x: Model Number = %s\n", partnum);

	setup_config_peripherals();
bad_config:
	return;
}


static struct at24_platform_data __initdata mityarm335x_fd_info = {
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
#ifdef CONFIG_MITYARM335X_TIWI
	{
		I2C_BOARD_INFO("tps65910", TPS65910_I2C_ID1),
		.platform_data  = &mityarm335x_tps65910_info,
	},
#endif
};

static void __init mityarm335x_i2c1_init(void)
{
	pr_info("Configuring I2C Bus 1\n");
	setup_pin_mux(i2c1_pin_mux);
	omap_register_i2c_bus(2, 100, mityarm335x_i2c1_boardinfo,
				ARRAY_SIZE(mityarm335x_i2c1_boardinfo));
}

/* TIWI module doesn't have I2C2 instead tps65910 moved to i2c1 */
#ifndef CONFIG_MITYARM335X_TIWI
static struct i2c_board_info __initdata mityarm335x_i2c2_boardinfo[] = {
	{
		I2C_BOARD_INFO("tps65910", TPS65910_I2C_ID1),
		.platform_data  = &mityarm335x_tps65910_info,
	},
};

static void __init mityarm335x_i2c2_init(void)
{
	pr_info("Configuring I2C Bus 2\n");
	setup_pin_mux(i2c2_pin_mux);
	omap_register_i2c_bus(3, 100, mityarm335x_i2c2_boardinfo,
				ARRAY_SIZE(mityarm335x_i2c2_boardinfo));
}
#endif

static struct omap_rtc_pdata am335x_rtc_info = {
	.pm_off		= false,
	.wakeup_capable	= 0,
};

static void __init am335x_rtc_init(void)
{
	void __iomem *base;
	struct clk *clk;
	struct omap_hwmod *oh;
	struct platform_device *pdev;
	char *dev_name = "am33xx-rtc";

	clk = clk_get(NULL, "rtc_fck");
	if (IS_ERR(clk)) {
		pr_err("rtc : Failed to get RTC clock\n");
		return;
	}

	if (clk_enable(clk)) {
		pr_err("rtc: Clock Enable Failed\n");
		return;
	}

	base = ioremap(AM33XX_RTC_BASE, SZ_4K);

	if (WARN_ON(!base))
		return;

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

	clk_disable(clk);
	clk_put(clk);

	if (omap_rev() >= AM335X_REV_ES2_0)
		am335x_rtc_info.wakeup_capable = 1;

	oh = omap_hwmod_lookup("rtc");
	if (!oh) {
		pr_err("could not look up %s\n", "rtc");
		return;
	}

	pdev = omap_device_build(dev_name, -1, oh, &am335x_rtc_info,
			sizeof(struct omap_rtc_pdata), NULL, 0, 0);
	WARN(IS_ERR(pdev), "Can't build omap_device for %s:%s.\n",
			dev_name, oh->name);
}

/* Enable clkout2 */
static struct pinmux_config __initdata clkout2_pin_mux[] = {
	{"xdma_event_intr1.clkout2", OMAP_MUX_MODE3 | AM33XX_PIN_OUTPUT},
	{NULL, 0},
};

static void __init clkout2_enable(void)
{
	struct clk *ck_32;

	ck_32 = clk_get(NULL, "clkout2_ck");
	if (IS_ERR(ck_32)) {
		pr_err("Cannot clk_get ck_32\n");
		return;
	}

	clk_enable(ck_32);

	setup_pin_mux(clkout2_pin_mux);
}

static void __iomem *am33xx_emif_base;

static void __iomem * __init am33xx_get_mem_ctlr(void)
{

	am33xx_emif_base = ioremap(AM33XX_EMIF0_BASE, SZ_32K);

	if (!am33xx_emif_base)
		pr_warning("%s: Unable to map DDR2 controller",	__func__);

	return am33xx_emif_base;
}

void __iomem *am33xx_get_ram_base(void)
{
	return am33xx_emif_base;
}

void __iomem *am33xx_gpio0_base;

void __iomem *am33xx_get_gpio0_base(void)
{
	am33xx_gpio0_base = ioremap(AM33XX_GPIO0_BASE, SZ_4K);

	return am33xx_gpio0_base;
}

static void __iomem *am33xx_i2c0_base;

int am33xx_map_i2c0(void)
{
	am33xx_i2c0_base = ioremap(AM33XX_I2C0_BASE, SZ_4K);

	if (!am33xx_i2c0_base)
		return -ENOMEM;

	return 0;
}

void __iomem *am33xx_get_i2c0_base(void)
{
	return am33xx_i2c0_base;
}

static struct resource am33xx_cpuidle_resources[] = {
	{
		.start		= AM33XX_EMIF0_BASE,
		.end		= AM33XX_EMIF0_BASE + SZ_32K - 1,
		.flags		= IORESOURCE_MEM,
	},
};

/* AM33XX devices support DDR2 power down */
static struct am33xx_cpuidle_config am33xx_cpuidle_pdata = {
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
static int mityarm335x_dbg_som_show(struct seq_file *s, void *unused)
{
	const char *partnum = NULL;
	if (!mityarm335x_valid_magic()) {
		pr_err("MitySOM-335x: Factory Config Magic Wrong (%X)\n",
			factory_config.magic);
		return -EFAULT;
	}

	if (!mityarm335x_valid_version()) {
		pr_err("MitySOM-335x: Factory Config Version Wrong (%X)\n",
			factory_config.version);
		return -EFAULT;
	}

	partnum = factory_config.partnum;
	seq_printf(s, "MitySOM-335x: Part Number = %s\n"
		      "            : Serial Num = %d\n"
		      "            : Found MAC = %pM\n",
		      partnum,
		      factory_config.serialnumber,
		      factory_config.mac);

	return 0;
}

static ssize_t mityarm335x_dbg_som_write(struct file *file,
					 const char __user *user_buf,
					 size_t count, loff_t *ppos)
{
	return 0;
}
static int mityarm335x_dbg_som_open(struct inode *inode, struct file *file)
{
	return single_open(file, mityarm335x_dbg_som_show, inode->i_private);
}

static const struct file_operations mityarm335x_dbg_som_fops = {
	.open		= mityarm335x_dbg_som_open,
	.read		= seq_read,
	.write		= mityarm335x_dbg_som_write,
	.llseek		= seq_lseek,
	.release	= single_release,
};

/* NOT static, so baseboard can see it... */
struct dentry *mityarm335x_dbg_dir;

static void __init mityarm335x_dbg_init(void)
{
	static struct dentry *mityarm335x_dbg_board_dir;

	mityarm335x_dbg_dir = debugfs_create_dir("mitysom335x", NULL);
	if (!mityarm335x_dbg_dir)
		return;
	mityarm335x_dbg_board_dir = debugfs_create_dir("module",
							mityarm335x_dbg_dir);
	if (!mityarm335x_dbg_board_dir)
		return;

	(void)debugfs_create_file("config", S_IRUGO,
				  mityarm335x_dbg_board_dir, NULL,
				  &mityarm335x_dbg_som_fops);

}

#ifdef CONFIG_MITYARM335X_TIWI
/**
 * ###################################################################
 *  TiWi (WiFi/BT module) data and access/control methods
 * ###################################################################
  */

void __init wl12xx_bluetooth_enable(void)
{
	if (am335x_tiwi_data.bt_enable_gpio > 0) {
		int status;
		pr_info("Configure Bluetooth Enable pin...\n");

		status = gpio_request_one(am335x_tiwi_data.bt_enable_gpio,
			GPIOF_OUT_INIT_LOW, "bt_en");

		if (status < 0) {
			pr_err("Failed to request gpio for bt_enable");
			return;
		}

		status = gpio_export(am335x_tiwi_data.bt_enable_gpio, false);
		pr_info("Exporting bt_enable: %d", status);

		if (status < 0) {
			pr_err("Failed to export gpio for bt_enable");
			return;
		}
	} else {
		pr_info("Bluetooth not Enabled!\n");
	}
}

static int wl12xx_set_power(struct device *dev, int slot, int on, int vdd)
{
	pr_info("wl12xx_set_power %s s=%d on=%d vdd=%d\n", dev_name(dev), slot, on, vdd);

	if (on) {
		gpio_set_value(am335x_tiwi_data.wlan_enable_gpio, 1);
		mdelay(70);
	}
	else {
		gpio_set_value(am335x_tiwi_data.wlan_enable_gpio, 0);
	}

	return 0;
}

static void __init init_wlan(void)
{
	setup_pin_mux(tiwi_pin_mux);

	/* Register WLAN and BT enable pins based on the evm board revision */
	am335x_tiwi_data.wlan_enable_gpio = GPIO_TO_PIN(1,13);
	am335x_tiwi_data.bt_enable_gpio = GPIO_TO_PIN(2,0);

	pr_info("WLAN GPIO Info.. IRQ = %3d WL_EN = %3d BT_EN = %3d\n",
			am335x_tiwi_data.irq,
			am335x_tiwi_data.wlan_enable_gpio,
			am335x_tiwi_data.bt_enable_gpio);

	wl12xx_bluetooth_enable();

	if (wl12xx_set_platform_data(&am335x_tiwi_data)) {
		pr_err("error setting wl12xx data\n");
	}
}

static void __init setup_wlan(void)
{
	int ret;
	struct device *dev;
	struct omap_mmc_platform_data *pdata;

	if (!board_mmc_info) {
		pr_err("wl12xx mmc device not instantiated\n");
		goto out;
	}

	dev = board_mmc_info->dev;
	if (!dev) {
		pr_err("wl12xx mmc device initialization failed\n");
		goto out;
	}
	pdata = dev->platform_data;
	if (!pdata) {
		pr_err("Platfrom data of wl12xx device not set\n");
		goto out;
	}

	ret = gpio_request_one(am335x_tiwi_data.wlan_enable_gpio,
		GPIOF_OUT_INIT_LOW, "wlan_en");
	if (ret) {
		pr_err("Error requesting wlan enable gpio: %d\n", ret);
		goto out;
	}

	pdata->slots[0].set_power = wl12xx_set_power;

	pr_info("setup_wlan: finished\n");
out:
	return;

}
#endif /* CONFIG_MITYARM335X_TIWI */

static void __init sgx_init(void)
{
	if (omap3_has_sgx()) {
		am33xx_gpu_init();
	}
}

/**
 * Set up peripherals that are dependent on things set in the factory configuration
 * These include:
 * NAND
 * TIWI (WLAN) (future)
 * SPI NOR (unimplemented)
 */
static void __init setup_config_peripherals(void)
{
	if(mityarm335x_config_callback) {
		(*mityarm335x_config_callback)(&factory_config);
	}

	/*
	 * Always init the spi and nor.  This is because the mtd # of the nand
	 * device changes if the nor isn't present.  Which means nand scripts for the
	 * chips with nor will act unexpectedly on chips without nor.
	 */
	spi1_init();
	if(0 == mityarm335x_nor_size()) {
		pr_info("No SPI NOR Flash found.\n");
	}

	if(0 != mityarm335x_nand_size()) {
		pr_info("Configuring %dMB NAND device\n", mityarm335x_nand_size()/(1024*1024));
	}
	else {
		pr_info("No NAND device configured\n");
	}

	/*
	 * Always call this because it calls a fixup to init the baseboard's
	 * nand if we're building for a baseboard that has one.
	 */
	mityarm335x_nand_init(mityarm335x_nand_size());

#ifdef CONFIG_MITYARM335X_TIWI
	if(mityarm335x_has_tiwi()) {
		init_wlan();
		setup_wlan();
	}
	else {
		WARN(true, "Kernel configured for TIWI, but no TIWI module found.\n");
	}
#else
	if(mityarm335x_has_tiwi()) {
		WARN(true, "TIWI module found, but kernel not configured for TIWI.\n");
	}
#endif

	return;
}

static void __init mityarm335x_init(void)
{
	am33xx_cpuidle_init();
	am33xx_mux_init(NULL);
	omap_serial_init();
	am335x_rtc_init();
	clkout2_enable();

	mityarm335x_i2c1_init();

#ifndef CONFIG_MITYARM335X_TIWI
	mityarm335x_i2c2_init();
#endif

	sgx_init();

	omap_sdrc_init(NULL, NULL);
	omap_board_config = mityarm335x_config;
	omap_board_config_size = ARRAY_SIZE(mityarm335x_config);

	/* Create an alias for icss clock */
	if (clk_add_alias("pruss", NULL, "pruss_uart_gclk", NULL))
		pr_err("failed to create an alias: pruss_uart_gclk --> pruss\n");
	/* Create an alias for gfx/sgx clock */
	if (clk_add_alias("sgx_ck", NULL, "gfx_fclk", NULL))
		pr_err("failed to create an alias: gfx_fclk --> sgx_ck\n");

	mityarm335x_dbg_init();

}

static void __init mityarm335x_map_io(void)
{
	omap2_set_globals_am33xx();
	omapam33xx_map_common_io();
}

MACHINE_START(MITYARM335X, "mitysom335x")
	/* Maintainer: Critical Link, LLC */
	.atag_offset	= 0x100,
	.map_io		= mityarm335x_map_io,
	.init_irq	= ti81xx_init_irq,
	.init_early	= am33xx_init_early,
	.handle_irq	= omap3_intc_handle_irq,
	.timer		= &omap3_am33xx_timer,
	.init_machine	= mityarm335x_init,
MACHINE_END

