libs := genavb
genavb_LDFLAGS:=--version-script=api/linux/apis.map

genavb-obj := init.o control.o

ifeq ($(CONFIG_AVTP),y)
genavb-obj+= streaming.o
endif


ifeq ($(CONFIG_SOCKET),y)
genavb-obj+= socket.o
endif
