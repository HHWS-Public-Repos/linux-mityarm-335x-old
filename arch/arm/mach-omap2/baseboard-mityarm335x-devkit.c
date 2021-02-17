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
#include <linux/delay.h>
#include <linux/gpio.h>

/* TSc controller */
#include <linux/input/ti_tsc.h>
#include <linux/platform_data/ti_adc.h>
#include <linux/mfd/ti_tscadc.h>
#include <linux/lis3lv02d.h>

#include <video/da8xx-fb.h>
#include <plat/omap-pm.h>
#include <plat/lcdc.h> /* uhhggg... */
#include <plat/mmc.h>
#include <plat/usb.h>
#include <plat/omap_device.h>
#include <plat/mcspi.h>
#include <plat/i2c.h>

#include <linux/wl12xx.h>
#include <linux/regulator/fixed.h>
#include <linux/regulator/machine.h>
#include <linux/mmc/host.h>
#include <linux/pwm/pwm.h>

#include <asm/hardware/asp.h>

#include "mux.h"
#include "devices.h"
#include "mityarm335x.h"

#define BASEBOARD_NAME "MitySOM-335x DevKit"
/* Vitesse 8601 register defs we need... */
#define VSC8601_PHY_ID   (0x00070420)
#define VSC8601_PHY_MASK (0xFFFFFFFC)
#define MII_EXTPAGE		 (0x1F)
#define RGMII_SKEW		 (0x1C)
#define MITY335X_DK_SPIBUS_TS (1)

/* TODO - refactor all the pinmux stuff for all board files to use */
#define GPIO_TO_PIN(bank, gpio) (32 * (bank) + (gpio))

#define MITY335X_DK_GPIO_TS_IRQ_N	GPIO_TO_PIN(0, 20)
#define MITY335X_DK_GPIO_BACKLIGHT	GPIO_TO_PIN(3, 14)


#if defined(CONFIG_TOUCHSCREEN_ADS7846) || \
	defined(CONFIG_TOUCHSCREEN_ADS7846_MODULE)
#define TS_USE_SPI 0 /*1 -- currently not supported*/
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

#ifndef CONFIG_MITYARM335X_TIWI
static int mmc2_wl12xx_init(void);
#endif

/******************************************************************************
 *
 *                                N O T E
 *
 *                       PUT ALL PINMUX SETTINGS HERE
 *
 *****************************************************************************/

