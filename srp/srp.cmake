if(CONFIG_SRP)

  genavb_conditional_include(${TARGET_OS}/srp.cmake)
  genavb_link_libraries(TARGET ${avb} LIB libsrp-${TARGET_ARCH}.a)

endif()
