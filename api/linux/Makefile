ifeq ($(CONFIG_1733),y)
libs := genavb1733
genavb1733_LDFLAGS:=--version-script=api/linux/apis_1733.map
else
libs := genavb
genavb_LDFLAGS:=--version-script=api/linux/apis.map
endif

ifeq ($(CONFIG_NET_STD),y)
CFLAGS+= -DCONFIG_LIB_DEFAULT_NET=NET_STD -DCONFIG_1733_DEFAULT_NET=NET_STD
else ifeq ($(CONFIG_NET_XDP),y)
CFLAGS+= -DCONFIG_LIB_DEFAULT_NET=NET_STD -DCONFIG_1733_DEFAULT_NET=NET_STD
endif

genavb-obj := init.o control.o

ifeq ($(CONFIG_AVTP),y)
genavb-obj+= streaming.o
endif

genavb1733-obj:= 1733.o

ifeq ($(CONFIG_SOCKET),y)
genavb-obj+= socket.o
endif
