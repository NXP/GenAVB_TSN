if(CONFIG_GPTP)

  genavb_conditional_include(${TARGET_OS}/gptp.cmake)
  genavb_add_prebuilt_library(LIB libgptp-${TARGET_ARCH}.a)
  genavb_link_libraries(TARGET ${tsn} LIB libgptp-${TARGET_ARCH}.a)

endif()
