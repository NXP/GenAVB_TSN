if(CONFIG_GENAVB_TSN_API)

  genavb_include_os(${TARGET_OS}/api.cmake)

  genavb_target_add_srcs(TARGET genavb
    SRCS
    error.c
    control.c
    clock.c
    version.c
  )

  if(CONFIG_GENAVB_TSN_AVTP)
    genavb_target_add_srcs(TARGET genavb SRCS streaming.c)
  endif()

  genavb_target_add_srcs(TARGET genavb SRCS socket.c)

endif()

