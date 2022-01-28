
#Set those below to match your environment
# CROSS_COMPILE: path to cross-compiler
# OBJDIR: path where generated binaries should be stored
# GENAVB_PATH: path where the genavb library is located
# GENAVB_INCLUDE: path to the public genavb includes files

CROSS_COMPILE?=/home/user/gcc-linaro-arm-linux-gnueabihf-4.9-2014.05_linux/bin/arm-linux-gnueabihf-
OBJDIR?=
GENAVB_PATH?=../../../lib
GENAVB_INCLUDE?=../../../include
APP_NAME=genavb-video-server-app

include ../common/gstreamer.inc

# Evaluate if CC has been set or not:
ifeq "$(origin CC)" "default"
CC:=$(CROSS_COMPILE)gcc
endif

CUSTOM_CFLAGS:=$(addprefix -D, $(CUSTOM_DEFINES))
CFLAGS=$(CUSTOM_CFLAGS) -O2 -Wall -Werror -g -pthread -lgenavb -L$(GENAVB_PATH) -I$(GENAVB_INCLUDE) $(GST_CFLAGS)

$(OBJDIR)$(APP_NAME): main.c ../common/common.c ../common/stats.c ../common/time.c ../common/ts_parser.c ../common/file_buffer.c ../common/aecp.c \
						../common/gstreamer.c ../common/gst_pipeline_definitions.c ../common/gstreamer_multisink.c ../common/gstreamer_custom_rt_pool.c
	$(CC) $(CFLAGS) -o $@ $^

install: $(OBJDIR)$(APP_NAME)
	install -D $(OBJDIR)$(APP_NAME) $(BIN_DIR)/$(APP_NAME)

clean:
	rm -rf $(OBJDIR)$(APP_NAME)
