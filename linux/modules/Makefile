ifeq ($(config),)
$(error config variable is not set)
endif

ifeq ($(target),)
$(error target variable is not set)
endif

CUR_DIR := $(shell pwd)
MODULES_OBJ_DIR ?= $(CUR_DIR)/build/$(target)/$(config)
GENAVB_ROOT_DIR ?= $(CUR_DIR)/../..
GENAVB_INCLUDE ?= $(GENAVB_ROOT_DIR)/include
PREFIX ?= $(MODULES_OBJ_DIR)/target

include $(GENAVB_ROOT_DIR)/linux/osal/configs/config
include $(GENAVB_ROOT_DIR)/$(osal_config_file)

MODULES_SRC := $(GENAVB_ROOT_DIR)/linux/modules/common/*
ifeq ($(CONFIG_NET_STD), y)
MODULES_SRC += $(GENAVB_ROOT_DIR)/linux/modules/std/*
MODULES_SRC += $(GENAVB_ROOT_DIR)/common/os/queue_common.*
else ifeq ($(CONFIG_NET_XDP), y)
MODULES_SRC += $(GENAVB_ROOT_DIR)/linux/modules/std/*
MODULES_SRC += $(GENAVB_ROOT_DIR)/common/os/queue_common.*
else
MODULES_SRC += $(GENAVB_ROOT_DIR)/linux/modules/avb/*
MODULES_SRC += $(GENAVB_ROOT_DIR)/common/os/*
MODULES_SRC += $(GENAVB_ROOT_DIR)/public/sr_class.c
MODULES_SRC += $(GENAVB_ROOT_DIR)/public/qos.c
endif

CONFIG_FILE := $(MODULES_OBJ_DIR)/config
AUTOCONF := $(MODULES_OBJ_DIR)/autoconf.h

all: modules

$(MODULES_OBJ_DIR)/modules.stamp: $(MODULES_SRC)
	@echo Generate modules sources in $(MODULES_OBJ_DIR)
	@mkdir -p $(MODULES_OBJ_DIR)
	@cp -a $(MODULES_SRC) $(MODULES_OBJ_DIR)
	@touch $@

modules_sources: $(MODULES_OBJ_DIR)/modules.stamp

$(CONFIG_FILE): $(MODULES_OBJ_DIR)/modules.stamp
	@echo Generating config file
	@cp $(GENAVB_ROOT_DIR)/configs/config_$(config) $(CONFIG_FILE)
	@cat $(GENAVB_ROOT_DIR)/$(osal_config_file) >> $(CONFIG_FILE)

$(AUTOCONF): $(CONFIG_FILE)
	@echo Generating autoconf.h
	@echo "/*\n * Auto-generated file\n * Do not modify\n */" > $(AUTOCONF)
	@grep ^CONFIG.*=y$ <$(CONFIG_FILE) | sed -e 's/CONFIG/#define &/' -e 's/=y/ 1/' >> $(AUTOCONF)
	@echo "#define CONFIG_$(target) 1" >> $(AUTOCONF)

install modules_install: modules

modules modules_install install clean: $(CONFIG_FILE) $(AUTOCONF)
	$(MAKE) -C $(MODULES_OBJ_DIR) $@ MAKEFLAGS= GENAVB_INCLUDE=$(GENAVB_INCLUDE) CONFIG_FILE=$(CONFIG_FILE) AUTOCONF=$(AUTOCONF) PREFIX=$(PREFIX)

.PHONY: modules modules_sources modules_install install clean
