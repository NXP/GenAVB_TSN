-include local_config_linux_imx6.mk

target_arch:=armv7a
target_os:=linux
target_soc:=imx6

config_list?= endpoint_avb endpoint_tsn hybrid bridge endpoint_gptp_only

export CROSS_COMPILE?=/home/user/dev/gcc-linaro-arm-linux-gnueabihf-4.8-2014.04_linux/bin/arm-linux-gnueabihf-
export KERNELDIR?=/home/user/dev/linux_avb
export STAGING_DIR?=${SDKTARGETSYSROOT}
export NXP_SWITCH_PATH?=/home/user/dev/nxp_switch

BIN_DIR?=$(PREFIX)/usr/bin
LIB_DIR?=$(PREFIX)/usr/lib
INCLUDE_DIR?=$(PREFIX)/usr/include
CONFIG_DIR?=$(PREFIX)/etc/genavb
INITSCRIPT_DIR?=$(PREFIX)/etc/init.d
DEVKIT_DIR?=$(shell pwd)/build/devkit

# Add a space-separated defines
APPS_CUSTOM_DEFINES=WL_BUILD

# Use already set CC/LD/AR when building using yocto environment
ifneq "$(origin CC)" "default"
$(info Compiling using preset CC=$(CC))
else
CC:=$(CROSS_COMPILE)gcc
LD:=$(CROSS_COMPILE)ld
AR:=$(CROSS_COMPILE)ar
OBJCOPY:=$(CROSS_COMPILE)objcopy
endif

