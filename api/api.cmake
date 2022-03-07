if(CONFIG_API)

  genavb_include_os(${TARGET_OS}/api.cmake)

  genavb_target_add_srcs(TARGET genavb
    SRCS
    error.c
    control.c
    clock.c
  )

  if(CONFIG_AVTP)
    genavb_target_add_srcs(TARGET genavb SRCS streaming.c)
  endif()

  genavb_target_add_srcs(TARGET genavb SRCS socket.c)

  genavb_target_add_srcs(TARGET genavb1733 SRCS 1733.c)

endif()

