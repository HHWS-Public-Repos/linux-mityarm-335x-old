/*
 * Critical Link MityARM-335x SoM Development Kit Baseboard Initialization File
 *
 * Someday... Someday... most of this should be replaced with device tree....
 */
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/phy.h>
#include <linux/usb/musb.h>
#include <linux/dma-mapping.h>
#include <linux/spi/spi.h>
#include <linux/spi/ads7846.h>

#include <linux/gpio.h>

/* TSc controller */
#include <linux/input/ti_tscadc.h>
#include <linux/lis3lv02d.h>

#include <video/da8xx-fb.h>
#include <plat/lcdc.h> /* uhhggg... */
#include <plat/mmc.h>
#include <plat/usb.h>
#include <plat/omap_device.h>
#include <plat/mcspi.h>
#include <plat/i2c.h>

#include <asm/hardware/asp.h>

#include "mux.h"
#include "hsmmc.h"
#include "devices.h"

#define BASEBOARD_NAME "MityARM-335x DevKit"
/* Vitesse 8601 register defs we need... */
#define VSC8601_PHY_ID   (0x00070420)
#define VSC8601_PHY_MASK (0xFFFFFFFC)
#define MII_EXTPAGE		 (0x1F)
#define RGMII_SKEW		 (0x1C)
#define MITY335X_DK_SPIBUS_TS (1)

/* TODO - refactor all the pinmux stuff for all board files to use */
#define GPIO_TO_PIN(bank, gpio) (32 * (bank) + (gpio))

#define MITY335X_DK_GPIO_TS_IRQ_N	GPIO_TO_PIN(0,20)
#define MITY335X_DK_GPIO_BACKLIGHT	GPIO_TO_PIN(3,14)


#if defined(CONFIG_TOUCHSCREEN_ADS7846) || \
	defined(CONFIG_TOUCHSCREEN_ADS7846_MODULE)
#define TS_USE_SPI 0 /*1*/
#else
#define TS_USE_SPI 0
#endif
struct pinmux_config {
	const char	*muxname;
	int		val;
};


#define setup_pin_mux(pin_mux) \
{ \
	int i = 0; \
	for (; pin_mux[i].muxname != NULL; i++) \
		omap_mux_init_signal(pin_mux[i].muxname, pin_mux[i].val); \
}

/******************************************************************************
 *
 *                                N O T E
 *
 *                       PUT ALL PINMUX SETTINGS HERE
 *
 *****************************************************************************/

static struct pinmux_config rgmii2_pin_mux[] = {
	{"gpmc_a0.rgmii2_tctl",	AM33XX_PIN_OUTPUT},
	{"gpmc_a1.rgmii2_rctl",	AM33XX_PIN_INPUT_PULLDOWN},
	{"gpmc_a2.rgmii2_td3",	AM33XX_PIN_OUTPUT},
	{"gpmc_a3.rgmii2_td2",	AM33XX_PIN_OUTPUT},
	{"gpmc_a4.rgmii2_td1",	AM33XX_PIN_OUTPUT},
	{"gpmc_a5.rgmii2_td0",	AM33XX_PIN_OUTPUT},
	{"gpmc_a6.rgmii2_tclk",	AM33XX_PIN_OUTPUT},
	{"gpmc_a7.rgmii2_rclk",	AM33XX_PIN_INPUT_PULLDOWN},
	{"gpmc_a8.rgmii2_rd3",	AM33XX_PIN_INPUT_PULLDOWN},
	{"gpmc_a9.rgmii2_rd2",	AM33XX_PIN_INPUT_PULLDOWN},
	{"gpmc_a10.rgmii2_rd1",	AM33XX_PIN_INPUT_PULLDOWN},
	{"gpmc_a11.rgmii2_rd0",	AM33XX_PIN_INPUT_PULLDOWN},
	{"mdio_data.mdio_data",	AM33XX_PIN_INPUT_PULLUP},
	{"mdio_clk.mdio_clk",	AM33XX_PIN_OUTPUT_PULLUP},
	{NULL, 0}
};

