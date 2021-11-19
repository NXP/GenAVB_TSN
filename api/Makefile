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

api_CFLAGS:= -iquote api/$(target_os)
