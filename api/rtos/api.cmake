genavb_target_add_srcs(TARGET ${avb}
  SRCS
  control.c
  fdb.c
  frame_preemption.c
  frer.c
  generic.c
  hsr.c
  init.c
  psfp.c
  qos.c
  scheduled_traffic.c
  stream_identification.c
  timer.c
  vlan.c
  ../error.c
  ../control.c
  ../clock.c
  ../version.c
  )

if(CONFIG_GENAVB_TSN_SOCKET)
  genavb_target_add_srcs(TARGET ${avb}
    SRCS
    socket.c
    ../socket.c
    )
endif()

if(CONFIG_GENAVB_TSN_AVTP)
  genavb_target_add_srcs(TARGET ${avb}
    SRCS
    streaming.c
    ../streaming.c
    )
endif()

if(CONFIG_GENAVB_TSN_DSA)
  genavb_target_add_srcs(TARGET ${avb}
    SRCS
    dsa.c
    )
endif()
