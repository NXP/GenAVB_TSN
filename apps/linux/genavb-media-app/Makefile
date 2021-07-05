
#Set those below to match your environment
# CROSS_COMPILE: path to cros-compiler
# OBJDIR: path where generated binaries should be stored
# GENAVB_PATH: path where the genavb library is located
# GENAVB_INCLUDE: path to the public genavb includes files

CROSS_COMPILE?=/home/user/gcc-linaro-arm-linux-gnueabihf-4.8-2014.05_linux/bin/arm-linux-gnueabihf-
OBJDIR?=
GENAVB_PATH?=../../../lib
GENAVB_INCLUDE?=../../../include
APP_NAME=genavb-media-app

include ../common/gstreamer.inc

# Evaluate if CC has been set or not:
ifeq "$(origin CC)" "default"
CC:=$(CROSS_COMPILE)gcc
endif

CUSTOM_CFLAGS:=$(addprefix -D, $(CUSTOM_DEFINES))
CFLAGS= $(CUSTOM_CFLAGS) -O2 -Wall -Werror -g -lgenavb -pthread -ldl -L$(GENAVB_PATH) -I$(GENAVB_INCLUDE) -lasound $(GST_CFLAGS)

$(OBJDIR)$(APP_NAME): ../common/clock_domain.c ../common/thread.c ../common/alsa2.c ../common/clock.c ../common/log.c ../common/stats.c \
		../common/msrp.c ../common/avb_stream.c ../common/crf_stream.c thread_config.c alsa_config.c avb_stream_config.c main.c \
		../common/alsa_stream.c ../common/stream_stats.c ../common/time.c ../common/gstreamer.c ../common/gstreamer_multisink.c ../common/gstreamer_single.c \
		../common/common.c ../common/ts_parser.c ../common/file_buffer.c ../common/avb_stream.c ../common/gst_pipeline_definitions.c ../common/aecp.c \
		../common/gstreamer_custom_rt_pool.c gstreamer_stream.c multi_frame_sync.c salsa_camera.c h264_camera.c ../common/helpers.c ../common/timer.c
	$(CC) $(CFLAGS) -o $@ $^

install: $(OBJDIR)$(APP_NAME)
	install -D $(OBJDIR)$(APP_NAME) $(BIN_DIR)/$(APP_NAME)

clean:
	rm -rf $(OBJDIR)$(APP_NAME)
