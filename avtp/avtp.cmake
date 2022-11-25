if(CONFIG_AVTP)

  genavb_conditional_include(${TARGET_OS}/avtp.cmake)
  genavb_add_prebuilt_library(LIB libavtp-${TARGET_ARCH}.a)
  genavb_link_libraries(TARGET ${avb} LIB libavtp-${TARGET_ARCH}.a)

endif()
