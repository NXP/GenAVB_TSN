
#Set those below to match your environment
# CROSS_COMPILE: path to cross-compiler
# OBJDIR: path where generated binaries should be stored
# GENAVB_PATH: path where the genavb library is located
# GENAVB_INCLUDE: path to the public genavb includes files

CROSS_COMPILE?=/home/user/gcc-linaro-arm-linux-gnueabihf-4.9-2014.05_linux/bin/arm-linux-gnueabihf-
STAGING_DIR?=/home/user/dev/buildroot/output/staging
OBJDIR?=
GENAVB_PATH?=../../../lib
GENAVB_INCLUDE?=../../../include
APP_NAME=genavb-video-player-app

include ../common/gstreamer.inc

# Evaluate if CC has been set or not:
ifeq "$(origin CC)" "default"
CC:=$(CROSS_COMPILE)gcc
endif

CUSTOM_CFLAGS:=$(addprefix -D, $(CUSTOM_DEFINES))
CFLAGS= $(CUSTOM_CFLAGS) -O2 -Wall -Werror -g -lgenavb -L$(GENAVB_PATH) -I$(GENAVB_INCLUDE) $(GST_CFLAGS)

$(OBJDIR)$(APP_NAME): main.c ../common/common.c ../common/stats.c ../common/time.c gstreamer.c ../common/ts_parser.c ../common/file_buffer.c gst_pipelines.c gstreamer_single.c ../common/helpers.c
	$(CC) $(CFLAGS) -o $@ $^

install: $(OBJDIR)$(APP_NAME) salsa-camera.sh
	install -D $(OBJDIR)$(APP_NAME) $(BIN_DIR)/$(APP_NAME)
	install -D salsa-camera.sh $(BIN_DIR)/salsa-camera.sh

clean:
	rm -rf $(OBJDIR)$(APP_NAME)
