
#Set those below to match your environment
# CROSS_COMPILE: path to cros-compiler
# OBJDIR: path where generated binaries should be stored
# GENAVB_PATH: path where the genavb library is located
# GENAVB_INCLUDE: path to the public genavb includes files

CROSS_COMPILE?=/home/user/gcc-linaro-arm-linux-gnueabihf-4.8-2014.05_linux/bin/arm-linux-gnueabihf-
OBJDIR?=
GENAVB_INCLUDE?=../../../include
APP_NAME=salsacamctrl

# Evaluate if CC has been set or not:
ifeq "$(origin CC)" "default"
CC:=$(CROSS_COMPILE)gcc
endif

CUSTOM_CFLAGS:=$(addprefix -D, $(CUSTOM_DEFINES))
CFLAGS= $(CUSTOM_CFLAGS) -O2 -Wall -Werror -g -I$(GENAVB_INCLUDE)

$(OBJDIR)$(APP_NAME): main.c udpcc2.c ../common/helpers.c ../../../public/helpers.c
	$(CC) $(CFLAGS) -o $@ $^


install: $(OBJDIR)$(APP_NAME) salsacam-cmd.sh salsacam-setup.sh salsacam-configs.inc
	install -D $(OBJDIR)$(APP_NAME) $(PREFIX)/usr/bin/$(APP_NAME)
	install -D salsacam-cmd.sh $(PREFIX)/usr/bin/salsacam-cmd.sh
	install -D salsacam-setup.sh $(PREFIX)/usr/bin/salsacam-setup.sh
	install -D salsacam-configs.inc $(PREFIX)/etc/genavb/salsacam-configs.inc

clean:
	rm -rf $(OBJDIR)$(APP_NAME)

