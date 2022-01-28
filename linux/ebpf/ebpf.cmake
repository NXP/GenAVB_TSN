if(NOT DEFINED FIRMWARE_DIR)
  message(FATAL_ERROR "FIRMWARE_DIR not defined")
endif()

set(GENAVB_XDP "../firmware/genavb-xdp.bin")

# TODO build firmware

install(FILES ${CMAKE_CURRENT_LIST_DIR}/${GENAVB_XDP} DESTINATION ${FIRMWARE_DIR})

