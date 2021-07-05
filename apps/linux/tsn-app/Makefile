
#Set those below to match your environment
# CROSS_COMPILE: path to cros-compiler
# OBJDIR: path where generated binaries should be stored
# GENAVB_PATH: path where the genavb library is located
# GENAVB_INCLUDE: path to the public genavb includes files

CROSS_COMPILE?=/home/user/gcc-linaro-arm-linux-gnueabihf-4.8-2014.05_linux/bin/arm-linux-gnueabihf-
OBJDIR?=
GENAVB_PATH?=../../../lib
GENAVB_INCLUDE?=../../../include
APP_NAME=tsn-app
OPCUA_SUPPORT?=0

include ../common/open62541.inc

# Evaluate if CC has been set or not:
ifeq "$(origin CC)" "default"
CC:=$(CROSS_COMPILE)gcc
endif

CUSTOM_CFLAGS:=$(addprefix -D, $(CUSTOM_DEFINES))
CFLAGS= $(CUSTOM_CFLAGS) -O2 -Wall -Werror -g -lgenavb -L$(GENAVB_PATH) -I$(GENAVB_INCLUDE) -pthread -ldl -DSTATS_LOG

srcs:= main.c tsn_task.c tsn_tasks_config.c thread_config.c cyclic_task.c serial_controller.c network_only.c tsn_timer.c \
       ../common/stats.c ../common/stats.c ../common/thread.c ../common/log.c ../common/time.c \
       ../common/timer.c ../common/helpers.c

ifeq ($(OPCUA_SUPPORT), 1)
	srcs+= opcua/opcua_server.c opcua/model/tsn_app_model.c
	CFLAGS+= $(OPCUA_CFLAGS) -DOPCUA_SUPPORT
endif

$(OBJDIR)$(APP_NAME): $(srcs)
	$(CC) $(CFLAGS) -o $@ $^

install: $(OBJDIR)$(APP_NAME)
	install -D $(OBJDIR)$(APP_NAME) $(BIN_DIR)/$(APP_NAME)

clean:
	rm -rf $(OBJDIR)$(APP_NAME)
