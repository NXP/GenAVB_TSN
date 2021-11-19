-include local_config_freertos_rt1176.mk

target_arch:=armv7m
target_os:=freertos
target_soc:=rt1176

config_list?= endpoint_avb endpoint_tsn

FREERTOS_SDK?=/home/user/freertos_avb
FREERTOS_APPS?=/home/user/freertos_avb_apps
FREERTOS_DIR?=$(FREERTOS_SDK)/rtos/freertos/freertos_kernel
FREERTOS_PORT?=GCC/ARM_CM4F
FREERTOS_APP_INCLUDE?=$(FREERTOS_APPS)/boards/src/demo_apps/avb_tsn/common
FREERTOS_BOARD_INCLUDE?=$(FREERTOS_APPS)/boards/evkmimxrt1170/demo_apps/avb_tsn/common
FREERTOS_DEVICE?=$(FREERTOS_SDK)/devices/MIMXRT1176

INCLUDE_DIR?=$(PREFIX)/include
LIB_DIR?=$(PREFIX)/lib
DEVKIT_DIR?=$(shell pwd)/build/freertos_devkit

CFLAGS += -Os -mthumb -mcpu=cortex-m7 -mfloat-abi=hard -mfpu=fpv5-d16 -DCPU_MIMXRT1176DVMAA_cm7 -DENET_ENHANCEDBUFFERDESCRIPTOR_MODE
export CROSS_COMPILE?=/home/user/gcc-arm-none-eabi-6-2017-q2-update/bin/arm-none-eabi-

CC:=$(CROSS_COMPILE)gcc
LD:=$(CROSS_COMPILE)ld
AR:=$(CROSS_COMPILE)ar
OBJCOPY:=$(CROSS_COMPILE)objcopy

