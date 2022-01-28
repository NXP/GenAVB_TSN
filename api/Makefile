os_subdirs := linux freertos

genavb-obj:= error.o control.o clock.o

ifeq ($(CONFIG_AVTP),y)
genavb-obj+= streaming.o
endif

genavb-obj+= socket.o

ifeq ($(target_os), freertos)
avb-obj+= socket.o
endif

genavb-major:= 1
genavb-minor:= 0
genavb1733-obj:= 1733.o
genavb1733-major:= 1
genavb1733-minor:= 0

api_CFLAGS:= -iquote api/$(target_os)
