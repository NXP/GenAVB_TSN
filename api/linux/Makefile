libs := genavb
genavb_LDFLAGS:=--version-script=api/linux/apis.map

ifeq ($(CONFIG_NET_STD),y)
CFLAGS+= -DCONFIG_LIB_DEFAULT_NET=NET_STD -DCONFIG_1733_DEFAULT_NET=NET_STD
else ifeq ($(CONFIG_NET_XDP),y)
CFLAGS+= -DCONFIG_LIB_DEFAULT_NET=NET_STD -DCONFIG_1733_DEFAULT_NET=NET_STD
endif

genavb-obj := init.o control.o

ifeq ($(CONFIG_AVTP),y)
genavb-obj+= streaming.o
endif


ifeq ($(CONFIG_SOCKET),y)
genavb-obj+= socket.o
endif
