
#Set those below to match your environment
# CROSS_COMPILE: path to cros-compiler
# OBJDIR: path where generated binaries should be stored
# GENAVB_PATH: path where the genavb library is located
# GENAVB_INCLUDE: path to the public genavb includes files

CROSS_COMPILE?=/home/user/dev/gcc-linaro-arm-linux-gnueabihf-4.8-2014.04_linux/bin/arm-linux-gnueabihf-
STAGING_DIR?=/home/user/dev/buildroot/output/staging
OBJDIR?=
GENAVB_PATH?=../../../lib
GENAVB_INCLUDE?=../../../include
APP_NAME=alsa-audio-app

# Evaluate if CC has been set or not:
ifeq "$(origin CC)" "default"
CC:=$(CROSS_COMPILE)gcc
endif

CUSTOM_CFLAGS:=$(addprefix -D, $(CUSTOM_DEFINES))
CFLAGS= $(CUSTOM_CFLAGS) -O2 -Wall -Werror -g -lgenavb -L$(GENAVB_PATH) -I$(GENAVB_INCLUDE) -lasound -lm -L$(STAGING_DIR)/usr/lib -I$(STAGING_DIR)/usr/include

$(OBJDIR)$(APP_NAME): main.c ../common/alsa.c ../common/stats.c ../common/time.c
	$(CC) $(CFLAGS) -o $@ $^

install: $(OBJDIR)$(APP_NAME)
	install -D $(OBJDIR)$(APP_NAME) $(BIN_DIR)/$(APP_NAME)

clean:
	rm -rf $(OBJDIR)$(APP_NAME)
