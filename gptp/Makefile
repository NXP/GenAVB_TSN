ifeq ($(CONFIG_GPTP),y)
os_subdirs:= linux freertos

ifeq ($(target_os), freertos)
$(avb-execs)-ar:= gptp.a
endif

$(fgptp-execs)-ar:= gptp.a
endif

archives:= gptp

