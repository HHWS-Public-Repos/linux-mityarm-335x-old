#!/bin/bash

# Push zImage, device tree, uEnv.txt, and modules to board over network

BUILDDIR=build-mitysom335x
HOST=${1:-usb}
SSH_CMD="ssh -o UserKnownHostsFile=/dev/null -o StrictHostKeyChecking=no"

# example .ssh/config
# Host usb
#   HostName 10.1.47.2
#   Port 22
#   User root
#   StrictHostKeyChecking no
#   ForwardAgent no
#   ForwardX11 no

set -x
set -e

rsync --backup -e "$SSH_CMD" "$BUILDDIR"/arch/arm/boot/dts/am335x-mitysom-devkit.dtb "$HOST":/run/media/mmcblk0p1/
rsync --backup -e "$SSH_CMD" "$BUILDDIR"/arch/arm/boot/zImage "$HOST":/run/media/mmcblk0p1/
# To prevent the kernel modules from every push from filling up the rootfs
#  create a main kernel version directory and symlink the build specific
#  versions to it
# Note it may be necessary to manually copy the extra folder from the older
#  lib/modules directory on the device to keep the gpu driver working
KERN_NEW=$(cat "$BUILDDIR"/include/config/kernel.release)
# Ex: 5.10.158-00101-gff367f18baa3
KERN_MAIN=$(echo "$KERN_NEW" | cut -d- -f1)
#  Becomes: 5.10.158
$SSH_CMD "$HOST" mkdir -p "/lib/modules/$KERN_MAIN"
$SSH_CMD "$HOST" ln -sf --no-dereference "/lib/modules/$KERN_MAIN" "/lib/modules/$KERN_NEW"
rsync -e "$SSH_CMD" -aP "$BUILDDIR/rootfs/lib/modules/$KERN_NEW/" "$HOST:/lib/modules/$KERN_NEW"
