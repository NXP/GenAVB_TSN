if(CONFIG_SRP)

  genavb_conditional_include(${TARGET_OS}/srp.cmake)
  genavb_add_prebuilt_library(LIB libsrp-${TARGET_ARCH}.a)
  genavb_link_libraries(TARGET ${tsn} LIB libsrp-${TARGET_ARCH}.a)

endif()
