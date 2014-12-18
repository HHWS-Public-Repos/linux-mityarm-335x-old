/*
 * Configuration and important values for CL MityARM-335x boards
 *
 * Copyright (C) 2013 Critical Link LLC
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/types.h>
#include "hsmmc.h"

#define FACTORY_CONFIG_MAGIC    0x012C0138
#define CONFIG_I2C_VERSION_1_1	0x00010001 /* prior to DDR3 configurations */
#define CONFIG_I2C_VERSION_1_2	0x00010002
#define FACTORY_CONFIG_VERSION	CONFIG_I2C_VERSION_1_2

/**
 *  Model numbering scheme:
 *  PPPP-YX-NAR-HC[-OO]
 *
 *  PPPP	- Part number (3359, 3354, etc.)
 *  Y		- Speed Grade (E or G - 720 MHz, H - 800MHz, I - 1GHz)
 *  X		- not used (fpga type)
 *  N		- NOR size (3 - 16 MB, 2 - 8MB)
 *  A		- NAND size (2 - 256 MB, 3 - 512 MB, 4 - 1GB)
 *  R           - RAM size (6 - 256 MB DDR2, 7 - 256 MB DDR3, 8 - 512 MB DDR3,
 *                          9 - 512MB DDR2, A - 1024 MB DDR3)
 *  H		- RoHS (R - compliant)
 *  C		- Temperature (C - commercial, I - industrial, L - Low Temp)
 *  -OO         - Option builds. Currently the only option is -R2 for the TiWi module
 */

#define SPEED_GRADE_POSITION	 5
#define SPEED_GRADE_720		'G'
#define SPEED_GRADE_800		'H'
#define SPEED_GRADE_1000	'I'

#define NOR_SIZE_POSITION	 8
#define NOR_SIZE_NONE		'X'
#define NOR_SIZE_2MB		'1'
#define NOR_SIZE_8MB		'2'
#define NOR_SIZE_16MB		'3'

#define NAND_SIZE_POSITION	 9
#define NAND_SIZE_NONE		'X'
#define NAND_SIZE_256MB		'2'
#define NAND_SIZE_512MB		'3'
#define NAND_SIZE_1GB		'4'

#define RAM_SIZE_POSITION	10
#define RAM_SIZE_256MB_DDR2	'6'
#define RAM_SIZE_256MB_DDR3	'7'
#define RAM_SIZE_512MB_DDR3	'8'
#define RAM_SIZE_512MB_DDR2	'9'
#define RAM_SIZE_1GB_DDR3	'A'

#define OPTION_POSITION         14
#define OPTION_TIWI             "-R2"
#define OPTION_TIWI_BLE         "-BLE"

struct mityarm335x_factory_config {
	u32	magic;
	u32	version;
	u8	mac[6];
	u32	reserved;
	u32	spare;
	u32	serialnumber;
	char	partnum[32];
};

bool mityarm335x_valid_magic(void);
bool mityarm335x_valid_version(void);
u32 mityarm335x_serial_number(void);
const char* mityarm335x_model_number(void);
const u8* mityarm335x_mac_addr(void);

const struct mityarm335x_factory_config* mityarm335x_factoryconfig(void);

/**
 * Function called when board file has read factory config to allow baseboard file
 * to add baseboard-specific processing.
 */
extern void mityarm335x_set_config_callback(
		void (*mityarm335x_config_callback)(const struct  mityarm335x_factory_config*));

bool mityarm335x_has_tiwi(void);
size_t mityarm335x_ram_size(void);
size_t mityarm335x_nand_size(void);
size_t mityarm335x_nor_size(void);
u32 mityarm335x_speed_grade(void);
int mityarm335x_som_mmc_fixup(struct omap2_hsmmc_info* devinfo);
