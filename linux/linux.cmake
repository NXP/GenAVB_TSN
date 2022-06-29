include(${CMAKE_CURRENT_LIST_DIR}/osal/configs/config.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/configs/configs.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/scripts/scripts.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/ebpf/ebpf.cmake)

if(CONFIG_AVTP OR CONFIG_AVDECC OR CONFIG_MAAP)
  set(avb avb)
endif()

if(CONFIG_GPTP OR CONFIG_SRP OR CONFIG_MANAGEMENT)
  set(tsn tsn)
endif()

# Include modules
include_directories("${CMAKE_BINARY_DIR}")

genavb_add_executable(NAME ${avb}
  SRCS
  assert.c
  stdlib.c
  string.c
  avb_main.c
  net.c
  log.c
  timer.c
  ipc.c
  clock.c
  cfgfile.c
  epoll.c
  init.c
  os_config.c
  net_logical_port.c
  )

if(CONFIG_1733)
  set(genavb genavb1733)
  genavb_add_shared_library(NAME ${genavb}
    SRCS
    stdlib.c
    string.c
    net.c
    log.c
    clock.c
    epoll.c
    init.c
    assert.c
    cfgfile.c
    os_config.c
    net_logical_port.c
  )
else()
  set(genavb genavb)
  genavb_add_shared_library(NAME ${genavb}
    SRCS
    ipc.c
    log.c
    clock.c
    string.c
    stdlib.c
    epoll.c
    init.c
    assert.c
    cfgfile.c
    os_config.c
    net_logical_port.c
  )
endif()

genavb_add_executable(NAME ${tsn}
  SRCS
  tsn_main.c
  stdlib.c
  string.c
  net.c
  log.c
  timer.c
  clock.c
  cfgfile.c
  epoll.c
  ipc.c
  init.c
  assert.c
  os_config.c
  net_logical_port.c
  fdb.c
)

# Backward compatibility
genavb_add_executable_alias("${tsn}" "fgptp")

genavb_add_dependencies(TARGET ${avb} DEP modules-dir)
genavb_add_dependencies(TARGET ${tsn} DEP modules-dir)
genavb_add_dependencies(TARGET genavb DEP modules-dir)
genavb_add_dependencies(TARGET genavb1733 DEP modules-dir)

if(CONFIG_NET_STD)
  add_definitions(-DCONFIG_AVB_DEFAULT_NET=NET_STD -DCONFIG_TSN_DEFAULT_NET=NET_STD)
  genavb_target_add_srcs(TARGET ${avb} SRCS net_std.c net_std_socket_filters.c)
  genavb_target_add_srcs(TARGET ${tsn} SRCS net_std.c net_std_socket_filters.c rtnetlink.c fqtss.c fqtss_std.c fdb_std.c)
  genavb_target_add_srcs(TARGET genavb1733 SRCS net_std.c net_std_socket_filters.c rtnetlink.c fqtss.c fqtss_std.c)
elseif(CONFIG_NET_XDP)
  add_definitions(-DCONFIG_AVB_DEFAULT_NET=NET_STD -DCONFIG_TSN_DEFAULT_NET=NET_STD)
  genavb_target_add_srcs(TARGET ${avb} SRCS net_std.c net_std_socket_filters.c)
  genavb_target_add_srcs(TARGET ${tsn} SRCS net_std.c net_std_socket_filters.c rtnetlink.c fqtss.c fqtss_std.c fdb_std.c)
  genavb_target_add_srcs(TARGET genavb1733 SRCS net_std.c net_std_socket_filters.c rtnetlink.c fqtss.c fqtss_std.c)
else()
  genavb_target_add_srcs(TARGET ${avb} SRCS net_avb.c shmem.c)
  genavb_target_add_srcs(TARGET ${tsn} SRCS net_avb.c shmem.c fqtss.c fqtss_avb.c)
  genavb_target_add_srcs(TARGET genavb1733 SRCS net_avb.c shmem.c fqtss.c fqtss_avb.c)
endif()

if(CONFIG_AVTP)
  genavb_target_add_srcs(TARGET ${avb} SRCS media_clock.c media.c timer_media.c)
  genavb_target_add_srcs(TARGET ${tsn} SRCS timer_media.c)
endif()

if(CONFIG_SJA1105)
  include_directories("${NXP_SWITCH_PATH}/drivers/modules")
  genavb_target_add_srcs(TARGET ${tsn} SRCS fqtss_sja.c fdb_sja.c rtnetlink.c)
endif()

if(CONFIG_SOCKET)
  if(CONFIG_NET_STD)
    genavb_target_add_srcs(TARGET genavb SRCS net.c net_std.c net_std_socket_filters.c)
  elseif(CONFIG_NET_XDP)
    include_directories("${KERNELDIR}/tools/lib")
    target_link_libraries(genavb bpf -L${KERNELDIR}/tools/lib/bpf)
    genavb_target_add_srcs(TARGET genavb SRCS net.c net_xdp.c pool.c net_std.c net_std_socket_filters.c)
  else()
    genavb_target_add_srcs(TARGET genavb SRCS net.c net_avb.c shmem.c)
  endif()
endif()