static struct pinmux_config lcdc_pin_mux[] = {
	{"lcd_data0.lcd_data0",		AM33XX_PIN_OUTPUT | AM33XX_PULL_DISA},
	{"lcd_data1.lcd_data1",		AM33XX_PIN_OUTPUT | AM33XX_PULL_DISA},
	{"lcd_data2.lcd_data2",		AM33XX_PIN_OUTPUT | AM33XX_PULL_DISA},
	{"lcd_data3.lcd_data3",		AM33XX_PIN_OUTPUT | AM33XX_PULL_DISA},
	{"lcd_data4.lcd_data4",		AM33XX_PIN_OUTPUT | AM33XX_PULL_DISA},
	{"lcd_data5.lcd_data5",		AM33XX_PIN_OUTPUT | AM33XX_PULL_DISA},
	{"lcd_data6.lcd_data6",		AM33XX_PIN_OUTPUT | AM33XX_PULL_DISA},
	{"lcd_data7.lcd_data7",		AM33XX_PIN_OUTPUT | AM33XX_PULL_DISA},
	{"lcd_data8.lcd_data8",		AM33XX_PIN_OUTPUT | AM33XX_PULL_DISA},
	{"lcd_data9.lcd_data9",		AM33XX_PIN_OUTPUT | AM33XX_PULL_DISA},
	{"lcd_data10.lcd_data10",	AM33XX_PIN_OUTPUT | AM33XX_PULL_DISA},
	{"lcd_data11.lcd_data11",	AM33XX_PIN_OUTPUT | AM33XX_PULL_DISA},
	{"lcd_data12.lcd_data12",	AM33XX_PIN_OUTPUT | AM33XX_PULL_DISA},
	{"lcd_data13.lcd_data13",	AM33XX_PIN_OUTPUT | AM33XX_PULL_DISA},
	{"lcd_data14.lcd_data14",	AM33XX_PIN_OUTPUT | AM33XX_PULL_DISA},
	{"lcd_data15.lcd_data15",	AM33XX_PIN_OUTPUT | AM33XX_PULL_DISA},
	{"lcd_vsync.lcd_vsync",		AM33XX_PIN_OUTPUT},
	{"lcd_hsync.lcd_hsync",		AM33XX_PIN_OUTPUT},
	{"lcd_pclk.lcd_pclk",		AM33XX_PIN_OUTPUT},
	{"lcd_ac_bias_en.lcd_ac_bias_en", AM33XX_PIN_OUTPUT},
	/* GPIO for the backlight */
	{ "mcasp0_aclkx.gpio3_14", AM33XX_PIN_OUTPUT},
	{NULL, 0}
};

static struct pinmux_config mmc0_pin_mux[] = {
	{"mmc0_dat3.mmc0_dat3", AM33XX_PIN_INPUT_PULLUP},
	{"mmc0_dat2.mmc0_dat2", AM33XX_PIN_INPUT_PULLUP},
	{"mmc0_dat1.mmc0_dat1", AM33XX_PIN_INPUT_PULLUP},
	{"mmc0_dat0.mmc0_dat0", AM33XX_PIN_INPUT_PULLUP},
	{"mmc0_clk.mmc0_clk",   AM33XX_PIN_INPUT_PULLUP},
	{"mmc0_cmd.mmc0_cmd",   AM33XX_PIN_INPUT_PULLUP},
	{"mii1_txen.gpio3_3",	AM33XX_PIN_INPUT_PULLUP}, /* SD Card Detect */
	{"mii1_col.gpio3_0",	AM33XX_PIN_INPUT_PULLUP}, /* SD Write Protect */
	{NULL, 0}
};

static struct pinmux_config can_pin_mux[] = {
	{"uart1_rxd.d_can1_tx", AM33XX_PULL_ENBL},
	{"uart1_txd.d_can1_rx", AM33XX_PIN_INPUT_PULLUP},
	{"mii1_txd3.d_can0_tx", AM33XX_PULL_ENBL},
	{"mii1_txd2.d_can0_rx", AM33XX_PIN_INPUT_PULLUP},
	{NULL, 0}
};

