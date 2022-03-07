
genavb_target_add_srcs(TARGET genavb
  SRCS
  init.c
  control.c
  )

if(CONFIG_NET_STD)
  add_definitions(-DCONFIG_LIB_DEFAULT_NET=NET_STD -DCONFIG_1733_DEFAULT_NET=NET_STD)
elseif(CONFIG_NET_XDP)
  add_definitions(-DCONFIG_LIB_DEFAULT_NET=NET_STD -DCONFIG_1733_DEFAULT_NET=NET_STD)
endif()

if(CONFIG_AVTP)
  genavb_target_add_srcs(TARGET genavb
    SRCS
    streaming.c
    )
endif()

genavb_target_add_srcs(TARGET genavb1733 SRCS 1733.c)

if(CONFIG_SOCKET)
  genavb_target_add_srcs(TARGET genavb
    SRCS
    socket.c
    )
endif()
