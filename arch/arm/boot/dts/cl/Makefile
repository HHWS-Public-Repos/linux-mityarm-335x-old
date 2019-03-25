# SPDX-License-Identifier: GPL-2.0
#
# Make file to build device tree binaries for boards based on
# Texas Instruments Inc processors
#
# Copyright (C) 2016-2018 Texas Instruments Incorporated - http://www.ti.com/
#

DTC_FLAGS += -@ -q

dtb-$(CONFIG_SOC_AM33XX) +=    am335x-mitysom-256MB.dtbo \
	am335x-mitysom-256MB-NOR.dtbo \
	am335x-mitysom-512MB.dtbo \
	am335x-mitysom-1GB.dtbo \

$(obj)/%.dtbo: $(src)/%.dtso FORCE
	$(call if_changed_dep,dtc)

always		:= $(dtb-y)
subdir-y	:= $(dts-dirs)
clean-files	:= *.dtb *.dtbo