static struct pinmux_config __initdata rgmii2_pin_mux[] = {
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

static struct pinmux_config __initdata lcdc_pin_mux[] = {
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

static struct pinmux_config __initdata mmc0_pin_mux[] = {
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

/**
 * Expansion connector pins for SDIO
 * Pin	WifiJ1		DevJ700		335X PIN/Function
 * 1	SDIO_D3		GPMC_AD15	GPMC_AD15/MMC2_DAT3
 * 3	SDIO_D2		GPMC_AD14	GPMC_AD14/MMC2_DAT2
 * 5	SDIO_D1		GPMC_AD13	GPMC_AD13/MMC2_DAT1
 * 7	SDIO_D0		GPMC_AD12	GPMC_AD12/MMC2_DAT0
 * 9	RESET		GPMC_AD11	GPMC_AD11/GPIO2_27
 * 22	SDIO_CMD	GPMC_CS3_N	GPMC_CSN3/MMC2_CMD
 * 30	SDIO_CLK	GPMC_CLK	GPMC_CLK/MMC2_CLK
 * Note: Not used for TiWi Module
 */
static struct pinmux_config __initdata __maybe_unused mmc2_pin_mux[] = {
	{"gpmc_ad15.mmc2_dat3", AM33XX_PIN_INPUT_PULLUP},
	{"gpmc_ad14.mmc2_dat2", AM33XX_PIN_INPUT_PULLUP},
	{"gpmc_ad13.mmc2_dat1", AM33XX_PIN_INPUT_PULLUP},
	{"gpmc_ad12.mmc2_dat0", AM33XX_PIN_INPUT_PULLUP},
	{"gpmc_clk.mmc2_clk",   AM33XX_PIN_INPUT_PULLUP},
	{"gpmc_csn3.mmc2_cmd",   AM33XX_PIN_INPUT_PULLUP},
	{NULL, 0}
};


static struct pinmux_config __initdata can0_pin_mux[] = {
	{"mii1_txd3.d_can0_tx", AM33XX_PULL_ENBL},
	{"mii1_txd2.d_can0_rx", AM33XX_PIN_INPUT_PULLUP},
	{NULL, 0}
};

/* Note: Not used for TiWi module */
static struct pinmux_config __initdata __maybe_unused can1_pin_mux[] = {
	{"uart1_rxd.d_can1_tx", AM33XX_PULL_ENBL},
	{"uart1_txd.d_can1_rx", AM33XX_PIN_INPUT_PULLUP},
	{NULL, 0}
};

static struct pinmux_config __initdata expansion_pin_mux[] = {
	{"uart0_ctsn.uart4_rxd", AM33XX_PIN_INPUT_PULLUP}, /* Exp0 RX */
	{"uart0_rtsn.uart4_txd", AM33XX_PULL_ENBL}, /* Exp0 TX */
	{"mii1_rxd3.uart3_rxd", AM33XX_PIN_INPUT_PULLUP}, /* Exp1 RX */
	{"mii1_rxd2.uart3_txd", AM33XX_PULL_ENBL}, /* Exp1 TX */
	{"mii1_rxd1.gpio2_20", AM33XX_PULL_ENBL}, /* Exp1 TX EN */
	{"mii1_txclk.gpio3_9", AM33XX_PULL_ENBL}, /* Exp0 TX EN */
	{NULL, 0}
};

static struct pinmux_config __initdata usb_pin_mux[] = {
	{"usb0_drvvbus.usb0_drvvbus",	AM33XX_PIN_OUTPUT},
	{"usb1_drvvbus.usb1_drvvbus",	AM33XX_PIN_OUTPUT},
	{NULL, 0}
};

#if (TS_USE_SPI)
static struct pinmux_config __initdata ts_pin_mux[] = {
	/* SPI0 CS0 taken care of by SPI pinmux setup */
	{"xdma_event_intr1.gpio0_20", AM33XX_PIN_INPUT}, /* Pen down */
	{"xdma_event_intr0.gpio0_19", AM33XX_PIN_INPUT}, /* 7843 busy (not used)*/
	{NULL, 0}
};
#endif

/* Module pin mux for mcasp1 */
static struct pinmux_config __initdata mcasp1_pin_mux[] = {
	{"mcasp0_aclkr.mcasp1_aclkx", AM33XX_PIN_INPUT_PULLDOWN},
	{"mcasp0_fsr.mcasp1_fsx", AM33XX_PIN_INPUT_PULLDOWN},
	{"mcasp0_axr1.mcasp1_axr0", AM33XX_PIN_OUTPUT},
	{"mcasp0_ahclkx.mcasp1_axr1", AM33XX_PIN_INPUT_PULLDOWN},
	{"mii1_rxd0.mcasp1_ahclkr", AM33XX_PIN_INPUT_PULLDOWN},
	{"rmii1_refclk.mcasp1_ahclkx", AM33XX_PIN_INPUT_PULLDOWN},
	{NULL, 0},
};


static struct pinmux_config __initdata i2c0_pin_mux[] = {
	{"i2c0_sda.i2c0_sda",	AM33XX_SLEWCTRL_SLOW | AM33XX_PULL_ENBL |
				AM33XX_INPUT_EN | AM33XX_PIN_OUTPUT},
	{"i2c0_scl.i2c0_scl",	AM33XX_SLEWCTRL_SLOW | AM33XX_PULL_ENBL |
				AM33XX_INPUT_EN | AM33XX_PIN_OUTPUT},
	{NULL, 0},
};

static struct pinmux_config __initdata spi0_pin_mux[] = {
	{"spi0_cs0.spi0_cs0", AM33XX_PIN_OUTPUT_PULLUP},
	{"spi0_cs1.spi0_cs1", AM33XX_PIN_OUTPUT_PULLUP},
	// spi clk needs to be an input. Pull should be down for spi modes 0 and 1
	// https://e2e.ti.com/support/arm/sitara_arm/f/791/p/649334/2395792#2395792
	{"spi0_sclk.spi0_sclk", AM33XX_PIN_INPUT_PULLDOWN},
	{"spi0_d0.spi0_d0", AM33XX_PIN_OUTPUT},
	{"spi0_d1.spi0_d1", AM33XX_PIN_INPUT_PULLDOWN},
	{NULL, 0},
};

#ifndef CONFIG_MITYARM335X_TIWI
static struct pinmux_config __initdata wl12xx_pin_mux[] = {
	{"gpmc_ad10.gpio0_26",  AM33XX_PIN_INPUT}, /* WL WL IRQ */
	{"gpmc_ad11.gpio0_27",  AM33XX_PIN_INPUT}, /* WL SPI I/O RST */
	{"gpmc_csn1.gpio1_30",  AM33XX_PIN_INPUT_PULLUP}, /* WL IRQ */
	{"gpmc_csn2.gpio1_31",  AM33XX_PIN_OUTPUT}, /* BT RST ?*/
	{NULL, 0},
};

#define AM335XEVM_WLAN_IRQ_GPIO		GPIO_TO_PIN(0, 26)

struct wl12xx_platform_data am335x_wlan_data = {
	.irq = OMAP_GPIO_IRQ(AM335XEVM_WLAN_IRQ_GPIO),
	.board_ref_clock = WL12XX_REFCLOCK_38_XTAL, /* 38.4Mhz */
};
#endif

static void __init baseboard_setup_expansion(void)
{
	setup_pin_mux(expansion_pin_mux);
}

static void __init baseboard_setup_can(void)
{
	setup_pin_mux(can0_pin_mux);
	am33xx_d_can_init(0);

#ifndef CONFIG_MITYARM335X_TIWI
	if(!mityarm335x_has_tiwi()) {
		setup_pin_mux(can1_pin_mux);
		am33xx_d_can_init(1);
	}
#endif
}

static struct omap_musb_board_data board_data = {
	.interface_type	= MUSB_INTERFACE_ULPI,
	.mode           = MUSB_OTG,
	.power		= 500,
	.instances	= 1,
};

static void __init baseboard_setup_usb(void)
{
	setup_pin_mux(usb_pin_mux);
	usb_musb_init(&board_data);
}

static struct omap2_hsmmc_info mmc_info[] = {
	{
		.mmc		= 1,
		.caps		= MMC_CAP_4_BIT_DATA,
		.gpio_cd	= -EINVAL, /* Disable card detect, this allows booting boards with microsd slots */
		.gpio_wp	= -EINVAL, /* Support booting boards with microsd */
		.ocr_mask	= MMC_VDD_32_33 | MMC_VDD_33_34,
	},
	{
		.mmc            = 0,	/* will be set at runtime */
	},
	{
		.mmc            = 0,	/* will be set at runtime */
	},
	{} /* Terminator */
	};

static void __init baseboard_setup_mmc(void)
{

#ifndef CONFIG_MITYARM335X_TIWI
	mmc2_wl12xx_init();
#endif

	/* pin mux */
	setup_pin_mux(mmc0_pin_mux);

	/* configure mmc */
	mityarm335x_som_mmc_fixup(mmc_info);
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
	.type			= "1024x768",
};
#ifdef CONFIG_BACKLIGHT_TPS6116X

static struct platform_device tps6116x_device = {
	.name   = "tps6116x",
	.id     = -1,
	.dev    = {
	    .platform_data  = (void *)MITY335X_DK_GPIO_BACKLIGHT,
	},
};
#endif /* CONFIG_BACKLIGHT_TPS6116X */


#if (TS_USE_SPI)
static struct ads7846_platform_data ads7846_config = {
	.model			= 7843,
	.vref_mv		= 3300,
	.x_max			= 0x0fff,
	.y_max			= 0x0fff,
	.x_plate_ohms		= 180,
	.pressure_max		= 255,
	.debounce_max		= 0, /* 200, */
	.debounce_tol		= 5,
	.debounce_rep		= 10,
	.gpio_pendown		= MITY335X_DK_GPIO_TS_IRQ_N,
	.keep_vref_on		= 1,
	.irq_flags		= IRQF_TRIGGER_FALLING,
	.vref_delay_usecs	= 100,
	.settle_delay_usecs	= 200,
	.penirq_recheck_delay_usecs = 1000,
	.filter_init		= 0,
	.filter			= 0,
	.filter_cleanup		= 0,
	.gpio_pendown		= MITY335X_DK_GPIO_TS_IRQ_N,
};

static void __init baseboard_setup_ts(void)
{
	setup_pin_mux(ts_pin_mux);
	/* SPI hookup already done by baseboard_setup_spi0() */
}
#else

static struct tsc_data am335x_touchscreen_data  = {
	.wires  = 4,
	.x_plate_resistance = 200,
	.steps_to_configure = 5,
};

static struct adc_data am335x_adc_data = {
	.adc_channels = 4,
};

static struct mfd_tscadc_board am335x_tscadc = {
	.tsc_init = &am335x_touchscreen_data,
	.adc_init = &am335x_adc_data,
};


static void __init baseboard_setup_ts(void)
{
	int err;

	/* Removed Pin Mux -- Analog pins don't require it */

	pr_info("IN : %s \n", __FUNCTION__);
	err = am33xx_register_mfd_tscadc(&am335x_tscadc);
	if (err)
		pr_err("failed to register touchscreen device\n");

	pr_info("Setup LCD touchscreen\n");
}
#endif /* CONFIG_TOUCHSCREEN_ADS7846 */

static int __init conf_disp_pll(int rate)
{
	struct clk *disp_pll;
	int ret = -EINVAL;

	disp_pll = clk_get(NULL, "dpll_disp_ck");
	if (IS_ERR(disp_pll)) {
		pr_err("Cannot clk_get disp_pll\n");
		goto out;
	}

	ret = clk_set_rate(disp_pll, rate);
	clk_put(disp_pll);
out:
	return ret;
}

static void __init baseboard_setup_dvi(void)
{
	/* pinmux */
	setup_pin_mux(lcdc_pin_mux);

	/* add I2C1 device entry */

	/* TODO - really need to modify da8xx driver to support mating to the
	 * TFP410 and tweaking settings at the driver level... need to stew on
	 * this..
	 */

	/* configure / enable LCDC */
	if (conf_disp_pll(300000000)) {
		pr_info("Failed configure display PLL, not attempting to"
				"register LCDC\n");
		return;
	}

	dvi_pdata.get_context_loss_count = omap_pm_get_dev_context_loss_count;

	if (am33xx_register_lcdc(&dvi_pdata))
		pr_warning("%s: Unable to register LCDC device.\n",
			__func__);

#ifdef CONFIG_BACKLIGHT_TPS6116X
	if (platform_device_register(&tps6116x_device))
		pr_err("failed to register backlight device\n");

#else
	/* backlight */
	/* TEMPORARY until driver is ready... just jam it on! */
	if (0 != gpio_request(MITY335X_DK_GPIO_BACKLIGHT, "backlight control")) {
		pr_warning("Unable to request GPIO %d\n",
				   MITY335X_DK_GPIO_BACKLIGHT);
		return;
	}
	if (0 != gpio_direction_output(MITY335X_DK_GPIO_BACKLIGHT, 1)) {
		pr_warning("Unable to set backlight GPIO %d ON\n",
				   MITY335X_DK_GPIO_BACKLIGHT);
		return;
	} else {
		pr_info("Backlight GPIO  = %d\n", MITY335X_DK_GPIO_BACKLIGHT);
	}
#endif /* CONFIG_BACKLIGHT_TPS6116X */
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
		.modalias	= "ads7846",
		.bus_num	= MITY335X_DK_SPIBUS_TS,
		.chip_select	= 0,
		.max_speed_hz	= 1500000,
		.controller_data = &spi0_ctlr_data,
		.irq		= OMAP_GPIO_IRQ(MITY335X_DK_GPIO_TS_IRQ_N),
		.platform_data	= &ads7846_config,
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

static void __init baseboard_setup_enet(void)
{
	/* pinmux */
	setup_pin_mux(rgmii2_pin_mux);

	am33xx_cpsw_init(AM33XX_CPSW_MODE_RGMII, "0:00", NULL);

	/* network configuration done in SOM code */
	/* PHY address setup? */
	/* Register PHY fixup to adjust rx clock skew */
	phy_register_fixup_for_uid(VSC8601_PHY_ID,
				VSC8601_PHY_MASK,
				am335x_vsc8601_phy_fixup);
}

#ifndef CONFIG_MITYARM335X_TIWI
static void wl12xx_bluetooth_enable(void)
{
	if (am335x_wlan_data.bt_enable_gpio != -EINVAL) {
		int status = gpio_request(am335x_wlan_data.bt_enable_gpio,
			"bt_en\n");
		if (status < 0)
			pr_err("Failed to request gpio for bt_enable");

		pr_info("Configure Bluetooth Enable pin...\n");
		gpio_direction_output(am335x_wlan_data.bt_enable_gpio, 0);
	} else {
		pr_info("Bluetooth not Enabled!\n");
	}
}

static int wl12xx_set_power(struct device *dev, int slot, int on, int vdd)
{
	if (on) {
		gpio_set_value(am335x_wlan_data.wlan_enable_gpio, 1);
		mdelay(70);
	} else
		gpio_set_value(am335x_wlan_data.wlan_enable_gpio, 0);

	return 0;
}

static int __init baseboard_setup_wlan(void)
{
	struct device *dev = NULL;
	struct omap_mmc_platform_data *pdata;
	int ret = -1;


	/* Register WLAN and BT enable pins based on the evm board revision */
	am335x_wlan_data.wlan_enable_gpio = GPIO_TO_PIN(3, 4);
	am335x_wlan_data.bt_enable_gpio = -EINVAL;

	pr_info("WLAN GPIO Info.. IRQ = %3d WL_EN = %3d BT_EN = %3d\n",
			am335x_wlan_data.irq,
			am335x_wlan_data.wlan_enable_gpio,
			am335x_wlan_data.bt_enable_gpio);

	wl12xx_bluetooth_enable();

	if (wl12xx_set_platform_data(&am335x_wlan_data))
		pr_err("error setting wl12xx data\n");

	dev = mmc_info[1].dev;
	if (!dev) {
		pr_err("wl12xx mmc device initialization failed\n");
		goto out;
	}

	pdata = dev->platform_data;
	if (!pdata) {
		pr_err("Platfrom data of wl12xx device not set\n");
		goto out;
	}

	ret = gpio_request_one(am335x_wlan_data.wlan_enable_gpio,
		GPIOF_OUT_INIT_LOW, "wlan_en");
	if (ret) {
		pr_err("Error requesting wlan enable gpio: %d\n", ret);
		goto out;
	}


	pdata->slots[0].set_power = wl12xx_set_power;
	pr_info("baseboard_setup_wlan: finished\n");
	ret = 0;
out:
	return ret;

}

/**
 * Configure the MMC2 interface for the baseboard WiFi module
 */
static int __init mmc2_wl12xx_init(void)
{
	/* Don't set up the Wifi for the expansion if the module has it */
	if(mityarm335x_has_tiwi())
		return 0;

	pr_info("Initializing MMC2 WL12xx device\n");

	setup_pin_mux(mmc2_pin_mux);
	setup_pin_mux(wl12xx_pin_mux);

	mmc_info[1].mmc = 3;
	mmc_info[1].name = "wl1271";
	mmc_info[1].caps = MMC_CAP_4_BIT_DATA | MMC_CAP_POWER_OFF_CARD
				| MMC_PM_KEEP_POWER;
	mmc_info[1].nonremovable = true;
	mmc_info[1].gpio_cd = -EINVAL;
	mmc_info[1].gpio_wp = -EINVAL;
	mmc_info[1].ocr_mask = MMC_VDD_32_33 | MMC_VDD_33_34; /* 3V3 */

	return 0;
}
#endif

static void __init factory_config_callback(const struct mityarm335x_factory_config* factory_config)
{
	int ii;

	pr_info("%s: %s\n", BASEBOARD_NAME, __FUNCTION__);

#ifndef CONFIG_MITYARM335X_TIWI
	/* mmc will be initialized when baseboard_setup_mmc is called */
	baseboard_setup_wlan();
#endif

	for(ii = 0; ii < 3; ++ii) {
		const char *name = "no dev";
		struct device *d  = mmc_info[ii].dev;
		if (d) {
			name=dev_name(d);
		}
		pr_debug("%d - MMC %d dev = %s\n",ii,mmc_info[ii].mmc, name);
	}
}

static __init int baseboard_init(void)
{
	pr_info("%s [%s]...\n", __func__, BASEBOARD_NAME);

	mityarm335x_set_config_callback(factory_config_callback);

	baseboard_setup_enet();

	baseboard_setup_mmc();

	baseboard_setup_usb();

	baseboard_setup_dvi();

	baseboard_setup_can();

	baseboard_setup_spi0_devices();

	baseboard_i2c0_init();

	baseboard_setup_expansion();

	return 0;
}
arch_initcall_sync(baseboard_init);
