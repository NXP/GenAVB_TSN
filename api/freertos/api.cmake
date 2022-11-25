genavb_target_add_srcs(TARGET ${avb}
  SRCS
  init.c
  control.c
  timer.c
  generic.c
  qos.c
  ../error.c
  ../control.c
  ../clock.c
  )

if(CONFIG_SOCKET)
  genavb_target_add_srcs(TARGET ${avb}
    SRCS
    socket.c
    ../socket.c
    )
endif()

if(CONFIG_AVTP)
  genavb_target_add_srcs(TARGET ${avb}
    SRCS
    streaming.c
    ../streaming.c
    )
endif()

