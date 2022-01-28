#Set those below to match your environment
# CROSS_COMPILE: path to cross-compiler
# STAGING_DIR: path to staging directory
export CROSS_COMPILE?=/home/user/gcc-linaro-arm-linux-gnueabihf-4.9-2014.05_linux/bin/arm-linux-gnueabihf-
export STAGING_DIR?=/home/user/buildroot/output/staging
CONFIG_FILE?=../config

# Use the following targets to build related OS applications
oses:= linux

include rules.mk
include $(CONFIG_FILE)

help:
	@echo
	@echo 'Cleaning:'
	@echo '  clean                  - delete all apps files created by build'
	@echo
	@echo 'Build:'
	@echo '  linux                  - build linux apps'
	@echo

$(foreach os,$(oses),$(eval $(call apps_os_handler,$(os))))

