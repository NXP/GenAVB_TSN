if(CONFIG_AVTP)

  genavb_conditional_include(${TARGET_OS}/avtp.cmake)
  genavb_link_libraries(TARGET ${avb} LIB libavtp-${TARGET_ARCH}.a)

endif()
