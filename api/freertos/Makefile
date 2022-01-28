avb-obj:= init.o control.o timer.o generic.o ../error.o ../control.o ../clock.o qos.o

ifeq ($(CONFIG_SOCKET),y)
avb-obj+= socket.o
endif

ifeq ($(CONFIG_AVTP),y)
avb-obj+= streaming.o ../streaming.o
endif
