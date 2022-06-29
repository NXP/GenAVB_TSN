if(CONFIG_AVDECC)

  genavb_conditional_include(${TARGET_OS}/avdecc.cmake)
  genavb_link_libraries(TARGET ${avb} LIB libavdecc-${TARGET_ARCH}.a)

endif()
