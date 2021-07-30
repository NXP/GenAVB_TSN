libs:= avb

avb-execs:= avb

avb-obj:= log.o string.o stdlib.o timer.o clock.o hr_timer.o hw_timer.o gpt.o net.o net_task.o \
	  net_socket.o net_rx.o net_tx.o net_port.o net_port_enet.o net_phy.o ptp.o ipc.o mrp.o avtp.o \
	  net_logical_port.o assert.o debug_print.o hw_clock.o gptp_dev.o atomic.o avb_queue.o \
	  stats_task.o l2.o pi.o rational.o fqtss.o net_port_enet_qos.o qos.o net_port_stats.o \
	  net_port_enet_qos_stats.o net_port_enet_stats.o fp.o fdb.o

ifeq ($(CONFIG_AVTP),y)
avb-obj+= media_clock.o mtimer.o media_clock_rec_pll.o media_clock_gen_ptp.o media.o media_queue.o \
	  imx-pll.o gpt_rec.o
endif

ifneq ($(FREERTOS_SDK),)
CFLAGS+= -I $(FREERTOS_SDK)/CMSIS/Core/Include
CFLAGS+= -I $(FREERTOS_SDK)/components/serial_manager
CFLAGS+= -I $(FREERTOS_SDK)/components/uart
CFLAGS+= -I $(FREERTOS_SDK)/components/phy
endif

ifneq ($(FREERTOS_DIR),)
CFLAGS+= -I $(FREERTOS_DIR)/include
CFLAGS+= -I $(FREERTOS_DIR)/include/private
ifneq ($(FREERTOS_PORT),)
CFLAGS+= -I $(FREERTOS_DIR)/portable/$(FREERTOS_PORT)
endif
endif

ifneq ($(FREERTOS_APP_INCLUDE),)
CFLAGS+= -I $(FREERTOS_APP_INCLUDE)
endif

ifneq ($(FREERTOS_BOARD_INCLUDE),)
CFLAGS+= -I $(FREERTOS_BOARD_INCLUDE)
endif

ifneq ($(FREERTOS_DEVICE),)
CFLAGS+= -I $(FREERTOS_DEVICE)
CFLAGS+= -I $(FREERTOS_DEVICE)/drivers
CFLAGS+= -I $(FREERTOS_DEVICE)/drivers/cm7
endif