static struct pinmux_config expansion_pin_mux[] = {
	{"uart0_ctsn.uart4_rxd", AM33XX_PIN_INPUT_PULLUP},/* Exp0 RX */
	{"uart0_rtsn.uart4_txd", AM33XX_PULL_ENBL},		/* Exp0 TX */
	{"mii1_rxd3.uart3_rxd", AM33XX_PIN_INPUT_PULLUP},/* Exp1 RX */
	{"mii1_rxd2.uart3_txd", AM33XX_PULL_ENBL},		/* Exp1 TX */
	{NULL, 0}
};

static struct pinmux_config usb_pin_mux[] = {
	{"usb0_drvvbus.usb0_drvvbus",	AM33XX_PIN_OUTPUT},
	{"usb1_drvvbus.usb1_drvvbus",	AM33XX_PIN_OUTPUT},
	{NULL, 0}
};

static struct pinmux_config ts_pin_mux[] = {
	/* SPI0 CS0 taken care of by SPI pinmux setup */
	{"xdma_event_intr1.gpio0_20",	AM33XX_PIN_INPUT}, /* Pen down */
	{"xdma_event_intr0.gpio0_19",	AM33XX_PIN_INPUT}, /* 7843 busy (not used)*/
	{NULL, 0}
};

/* Module pin mux for mcasp1 */
static struct pinmux_config mcasp1_pin_mux[] = {
	{"mcasp0_aclkr.mcasp1_aclkx", AM33XX_PIN_INPUT_PULLDOWN},
	{"mcasp0_fsr.mcasp1_fsx", AM33XX_PIN_INPUT_PULLDOWN},
	{"mcasp0_axr1.mcasp1_axr0", AM33XX_PIN_OUTPUT},
	{"mcasp0_ahclkx.mcasp1_axr1", AM33XX_PIN_INPUT_PULLDOWN},
	{"mii1_rxd0.mcasp1_ahclkr", AM33XX_PIN_INPUT_PULLDOWN},
	{"rmii1_refclk.mcasp1_ahclkx", AM33XX_PIN_INPUT_PULLDOWN},
	{NULL, 0},
};


static struct pinmux_config i2c0_pin_mux[] = {
	{"i2c0_sda.i2c0_sda",	AM33XX_SLEWCTRL_SLOW | AM33XX_PULL_ENBL |
				AM33XX_INPUT_EN | AM33XX_PIN_OUTPUT},
	{"i2c0_scl.i2c0_scl",	AM33XX_SLEWCTRL_SLOW | AM33XX_PULL_ENBL |
				AM33XX_INPUT_EN | AM33XX_PIN_OUTPUT},
	{NULL, 0},
};

static struct pinmux_config spi0_pin_mux[] = {
	{"spi0_cs0.spi0_cs0", AM33XX_PIN_OUTPUT_PULLUP},
	{"spi0_cs1.spi0_cs1", AM33XX_PIN_OUTPUT_PULLUP},
	{"spi0_sclk.spi0_sclk", AM33XX_PIN_OUTPUT_PULLUP},
	{"spi0_d0.spi0_d0", AM33XX_PIN_OUTPUT},
	{"spi0_d1.spi0_d1", AM33XX_PULL_ENBL | AM33XX_INPUT_EN},
	{NULL, 0},
};


static struct pinmux_config tsc_pin_mux[] = {
	{"ain0.ain0",     AM33XX_INPUT_EN},
	{"ain1.ain1",     AM33XX_INPUT_EN},
	{"ain2.ain2",     AM33XX_INPUT_EN},
	{"ain3.ain3",     AM33XX_INPUT_EN},
	{"vrefp.vrefp",   AM33XX_INPUT_EN},
	{"vrefn.vrefn",   AM33XX_INPUT_EN},
	{NULL, 0},
};

