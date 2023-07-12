#!/bin/bash
MDKDATE="2023-03-13"
source /opt/criticallink/mitysom-335x_$MDKDATE/environment-setup-cortexa8t2hf-neon-criticallink-linux-gnueabi 

set -x
set -e



make mitysom-335x-harmony_defconfig

make -j$(nproc)  zImage modules dtbs
mkimage -f fitImage.its.watts fitImage

if [ -a /run/media/bob/boot/MLO ]
then   
   cp fitImage /run/media/bob/boot/
   sudo -E bash -c "source /opt/criticallink/mitysom-335x_$MDKDATE/environment-setup-cortexa8t2hf-neon-criticallink-linux-gnueabi; make INSTALL_MOD_PATH=/run/media/bob/root/  modules_install"
   sudo umount /run/media/bob/*
fi;
   
   
