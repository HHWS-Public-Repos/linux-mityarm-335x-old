[#!/bin/bash

# Build zImage, dtbs, and modules
# Assumes toolchain has been preinstalled

MDKDATE="2023-03-13"
source /opt/criticallink/mitysom-335x_$MDKDATE/environment-setup-cortexa8t2hf-neon-criticallink-linux-gnueabi 

set -x
set -e

BUILDDIR=$PWD/build-mitysom335x

# Updating defconfig
# make O="$BUILDDIR" menuconfig
# make O="$BUILDDIR" savedefconfig
# mv $BUILDDIR/defconfig arch/arm/configs/mitysom-335x-devkit_defconfig

make O="$BUILDDIR" mitysom-335x-harmony_defconfig
make -j"$(nproc)" O="$BUILDDIR" zImage dtbs modules
rm -rf "$BUILDDIR/rootfs" # Clean old modules
make -j"$(nproc)" O="$BUILDDIR" INSTALL_MOD_PATH="$BUILDDIR/rootfs" INSTALL_MOD_STRIP=1 modules_install
rm -r "$BUILDDIR"/rootfs/lib/modules/*/{build,source} # Remove symlinks to make easier to copy to SD card


mkimage -f fitImage.its.watts fitImage
