# set os dependent target name
# for freertos there is no separate target for tsn, so ${tsn} equals ${avb}
set(avb stack-freertos)
set(tsn ${avb})

include_directories("${FREERTOS_APP_INCLUDES}")
include_directories("${FREERTOS_SDK_TARGET_DIRS}")
include_directories("${FREERTOS_BOARD_INCLUDE}")
include_directories("${FREERTOS_SDK_DEVICE_DIRS}")
include_directories("${FREERTOS_DIR}/include")
include_directories("${FREERTOS_DIR}/portable/${FREERTOS_PORT}")
include_directories("${FREERTOS_SDK}/components/phy")
include_directories(${FREERTOS_SDK_DRIVERS_DIRS})

genavb_add_os_library(NAME ${avb}
  SRCS
  assert.c
  atomic.c
  avb_queue.c
  avtp.c
  clock.c
  debug_print.c
  fdb.c
  fp.c
  fqtss.c
  gpt.c
  gptp_dev.c
  hr_timer.c
  hw_clock.c
  hw_timer.c
  ipc.c
  l2.c
  log.c
  mrp.c
  net.c
  net_logical_port.c
  net_phy.c
  net_port.c
  net_port_enet.c
  net_port_enet_qos.c
  net_port_enet_qos_stats.c
  net_port_enet_stats.c
  net_port_stats.c
  net_rx.c
  net_socket.c
  net_task.c
  net_tx.c
  pi.c
  ptp.c
  qos.c
  rational.c
  stats_task.c
  stdlib.c
  string.c
  timer.c
)

if(CONFIG_AVTP)
  genavb_target_add_srcs(TARGET ${avb}
    SRCS
    gpt_rec.c
    imx-pll.c
    media.c
    media_clock.c
    media_clock_gen_ptp.c
    media_clock_rec_pll.c
    media_queue.c
    mtimer.c
    )
endif()
