if(CONFIG_MAAP)

  genavb_conditional_include(${TARGET_OS}/maap.cmake)
  genavb_add_prebuilt_library(LIB libmaap-${TARGET_ARCH}.a)
  genavb_link_libraries(TARGET ${avb} LIB libmaap-${TARGET_ARCH}.a)

endif()
