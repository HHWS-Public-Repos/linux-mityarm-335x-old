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

#include <video/da8xx-fb.h>
#include <plat/lcdc.h> /* uhhggg... */
#include <plat/mmc.h>
#include <plat/usb.h>
#include <plat/omap_device.h>

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
	{"usb1_drvvbus.gpio3_13", AM33XX_PULL_ENBL}, 	   /* Exp0 TX Enable */
	{"mii1_rxd3.uart3_rxd", AM33XX_PIN_INPUT_PULLUP},  /* Exp1 RX */
	{"mii1_rxd2.uart3_txd", AM33XX_PULL_ENBL},	   /* Exp1 TX */
	{"mcasp0_aclkx.gpio4_14", AM33XX_PULL_ENBL}, 	   /* Exp1 TX Enable */
	{NULL, 0}
};

static struct pinmux_config usb_pin_mux[] = {
	{"usb0_drvvbus.usb0_drvvbus",	AM33XX_PIN_OUTPUT},
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
	.instances	= 2,
};

static struct musb_hdrc_config musb_config = {
	.fifo_mode	= 4,
	.multipoint	= 1,
	.dyn_fifo	= 1,
	.num_eps	= 16,
	.ram_bits	= 12,
};

static struct musb_hdrc_platform_data musb_plat[] = {
	{
		.config		= &musb_config,
		.clock		= "ick",
		.board_data	= &board_data,
		.power		= 500 / 2,
		.mode		= MUSB_OTG,
		.extvbus	= 1,
	},
	{
		.config		= &musb_config,
		.clock		= "ick",
		.board_data	= &board_data,
		.power		= 500 / 2,
		.mode		= MUSB_HOST,
		.extvbus	= 1,
	},
};

static u64 musb_dmamask = DMA_BIT_MASK(32);

static __init void baseboard_setup_usb(void)
{
	struct omap_hwmod               *oh;
	struct platform_device          *pdev;
	struct device                   *dev;

	setup_pin_mux(usb_pin_mux);

	/*
	 * TODO - there is usb_musb_init() routine in usb-musb.c, but it
	 * doesn't allow full configuration of the MODE for each of the
	 * ports, and there is some other chicanery in there for other
	 * platforms that is a bit scary....
	 */
	oh = omap_hwmod_lookup("usb_otg_hs");
	if (WARN(!oh, "%s: could not find omap_hwmod for usb_otg_hs\n",
		__func__))
		return;

	pdev = omap_device_build("ti81xx-usbss", -1, oh, &musb_plat,
		sizeof(musb_plat), NULL, 0, false);
	if (IS_ERR(pdev)) {
		pr_err("Could not build omap_device for ti81xx-usbss\n");
		return;
	}

	dev = &pdev->dev;
	get_device(dev);
	dev->dma_mask = &musb_dmamask;
	dev->coherent_dma_mask = musb_dmamask;
	put_device(dev);
}

static __init void baseboard_setup_mmc(void)
{
	/* pin mux */
	setup_pin_mux(mmc0_pin_mux);

	/* configure mmc */
	omap2_hsmmc_init(mmc_info);
}

static const struct display_panel disp_panel = {
	WVGA,
	16,
	16,
	COLOR_ACTIVE,
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

static __init void baseboard_setup_audio(void)
{
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

#if 0   /* MAW - BROKEN */
	baseboard_setup_usb();
#endif

	baseboard_setup_expansion();

	baseboard_setup_dvi();

	baseboard_setup_can();

	baseboard_setup_audio();

	return 0;
}
arch_initcall_sync(baseboard_init);

