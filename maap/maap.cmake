if(CONFIG_MAAP)

  genavb_conditional_include(${TARGET_OS}/maap.cmake)
  genavb_link_libraries(TARGET ${avb} LIB libmaap-${TARGET_ARCH}.a)

endif()
