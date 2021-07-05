# If KERNELRELEASE is not empty, we are being invoked from the KBuild system
ifneq ($(KERNELRELEASE),)

GENAVB_INCLUDE?=$(src)/../../include
CONFIG_FILE?=$(src)/../../config
AUTOCONF?=$(GENAVB_INCLUDE)/genavb/autoconf.h

include $(CONFIG_FILE)

ccflags-y := -I$(GENAVB_INCLUDE) -I$(GENAVB_INCLUDE)/linux
ccflags-y += -include$(AUTOCONF)

obj-m += avb.o

KBUILD_EXTRA_SYMBOLS?=

avb-y = avbdrv.o netdrv.o net_rx.o net_socket.o ipc.o pool.o pool_dma.o net_port.o avtp.o ptp.o mrp.o ipv4.o \
	ipv6.o rtp.o queue.o dmadrv.o cs2000.o gic.o epit.o net_tx.o debugfs.o media.o media_clock.o \
	media_clock_drv.o media_clock_rec_pll.o media_clock_gen_ptp.o imx-pll.o hw_timer.o ftm.o pi.o \
	rational.o mle145170.o gpt.o stats.o net_logical_port.o net_bridge.o mtimer_drv.o mtimer.o \
	sr_class.o qos.o

ifeq ($(CONFIG_SJA1105),y)
avb-y += sja1105.o
KBUILD_EXTRA_SYMBOLS=$(NXP_SWITCH_PATH)/drivers/modules/Module.symvers
HAL_INC_PATH = $(NXP_SWITCH_PATH)/hal/inc
ccflags-y += -I$(NXP_SWITCH_PATH)/drivers/modules -I$(HAL_INC_PATH) -I$(HAL_INC_PATH)/low_level_driver
endif

else

ARCH?=arm
V?=0

INSTALL_ARGS := INSTALL_MOD_PATH=$(PREFIX) INSTALL_MOD_DIR="genavb"
BUILD_ARGS := CROSS_COMPILE="$(CROSS_COMPILE)" V=$(V) ARCH=$(ARCH)

all: modules

modules clean:
	$(MAKE) -C $(KERNELDIR) M=`pwd` $@ $(BUILD_ARGS)

modules_install install: modules
	$(MAKE) -C $(KERNELDIR) M=`pwd` modules_install $(INSTALL_ARGS)

endif # ifneq ($(KERNELRELEASE),)
