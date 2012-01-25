/*
 * Critical Link MityARM-335x SoM Development Kit Baseboard Initialization File
 *
 * Someday... Someday... most of this should be replaced with device tree....
 */
#include <linux/kernel.h>
#include <linux/init.h>

#include "mux.h"

#define BASEBOARD_NAME "MityARM-335x DevKit"

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

static __init void baseboard_setup_can(void)
{
}

static __init void baseboard_setup_usb(void)
{
}

static __init void baseboard_setup_mmc(void)
{
}

static __init void baseboard_setup_dvi(void)
{
	/* pinmux */

	/* add I2C1 device entry */

	/* configure / enable LCDC */

	/* backlight */
}

static __init void baseboard_setup_audio(void)
{
}

static __init void baseboard_setup_enet(void)
{
	/* pinmux */
	setup_pin_mux(rgmii2_pin_mux);

	/* network configuration done in SOM code */
}

static __init int baseboard_init(void)
{
	pr_info("%s [%s]...\n", __func__, BASEBOARD_NAME);

	baseboard_setup_enet();

	baseboard_setup_audio();

	baseboard_setup_mmc();

	baseboard_setup_dvi();

	baseboard_setup_usb();

	baseboard_setup_can();

	return 0;
}
arch_initcall_sync(baseboard_init);

