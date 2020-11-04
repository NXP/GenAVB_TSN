ifeq ($(KERNELRELEASE),)
src:=.
endif

CONFIG_FILE?=$(src)/../../config

include $(CONFIG_FILE)

ifeq ($(CONFIG_NET_STD), y)
objs += ipc_module.o ipc.o pool.o queue.o
else
objs += avbdrv.o netdrv.o net_rx.o net_socket.o ipc.o pool.o pool_dma.o net_port.o avtp.o ptp.o mrp.o ipv4.o \
	ipv6.o rtp.o queue.o dmadrv.o cs2000.o gic.o epit.o net_tx.o debugfs.o media.o media_clock.o \
	media_clock_drv.o media_clock_rec_pll.o media_clock_gen_ptp.o imx-pll.o hw_timer.o ftm.o pi.o \
	rational.o mle145170.o gpt.o stats.o net_logical_port.o net_bridge.o mtimer_drv.o mtimer.o \
	sr_class.o qos.o
endif

ifeq ($(CONFIG_SJA1105),y)
objs += sja1105.o
endif

# If KERNELRELEASE is not empty, we are being invoked from the KBuild system
ifneq ($(KERNELRELEASE),)

GENAVB_INCLUDE?=$(src)/../../include
AUTOCONF?=$(GENAVB_INCLUDE)/genavb/autoconf.h

ccflags-y := -I$(GENAVB_INCLUDE) -I$(GENAVB_INCLUDE)/linux
ccflags-y += -include$(AUTOCONF)

obj-m += avb.o

KBUILD_EXTRA_SYMBOLS?=

avb-y = $(objs)

ifeq ($(CONFIG_SJA1105),y)
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

devkit-install:
	cp $(objs:.o=.[ch]) $(DEVKIT_DIR)
	cp Makefile $(DEVKIT_DIR)

endif # ifneq ($(KERNELRELEASE),)
