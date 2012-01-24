/*
 * Critical Link MityARM-335x SoM Development Kit Baseboard Initialization File
 *
 * Someday... Someday... most of this should be replaced with device tree....
 */
#include <linux/kernel.h>
#include <linux/init.h>

#define BASEBOARD_NAME "MityARM-335x DevKit"

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
	/* add I2C1 device entry */

	/* configure / enable LCDC */

	/* backlight */
}

static __init void baseboard_setup_audio(void)
{
}

static __init void baseboard_setup_enet(void)
{
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

