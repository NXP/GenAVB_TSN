fgptp-execs:= fgptp
avb-execs:=avb

genavb-exec:= $(CONFIG_AVTP)$(CONFIG_AVDECC)$(CONFIG_MAAP)$(CONFIG_SRP)

ifneq ($(genavb-exec), )
execs:=$(avb-execs)
endif

ifeq ($(CONFIG_GPTP),y)
execs+=$(fgptp-execs)
endif

$(avb-execs)-obj:= assert.o stdlib.o string.o avb_main.o net.o log.o timer.o ipc.o clock.o cfgfile.o epoll.o init.o os_config.o net_logical_port.o fdb.o

$(avb-execs)_CFLAGS+= -lm -L$(STAGING_DIR)/usr/lib

genavb-obj:= ipc.o log.o clock.o string.o stdlib.o epoll.o init.o assert.o cfgfile.o os_config.o net_logical_port.o


$(fgptp-execs)-obj:= fgptp_main.o stdlib.o string.o net.o log.o timer.o clock.o cfgfile.o epoll.o ipc.o init.o assert.o os_config.o net_logical_port.o

ifeq ($(CONFIG_NET_STD),y)
$(avb-execs)-obj+= net_std.o net_std_socket_filters.o rtnetlink.o fqtss_std.o fdb_std.o
$(fgptp-execs)-obj+= net_std.o net_std_socket_filters.o
else
$(avb-execs)-obj+= net_avb.o shmem.o fqtss_avb.o
$(fgptp-execs)-obj+= net_avb.o shmem.o
endif

ifeq ($(CONFIG_AVTP),y)
$(avb-execs)-obj+= media_clock.o media.o timer_media.o
$(fgptp-execs)-obj+= timer_media.o
endif

ifeq ($(CONFIG_SJA1105),y)
CFLAGS+= -I$(NXP_SWITCH_PATH)/drivers/modules
$(avb-execs)-obj+= fqtss_sja.o fdb_sja.o rtnetlink.o
endif

ifeq ($(CONFIG_SOCKET),y)
ifeq ($(CONFIG_NET_STD),y)
genavb-obj+= net.o net_std.o net_std_socket_filters.o
else
genavb-obj+= net.o net_avb.o shmem.o
endif
endif
