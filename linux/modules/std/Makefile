# If KERNELRELEASE is not empty, we are being invoked from the KBuild system
ifneq ($(KERNELRELEASE),)

obj-m += avb.o

avb-y = ipc_module.o ipc.o pool.o queue.o

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