static struct omap2_hsmmc_info mmc_info[] __initdata = {
	{
		.mmc		= 1,
		.caps		= MMC_CAP_4_BIT_DATA,
		.gpio_cd	= GPIO_TO_PIN(3, 3),
		.gpio_wp	= GPIO_TO_PIN(3, 0),
		.ocr_mask	= MMC_VDD_32_33 | MMC_VDD_33_34,
	},
	{} /* Terminator */
	};

static __init void baseboard_setup_can(void)
{
	setup_pin_mux(can_pin_mux);

	am33xx_d_can_init(0);
	am33xx_d_can_init(1);
}

static struct omap_musb_board_data board_data = {
	.interface_type	= MUSB_INTERFACE_ULPI,
	.mode           = MUSB_OTG,
	.power		= 500,
	.instances	= 1,
};

static __init void baseboard_setup_usb(void)
{
	setup_pin_mux(usb_pin_mux);
	usb_musb_init(&board_data);
}

static __init void baseboard_setup_mmc(void)
{
	/* pin mux */
	setup_pin_mux(mmc0_pin_mux);

	/* configure mmc */
	omap2_hsmmc_init(mmc_info);
}

static const struct display_panel disp_panel = {
	WVGA,		/* panel_type */
	32,		/* max_bpp */
	16,		/* min_bpp */
	COLOR_ACTIVE,	/* panel_shade */
};

static struct lcd_ctrl_config dvi_cfg = {
	.p_disp_panel		= &disp_panel,
	.ac_bias		= 255,
	.ac_bias_intrpt		= 0,
	.dma_burst_sz		= 16,
	.bpp			= 16,
	.fdd			= 0x80,
	.tft_alt_mode		= 0,
	.stn_565_mode		= 0,
	.mono_8bit_mode		= 0,
	.invert_line_clock	= 1,
	.invert_frm_clock	= 1,
	.sync_edge		= 0,
	.sync_ctrl		= 1,
	.raster_order		= 0,
};

/* TODO - should really update driver to support VESA mode timings... */
struct da8xx_lcdc_platform_data dvi_pdata = {
	.manu_name		= "VESA",
	.controller_data	= &dvi_cfg,
	.type			= "800x600",
};
#ifdef CONFIG_BACKLIGHT_TPS6116X

static struct platform_device tps6116x_device = {
	.name   = "tps6116x",
	.id     = -1,
	.dev    = {
			.platform_data  = (void*)MITY335X_DK_GPIO_BACKLIGHT,
	},
};
#endif /* CONFIG_BACKLIGHT_TPS6116X */


#if (TS_USE_SPI)
static struct ads7846_platform_data ads7846_config = {
	.model				= 7843,
	.vref_mv			= 3300,
	.x_max				= 0x0fff,
	.y_max				= 0x0fff,
	.x_plate_ohms		= 180,
	.pressure_max		= 255,
	.debounce_max		= 0, //200,
	.debounce_tol		= 5,
	.debounce_rep		= 10,
	.gpio_pendown		= MITY335X_DK_GPIO_TS_IRQ_N,
	.keep_vref_on		= 1,
	.irq_flags			= IRQF_TRIGGER_FALLING,
	.vref_delay_usecs	= 100,
	.settle_delay_usecs	= 200,
	.penirq_recheck_delay_usecs = 1000,
	.filter_init		= 0,
	.filter				= 0,
	.filter_cleanup		= 0,
	.gpio_pendown		= MITY335X_DK_GPIO_TS_IRQ_N,
};

static __init void baseboard_setup_ts(void)
{
	setup_pin_mux(ts_pin_mux);
	/* SPI hookup already done by baseboard_setup_spi0() */
}
#else

static struct resource tsc_resources[]  = {
	[0] = {
		.start  = AM33XX_TSC_BASE,
		.end    = AM33XX_TSC_BASE + SZ_8K - 1,
		.flags  = IORESOURCE_MEM,
	},
	[1] = {
		.start  = AM33XX_IRQ_ADC_GEN,
		.end    = AM33XX_IRQ_ADC_GEN,
		.flags  = IORESOURCE_IRQ,
	},
};

