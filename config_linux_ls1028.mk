-include local_config_linux_ls1028.mk

target_arch:=armv8a
target_os:=linux
target_soc:=ls1028

config_list?= bridge

export CROSS_COMPILE?=/home/user/dev/openil/output/host/usr/bin/aarch64-linux-gnu-
export KERNELDIR?=/home/user/dev/openil/output/build/linux-OpenIL-linux-201908
export STAGING_DIR?=/home/user/dev/openil/output/staging
export ARCH=arm64

BIN_DIR?=$(PREFIX)/usr/bin
LIB_DIR?=$(PREFIX)/usr/lib
INCLUDE_DIR?=$(PREFIX)/usr/include
CONFIG_DIR?=$(PREFIX)/etc/genavb
INITSCRIPT_DIR?=$(PREFIX)/etc/init.d
FIRMWARE_DIR?=$(PREFIX)/lib/firmware/genavb
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

