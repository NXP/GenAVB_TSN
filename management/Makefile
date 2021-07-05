ifeq ($(CONFIG_MANAGEMENT),y)
os_subdirs:= linux freertos

ifeq ($(target_os), freertos)
$(avb-execs)-ar:= management.a
endif

$(fgptp-execs)-ar:= management.a
endif

archives:= management