static struct tsc_data am335x_touchscreen_data  = {
	.wires  = 4,
	.analog_input  = 1,
	.x_plate_resistance = 200,
};

static struct platform_device tsc_device = {
	.name   = "tsc",
	.id     = -1,
	.dev    = {
			.platform_data  = &am335x_touchscreen_data,
	},
	.num_resources  = ARRAY_SIZE(tsc_resources),
	.resource       = tsc_resources,
};


static __init void baseboard_setup_ts(void)
{
	int err;

	setup_pin_mux(tsc_pin_mux);
	err = platform_device_register(&tsc_device);
	if (err)
		pr_err("failed to register touchscreen device\n");
}
#endif /* CONFIG_TOUCHSCREEN_ADS7846 */

static __init void baseboard_setup_dvi(void)
{
	struct clk *disp_pll;
	int err = 0;

	/* pinmux */
	setup_pin_mux(lcdc_pin_mux);

	/* add I2C1 device entry */

	/* TODO - really need to modify da8xx driver to support mating to the
	 * TFP410 and tweaking settings at the driver level... need to stew on
	 * this..
	 */

	/* configure / enable LCDC */
	disp_pll = clk_get(NULL, "dpll_disp_ck");
	if (IS_ERR(disp_pll)) {
		pr_err("Connect get disp_pll\n");
		return;
	}

	if (clk_set_rate(disp_pll, 300000000)) {
		pr_warning("%s: Unable to initialize display PLL.\n",
			__func__);
		goto out;
	}

	if (am33xx_register_lcdc(&dvi_pdata))
		pr_warning("%s: Unable to register LCDC device.\n",
			__func__);

#ifdef CONFIG_BACKLIGHT_TPS6116X
	err = platform_device_register(&tps6116x_device);
	if (err)
		pr_err("failed to register backlight device\n");

#else
	/* backlight */
	/* TEMPORARY until driver is ready... just jam it on! */
	if(0 != gpio_request(MITY335X_DK_GPIO_BACKLIGHT, "backlight control")) {
		pr_warning("Unable to request GPIO %d\n",MITY335X_DK_GPIO_BACKLIGHT);
		goto out;
	}
	if(0 != gpio_direction_output(MITY335X_DK_GPIO_BACKLIGHT, 1) ){
		pr_warning("Unable to set backlight GPIO %d ON\n",
				   MITY335X_DK_GPIO_BACKLIGHT);
		goto out;
	} else {
		pr_info("Backlight GPIO  = %d\n", MITY335X_DK_GPIO_BACKLIGHT);
	}
#endif // CONFIG_BACKLIGHT_TPS6116X
out:
	clk_put(disp_pll);
}



static u8 am335x_iis_serializer_direction[] = {
	TX_MODE,	RX_MODE,	INACTIVE_MODE,	INACTIVE_MODE,
	INACTIVE_MODE,	INACTIVE_MODE,	INACTIVE_MODE,	INACTIVE_MODE,
	INACTIVE_MODE,	INACTIVE_MODE,	INACTIVE_MODE,	INACTIVE_MODE,
	INACTIVE_MODE,	INACTIVE_MODE,	INACTIVE_MODE,	INACTIVE_MODE,
};

static struct snd_platform_data baseboard_snd_data = {
	.tx_dma_offset	= 0x46400000,	/* McASP1 */
	.rx_dma_offset	= 0x46400000,
	.op_mode	= DAVINCI_MCASP_IIS_MODE,
	.num_serializer	= ARRAY_SIZE(am335x_iis_serializer_direction),
	.tdm_slots	= 2,
	.serial_dir	= am335x_iis_serializer_direction,
	.asp_chan_q	= EVENTQ_2,
	.version	= MCASP_VERSION_3,
	.txnumevt	= 1,
	.rxnumevt	= 1,
};

