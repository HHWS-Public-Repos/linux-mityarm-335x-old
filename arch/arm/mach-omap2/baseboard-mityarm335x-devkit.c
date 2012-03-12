/*
 * Critical Link MityARM-335x SoM Development Kit Baseboard Initialization File
 *
 * Someday... Someday... most of this should be replaced with device tree....
 */
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/usb/musb.h>
#include <linux/dma-mapping.h>
#include <linux/spi/spi.h>

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

/* TODO - refactor all the pinmux stuff for all board files to use */
#define GPIO_TO_PIN(bank, gpio) (32 * (bank) + (gpio))

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
	{"uart0_ctsn.uart4_rxd", AM33XX_PIN_INPUT_PULLUP}, /* Exp0 RX */
	{"uart0_rtsn.uart4_txd", AM33XX_PULL_ENBL}, 	   /* Exp0 TX */
	{"mii1_rxd3.uart3_rxd", AM33XX_PIN_INPUT_PULLUP},  /* Exp1 RX */
	{"mii1_rxd2.uart3_txd", AM33XX_PULL_ENBL},	   /* Exp1 TX */
	{NULL, 0}
};

static struct pinmux_config usb_pin_mux[] = {
	{"usb0_drvvbus.usb0_drvvbus",	AM33XX_PIN_OUTPUT},
	{"usb1_drvvbus.usb1_drvvbus",	AM33XX_PIN_OUTPUT},
	{NULL, 0}
};

static struct omap2_hsmmc_info mmc_info[] __initdata = {
	{
		.mmc		= 1,
		.caps		= MMC_CAP_4_BIT_DATA,
		.gpio_cd	= GPIO_TO_PIN(3, 3),
		.gpio_wp	= GPIO_TO_PIN(3, 0),
		.ocr_mask	= MMC_VDD_32_33 | MMC_VDD_33_34,
	},
	{}
};

static __init void baseboard_setup_expansion(void)
{
	setup_pin_mux(expansion_pin_mux);
}

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
	.type			= "svga_800x600",
};

static __init void baseboard_setup_dvi(void)
{
	struct clk *disp_pll;

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

	/* backlight */
out:
	clk_put(disp_pll);
}

/* Module pin mux for mcasp1 */
static struct pinmux_config mcasp1_pin_mux[] = {
	{"mcasp0_aclkr.mcasp1_aclkx", AM33XX_PIN_INPUT_PULLDOWN},
	{"mcasp0_fsr.mcasp1_fsx", AM33XX_PIN_INPUT_PULLDOWN},
	{"mcasp0_axr1.mcasp1_axr0", AM33XX_PIN_OUTPUT},
	{"mcasp0_ahclkx.mcasp1_axr1", AM33XX_PIN_INPUT_PULLDOWN},
	{"mii1_rxd0.mcasp1_ahclkr", AM33XX_PIN_INPUT_PULLDOWN},
	{"mii1_refclk.mcasp1_ahclkx", AM33XX_PIN_INPUT_PULLDOWN},
	{NULL, 0},
};

static struct pinmux_config spi0_pin_mux[] = {
	{"spi0_cs0.spi0_cs0", AM33XX_PIN_OUTPUT_PULLUP},
	{"spi0_cs1.spi0_cs1", AM33XX_PIN_OUTPUT_PULLUP},
	{"spi0_sclk.spi0_sclk", AM33XX_PIN_OUTPUT_PULLUP},
	{"spi0_d0.spi0_d0", AM33XX_PIN_OUTPUT},
	{"spi0_d1.spi0_d1", AM33XX_PIN_INPUT_PULLUP},
	{NULL, 0},
};

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

static const struct omap2_mcspi_device_config spi0_ctlr_data = {
	.turbo_mode = 0,
	.d0_is_mosi = 1,
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
	/* TODO -- add touchscreen connector options */
};

static __init void baseboard_setup_audio(void)
{
	pr_info("Configuring audio...\n");
	setup_pin_mux(mcasp1_pin_mux);
	setup_pin_mux(spi0_pin_mux);
	am335x_register_mcasp(&baseboard_snd_data, 1);
	spi_register_board_info(baseboard_spi0_slave_info,
			ARRAY_SIZE(baseboard_spi0_slave_info));
}

static struct pinmux_config i2c0_pin_mux[] = {
	{"i2c0_sda.i2c0_sda",	AM33XX_SLEWCTRL_SLOW | AM33XX_PULL_ENBL |
				AM33XX_INPUT_EN | AM33XX_PIN_OUTPUT},
	{"i2c0_scl.i2c0_scl",	AM33XX_SLEWCTRL_SLOW | AM33XX_PULL_ENBL |
				AM33XX_INPUT_EN | AM33XX_PIN_OUTPUT},
	{NULL, 0},
};

static void __init baseboard_i2c0_init(void)
{
	setup_pin_mux(i2c0_pin_mux);
	omap_register_i2c_bus(1, 100, NULL, 0);
}

static __init void baseboard_setup_enet(void)
{
	/* pinmux */
	setup_pin_mux(rgmii2_pin_mux);

	/* network configuration done in SOM code */
	/* PHY address setup? */
}

static __init int baseboard_init(void)
{
	pr_info("%s [%s]...\n", __func__, BASEBOARD_NAME);

	baseboard_setup_enet();

	baseboard_setup_mmc();

	baseboard_setup_usb();

	baseboard_setup_expansion();

	baseboard_setup_dvi();

	baseboard_setup_can();

	baseboard_setup_audio();

	baseboard_i2c0_init();

	return 0;
}
arch_initcall_sync(baseboard_init);