static struct omap2_mcspi_device_config spi0_ctlr_data = {
	.turbo_mode = 0,	/* diable "turbo" mode */
	.d0_is_mosi = 1,	/* D0 is output from 3335X */
};

static struct spi_board_info baseboard_spi0_slave_info[] = {
	{
		.modalias	= "tlv320aic26-codec",
		.controller_data = &spi0_ctlr_data,
		.irq		= -1,
		.max_speed_hz	= 2000000,
		.bus_num	= 1,
		.chip_select	= 1,
		.mode		= SPI_MODE_1,
	},
#if (TS_USE_SPI)
	{
		.modalias			= "ads7846",
		.bus_num			= MITY335X_DK_SPIBUS_TS,
		.chip_select		= 0,
		.max_speed_hz		= 1500000,
		.controller_data	= &spi0_ctlr_data,
		.irq				= OMAP_GPIO_IRQ(MITY335X_DK_GPIO_TS_IRQ_N),
		.platform_data		= &ads7846_config,
	}
#endif /* TS_USE_SPI */
};

static __init void baseboard_setup_audio(void)
{
	pr_info("Configuring audio...\n");
	setup_pin_mux(mcasp1_pin_mux);
	am335x_register_mcasp(&baseboard_snd_data, 1);
}


static __init void baseboard_setup_spi0_devices(void)
{
	setup_pin_mux(spi0_pin_mux);
	spi_register_board_info(baseboard_spi0_slave_info,
			ARRAY_SIZE(baseboard_spi0_slave_info));

	baseboard_setup_audio();
	baseboard_setup_ts();

}


static void __init baseboard_i2c0_init(void)
{
	setup_pin_mux(i2c0_pin_mux);
	omap_register_i2c_bus(1, 100, NULL, 0);
}

/* fixup for the Vitesse 8601 PHY on the MityARM335x dev kit.
 * We need to adjust the recv clock skew to recenter the data eye.
 */
static int am335x_vsc8601_phy_fixup(struct phy_device *phydev)
{
	unsigned int val;

	pr_info("am335x_vsc8601_phy_fixup %x here addr = %d\n",
			phydev->phy_id, phydev->addr);

	/* skew control is in extended register set */
	if (phy_write(phydev,  MII_EXTPAGE, 1) < 0) {
		pr_err("Error enabling extended PHY regs\n");
		return 1;
	}
	/* read the skew */
	val = phy_read(phydev, RGMII_SKEW);
	if (val < 0) {
		pr_err("Error reading RGMII skew reg\n");
		return val;
	}
	val &= 0x0FFF; /* clear skew values */
	val |= 0x3000; /* 0 Tx skew, 2.0ns Rx skew */
	if (phy_write(phydev, RGMII_SKEW, val) < 0) {
		pr_err("failed to write RGMII_SKEW\n");
		return 1;
	}
	/* disable the extended page access */
	if (phy_write(phydev, MII_EXTPAGE, 0) < 0) {
		pr_err("Error disabling extended PHY regs\n");
		return 1;
	}
	return 0;
}

static __init void baseboard_setup_enet(void)
{
	/* pinmux */
	setup_pin_mux(rgmii2_pin_mux);

	/* network configuration done in SOM code */
	/* PHY address setup? */
	/* Register PHY fixup to adjust rx clock skew */
	phy_register_fixup_for_uid(VSC8601_PHY_ID,
				VSC8601_PHY_MASK,
				am335x_vsc8601_phy_fixup);
}


static __init int baseboard_init(void)
{
	pr_info("%s [%s]...\n", __func__, BASEBOARD_NAME);

	baseboard_setup_enet();

	baseboard_setup_mmc();

	baseboard_setup_usb();

	baseboard_setup_dvi();

	baseboard_setup_can();

	baseboard_setup_spi0_devices();

	baseboard_i2c0_init();

	return 0;
}
arch_initcall_sync(baseboard_init);

